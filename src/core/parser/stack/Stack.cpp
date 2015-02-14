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

#include <sstream>

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
      inField(false),
      inDefaultField(false),
      inImplicitDefaultField(false),
      hasDefaultField(false)
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
	hasDefaultField = hasDefaultField || inDefaultField;
	fieldIdx++;
}

void HandlerInfo::fieldEnd()
{
	inField = false;
	inDefaultField = false;
	inImplicitDefaultField = false;
	inValidField = false;
	if (fieldIdx > 0) {
		fieldIdx--;
	}
}

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

Stack::~Stack() {}

bool Stack::deduceState()
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
		throw LoggableException{
		    "Error while including file: Cannot deduce parser state."};
	}

	// Switch to this state by creating a dummy handler
	const State *state = possibleStates[0];
	stack.emplace(std::shared_ptr<Handler>{EmptyHandler::create({ctx, "", *state, *state, SourceLocation{}})});
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
	return stack.empty() ? States::None : stack.top()->state();
}

std::string Stack::currentCommandName()
{
	return stack.empty() ? std::string{} : stack.top()->name();
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

void Stack::command(const Variant &name, const Variant::mapType &args)
{
	// Make sure the given identifier is valid
	if (!Utils::isNamespacedIdentifier(name.asString())) {
		throw LoggableException(std::string("Invalid identifier \"") +
		                        name.asString() + std::string("\""), name);
	}

	// Try to find a target state for the given command
	State const *targetState = findTargetState(name.asString());

	// No target state is found, try to find a wildcard handler for the current
	// state
	if (targetState == nullptr) {
		targetState = findTargetState("*");
	}

	// No handler has been found at all,
	if (targetState == nullptr) {
		throw buildInvalidCommandException(name.asString(), expectedCommands());
	}

	// Fetch the associated constructor
	HandlerConstructor ctor = targetState->elementHandler
	                              ? targetState->elementHandler
	                              : DefaultHandler::create;

	// Canonicalize the arguments, allow additional arguments
	targetState->arguments.validateMap(args, ctx.getLogger(), true);

	// Instantiate the handler and push it onto the stack
	Handler *handler =
	    ctor({ctx, name.asString(), *targetState, currentState(), name.getLocation()});
	stack.emplace_back(std::shared_ptr<Handler>{handler});

	// Call the "start" method of the handler, store the result of the start
	// method as the validity of the handler -- do not call the start method
	// if the stack is currently invalid (as this may cause further, unwanted
	// errors)
	try {
		stack.back().valid = handlersValid() && handler->start(args);
	} catch (LoggableException ex) {
		stack.back().valid = false;
		logger.log(ex, )
	}
}

void Stack::end()
{
	// Check whether the current command could be ended
	if (stack.empty()) {
		throw LoggableException{"No command to end."};
	}

	// Remove the current HandlerInstance from the stack
	std::shared_ptr<Handler> inst{stack.top()};
	stack.pop();

	// Call the end function of the last Handler
	inst->end();
}

void Stack::data(const std::string &data, int field)
{
	// Check whether there is any command the data can be sent to
	if (stack.empty()) {
		throw LoggableException{"No command to receive data."};
	}

	// Pass the data to the current Handler instance
	stack.top()->data(data, field);
}
}
}

