/*
    Ousía
    Copyright (C) 2014, 2015  Benjamin Paaßen, Andreas Stöckel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <core/common/Logger.hpp>
#include <core/common/Utils.hpp>
#include <core/common/Exceptions.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

#include "Handler.hpp"
#include "Stack.hpp"
#include "State.hpp"

namespace ousia {
namespace parser_stack {

/* Class HandlerInfo */

HandlerInfo::HandlerInfo() : HandlerInfo(nullptr) {}

HandlerInfo::HandlerInfo(std::shared_ptr<Handler> handler)
    : handler(handler),
      fieldIdx(0),
      valid(true),
      implicit(false),
      inField(false),
      inDefaultField(false),
      inImplicitDefaultField(false),
      inValidField(false),
      hadDefaultField(false)
{
}

HandlerInfo::HandlerInfo(bool valid, bool implicit, bool inField,
                         bool inDefaultField, bool inImplicitDefaultField,
                         bool inValidField)
    : handler(nullptr),
      fieldIdx(0),
      valid(valid),
      implicit(implicit),
      inField(inField),
      inDefaultField(inDefaultField),
      inImplicitDefaultField(inImplicitDefaultField),
      inValidField(inValidField),
      hadDefaultField(false)
{
}

HandlerInfo::~HandlerInfo()
{
	// Do nothing
}

void HandlerInfo::fieldStart(bool isDefault, bool isImplicit, bool isValid)
{
	inField = true;
	inDefaultField = isDefault || isImplicit;
	inImplicitDefaultField = isImplicit;
	inValidField = isValid;
	fieldIdx++;
}

void HandlerInfo::fieldEnd()
{
	hadDefaultField = hadDefaultField || inDefaultField;
	inField = false;
	inDefaultField = false;
	inImplicitDefaultField = false;
	inValidField = false;
}

/**
 * Stub instance of HandlerInfo containing no handler information.
 */
static HandlerInfo EmptyHandlerInfo{true, true, true, true, false, true};

/* Helper functions */

/**
 * Returns an Exception that should be thrown when a currently invalid command
 * is thrown.
 *
 * @param name is the name of the command for which no state transition is
 * found.
 * @param expected is a set containing the names of the expected commands.
 */
static LoggableException buildInvalidCommandException(
    const std::string &name, const std::set<std::string> &expected)
{
	if (expected.empty()) {
		return LoggableException{
		    std::string{"No nested elements allowed, but got \""} + name +
		    std::string{"\""}};
	} else {
		return LoggableException{
		    std::string{"Expected "} +
		    (expected.size() == 1 ? std::string{"\""}
		                          : std::string{"one of \""}) +
		    Utils::join(expected, "\", \"") + std::string{"\", but got \""} +
		    name + std::string{"\""}};
	}
}

/* Class Stack */

Stack::Stack(ParserContext &ctx,
             const std::multimap<std::string, const State *> &states)
    : ctx(ctx), states(states)
{
	// If the scope instance is not empty we need to deduce the current parser
	// state
	if (!ctx.getScope().isEmpty()) {
		deduceState();
	}
}

Stack::~Stack()
{
	while (!stack.empty()) {
		// Fetch the topmost stack element
		HandlerInfo &info = currentInfo();

		// It is an error if we're still in a field of an element while the
		// Stack instance is destroyed. Log that
		if (handlersValid()) {
			if (info.inField && !info.implicit &&
			    !info.inImplicitDefaultField) {
				logger().error(
				    std::string("Reached end of stream, but command \"") +
				        info.handler->getName() +
				        "\" has not ended yet. Command was started here:",
				    info.handler->getLocation());
			}
		}

		// Remove the command from the stack
		endCurrentHandler();
	}
}

void Stack::deduceState()
{
	// Assemble all states
	std::vector<const State *> states;
	for (const auto &e : this->states) {
		states.push_back(e.second);
	}

	// Fetch the type signature of the scope and derive all possible states,
	// abort if no unique parser state was found
	std::vector<const State *> possibleStates =
	    StateDeductor(ctx.getScope().getStackTypeSignature(), states).deduce();
	if (possibleStates.size() != 1U) {
		throw LoggableException(
		    "Error while including file: Cannot deduce parser state.");
	}

	// Switch to this state by creating a handler, but do not call its start
	// function
	const State &state = *possibleStates[0];
	HandlerConstructor ctor =
	    state.elementHandler ? state.elementHandler : EmptyHandler::create;

	std::shared_ptr<Handler> handler =
	    std::shared_ptr<Handler>{ctor({ctx, "", state, SourceLocation{}})};
	stack.emplace_back(handler);

	// Set the correct flags for this implicit handler
	HandlerInfo &info = currentInfo();
	info.implicit = true;
	info.fieldStart(true, false, true);
}

std::set<std::string> Stack::expectedCommands()
{
	const State *currentState = &(this->currentState());
	std::set<std::string> res;
	for (const auto &v : states) {
		if (v.second->parents.count(currentState)) {
			res.insert(v.first);
		}
	}
	return res;
}

const State &Stack::currentState()
{
	return stack.empty() ? States::None : stack.back().handler->getState();
}

std::string Stack::currentCommandName()
{
	return stack.empty() ? std::string{} : stack.back().handler->getName();
}

const State *Stack::findTargetState(const std::string &name)
{
	const State *currentState = &(this->currentState());
	auto range = states.equal_range(name);
	for (auto it = range.first; it != range.second; it++) {
		const StateSet &parents = it->second->parents;
		if (parents.count(currentState) || parents.count(&States::All)) {
			return it->second;
		}
	}

	return nullptr;
}

const State *Stack::findTargetStateOrWildcard(const std::string &name)
{
	// Try to find the target state with the given name, if none is found, try
	// find a matching "*" state.
	State const *targetState = findTargetState(name);
	if (targetState == nullptr) {
		return findTargetState("*");
	}
	return targetState;
}

HandlerInfo &Stack::currentInfo()
{
	return stack.empty() ? EmptyHandlerInfo : stack.back();
}
HandlerInfo &Stack::lastInfo()
{
	return stack.size() < 2U ? EmptyHandlerInfo : stack[stack.size() - 2];
}

void Stack::endCurrentHandler()
{
	if (!stack.empty()) {
		// Fetch the handler info for the current top-level element
		HandlerInfo &info = stack.back();

		// Do not call any callback functions while the stack is marked as
		// invalid or this is an elment marked as "implicit"
		if (!info.implicit && handlersValid()) {
			// Make sure the fieldEnd handler is called if the element still
			// is in a field
			if (info.inField) {
				if (info.inValidField) {
					info.handler->fieldEnd();
				}
				info.fieldEnd();
			}

			// Call the "end" function of the corresponding Handler instance
			info.handler->end();
		}

		// Remove the element from the stack
		stack.pop_back();
	}
}

void Stack::endOverdueHandlers()
{
	if (!stack.empty()) {
		// Fetch the handler info for the current top-level element
		HandlerInfo &info = stack.back();

		// Abort if this handler currently is inside a field
		if (info.inField || (!info.hadDefaultField && info.valid)) {
			return;
		}

		// Otherwise end the current handler
		endCurrentHandler();
	}
}

bool Stack::ensureHandlerIsInField()
{
	// If the current handler is not in a field (and actually has a handler)
	// try to start a default field
	HandlerInfo &info = currentInfo();
	if (!info.inField && info.handler != nullptr) {
		// Abort if the element already had a default field or the handler is
		// not valid
		if (info.hadDefaultField || !info.valid) {
			return false;
		}

		// Try to start a new default field, abort if this did not work
		bool isDefault = true;
		if (!info.handler->fieldStart(isDefault, info.fieldIdx)) {
			return false;
		}

		// Mark the field as started
		info.fieldStart(true, true, true);
	}
	return true;
}

bool Stack::handlersValid()
{
	for (auto it = stack.crbegin(); it != stack.crend(); it++) {
		if (!it->valid) {
			return false;
		}
	}
	return true;
}

Logger &Stack::logger() { return ctx.getLogger(); }

void Stack::command(const Variant &name, const Variant::mapType &args)
{
	// End handlers that already had a default field and are currently not
	// active.
	endOverdueHandlers();

	// Make sure the given identifier is valid (preventing "*" from being
	// malicously passed to this function)
	if (!Utils::isNamespacedIdentifier(name.asString())) {
		throw LoggableException(std::string("Invalid identifier \"") +
		                            name.asString() + std::string("\""),
		                        name);
	}

	while (true) {
		// Try to find a target state for the given command, if none can be
		// found and the current command does not have an open field, then try
		// to create an empty default field, otherwise this is an exception
		const State *targetState = findTargetStateOrWildcard(name.asString());
		if (targetState == nullptr) {
			HandlerInfo &info = currentInfo();
			if (info.inImplicitDefaultField || !info.inField) {
				endCurrentHandler();
				continue;
			} else {
				throw buildInvalidCommandException(name.asString(),
				                                   expectedCommands());
			}
		}

		// Make sure we're currently inside a field
		if (!ensureHandlerIsInField()) {
			endCurrentHandler();
			continue;
		}

		// Fork the logger. We do not want any validation errors to skip
		LoggerFork loggerFork = logger().fork();

		// Instantiate the handler and push it onto the stack
		HandlerConstructor ctor = targetState->elementHandler
		                              ? targetState->elementHandler
		                              : EmptyHandler::create;
		std::shared_ptr<Handler> handler{
		    ctor({ctx, name.asString(), *targetState, name.getLocation()})};
		stack.emplace_back(handler);

		// Fetch the HandlerInfo for the parent element and the current element
		HandlerInfo &parentInfo = lastInfo();
		HandlerInfo &info = currentInfo();

		// Call the "start" method of the handler, store the result of the start
		// method as the validity of the handler -- do not call the start method
		// if the stack is currently invalid (as this may cause further,
		// unwanted errors)
		bool validStack = handlersValid();
		info.valid = false;
		if (validStack) {
			// Canonicalize the arguments (if this has not already been done),
			// allow additional arguments and numeric indices
			Variant::mapType canonicalArgs = args;
			targetState->arguments.validateMap(canonicalArgs, loggerFork, true,
			                                   true);

			handler->setLogger(loggerFork);
			try {
				info.valid = handler->start(canonicalArgs);
			}
			catch (LoggableException ex) {
				loggerFork.log(ex);
			}
			handler->resetLogger();
		}

		// We started the command within an implicit default field and it is not
		// valid -- remove both the new handler and the parent field from the
		// stack
		if (!info.valid && parentInfo.inImplicitDefaultField) {
			endCurrentHandler();
			endCurrentHandler();
			continue;
		}

		// If we ended up here, starting the command may or may not have worked,
		// but after all, we cannot unroll the stack any further. Update the
		// "valid" flag, commit any potential error messages and return.
		info.valid = parentInfo.valid && info.valid;
		loggerFork.commit();
		return;
	}
}

void Stack::data(const Variant &data)
{
	// End handlers that already had a default field and are currently not
	// active.
	endOverdueHandlers();

	while (true) {
		// Check whether there is any command the data can be sent to
		if (stack.empty()) {
			throw LoggableException("No command here to receive data.", data);
		}

		// Fetch the current command handler information
		HandlerInfo &info = currentInfo();

		// Make sure the current handler has an open field
		if (!ensureHandlerIsInField()) {
			endCurrentHandler();
			continue;
		}

		// If this field should not get any data, log an error and do not call
		// the "data" handler
		if (!info.inValidField) {
			// If the "hadDefaultField" flag is set, we already issued an error
			// message
			if (!info.hadDefaultField) {
				logger().error("Did not expect any data here", data);
			}
		}

		if (handlersValid() && info.inValidField) {
			// Fork the logger and set it as temporary logger for the "start"
			// method. We only want to keep error messages if this was not a try
			// to implicitly open a default field.
			LoggerFork loggerFork = logger().fork();
			info.handler->setLogger(loggerFork);

			// Pass the data to the current Handler instance
			bool valid = false;
			try {
				Variant dataCopy = data;
				valid = info.handler->data(dataCopy);
			}
			catch (LoggableException ex) {
				loggerFork.log(ex);
			}

			// Reset the logger instance as soon as possible
			info.handler->resetLogger();

			// If placing the data here failed and we're currently in an
			// implicitly opened field, just unroll the stack to the next field
			// and try again
			if (!valid && info.inImplicitDefaultField) {
				endCurrentHandler();
				continue;
			}

			// Commit the content of the logger fork. Do not change the valid
			// flag.
			loggerFork.commit();
		}

		// There was no reason to unroll the stack any further, so continue
		return;
	}
}

void Stack::fieldStart(bool isDefault)
{
	// Make sure the current handler stack is not empty
	if (stack.empty()) {
		throw LoggableException(
		    "No command for which a field could be started");
	}

	// Fetch the information attached to the current handler
	HandlerInfo &info = currentInfo();
	if (info.inField) {
		logger().error(
		    "Got field start, but there is no command for which to start the "
		    "field.");
		return;
	}

	// If the handler already had a default field we cannot start a new field
	// (the default field always is the last field) -- mark the command as
	// invalid
	if (info.hadDefaultField) {
		logger().error(std::string("Got field start, but command \"") +
		               currentCommandName() +
		               std::string("\" does not have any more fields"));
	}

	// Copy the isDefault flag to a local variable, the fieldStart method will
	// write into this variable
	bool defaultField = isDefault;

	// Do not call the "fieldStart" function if we're in an invalid subtree
	// or the handler already had a default field
	bool valid = false;
	if (handlersValid() && !info.hadDefaultField) {
		try {
			valid = info.handler->fieldStart(defaultField, info.fieldIdx);
		}
		catch (LoggableException ex) {
			logger().log(ex);
		}
		if (!valid && !defaultField) {
			logger().error(
			    std::string("Cannot start a new field here (index ") +
			    std::to_string(info.fieldIdx + 1) +
			    std::string("), field does not exist"));
		}
	}

	// Mark the field as started
	info.fieldStart(defaultField, false, valid);
}

void Stack::fieldEnd()
{
	// Unroll the stack until the next explicitly open field
	while (!stack.empty()) {
		HandlerInfo &info = currentInfo();
		if (info.inField && !info.inImplicitDefaultField) {
			break;
		}
		endCurrentHandler();
	}

	// Fetch the information attached to the current handler
	HandlerInfo &info = currentInfo();
	if (!info.inField || info.inImplicitDefaultField || stack.empty()) {
		logger().error(
		    "Got field end, but there is no command for which to end the "
		    "field.");
		return;
	}

	// Only continue if the current handler stack is in a valid state, do not
	// call the fieldEnd function if something went wrong before
	if (handlersValid() && !info.hadDefaultField && info.inValidField) {
		try {
			info.handler->fieldEnd();
		}
		catch (LoggableException ex) {
			logger().log(ex);
		}
	}

	// This command no longer is in a field
	info.fieldEnd();
}

void Stack::annotationStart(const Variant &className, const Variant &args)
{
	// TODO
}

void Stack::annotationEnd(const Variant &className, const Variant &elementName)
{
	// TODO
}

void Stack::token(Variant token)
{
	// TODO
}
}
}