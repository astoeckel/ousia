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
#include <core/parser/utils/TokenizedData.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

#include "Callbacks.hpp"
#include "Handler.hpp"
#include "Stack.hpp"
#include "State.hpp"
#include "TokenRegistry.hpp"
#include "TokenStack.hpp"

namespace ousia {
namespace parser_stack {
namespace {
/* Class HandlerInfo */

/**
 * The HandlerInfo class is used internally by the stack to associate additional
 * (mutable) data with a handler instance.
 */
class HandlerInfo {
public:
	/**
	 * Pointer pointing at the actual handler instance.
	 */
	std::shared_ptr<Handler> handler;

	/**
	 * Next field index to be passed to the "fieldStart" function of the Handler
	 * class.
	 */
	size_t fieldIdx;

	/**
	 * Set to true if the handler is valid (which is the case if the "start"
	 * method has returned true). If the handler is invalid, no more calls are
	 * directed at it until it can be removed from the stack.
	 */
	bool valid : 1;

	/**
	 * Set to true if this is an implicit handler, that was created when the
	 * current stack state was deduced.
	 */
	bool implicit : 1;

	/**
	 * Set to true if the handled command or annotation has a range.
	 */
	bool range : 1;

	/**
	 * Set to true if the handler currently is in a field.
	 */
	bool inField : 1;

	/**
	 * Set to true if the handler currently is in the default field.
	 */
	bool inDefaultField : 1;

	/**
	 * Set to true if the handler currently is in an implicitly started default
	 * field.
	 */
	bool inImplicitDefaultField : 1;

	/**
	 * Set to false if this field is only opened pro-forma and does not accept
	 * any data. Otherwise set to true.
	 */
	bool inValidField : 1;

	/**
	 * Set to true, if the default field was already started.
	 */
	bool hadDefaultField : 1;

	/**
	 * Default constructor of the HandlerInfo class.
	 */
	HandlerInfo();

	/**
	 * Constructor of the HandlerInfo class, allows to set all flags manually.
	 */
	HandlerInfo(bool valid, bool implicit, bool range, bool inField,
	            bool inDefaultField, bool inImplicitDefaultField,
	            bool inValidField);

	/**
	 * Constructor of the HandlerInfo class, taking a shared_ptr to the handler
	 * to which additional information should be attached.
	 */
	HandlerInfo(std::shared_ptr<Handler> handler);

	/**
	 * Destructor of the HandlerInfo class (to allow Handler to be forward
	 * declared).
	 */
	~HandlerInfo();

	/**
	 * Updates the "field" flags according to a "fieldStart" event.
	 */
	void fieldStart(bool isDefault, bool isImplicit, bool isValid);

	/**
	 * Updates the "fields" flags according to a "fieldEnd" event.
	 */
	void fieldEnd();

	/**
	 * Returns the name of the referenced handler or an empty string if no
	 * handler is present.
	 *
	 * @return the current handler name.
	 */
	std::string name() const;

	/**
	 * Returns the type of the referenced handler or COMMAND if no handler is
	 * present.
	 *
	 * @return the current handler type.
	 */
	HandlerType type() const;

	/**
	 * Returns the current state the handler is on or States::None if no handler
	 * is present.
	 *
	 * @return the current state machine state.
	 */
	const State &state() const;
};

HandlerInfo::HandlerInfo() : HandlerInfo(nullptr) {}

HandlerInfo::HandlerInfo(std::shared_ptr<Handler> handler)
    : handler(handler),
      fieldIdx(0),
      valid(true),
      implicit(false),
      range(false),
      inField(false),
      inDefaultField(false),
      inImplicitDefaultField(false),
      inValidField(false),
      hadDefaultField(false)
{
}

HandlerInfo::HandlerInfo(bool valid, bool implicit, bool range, bool inField,
                         bool inDefaultField, bool inImplicitDefaultField,
                         bool inValidField)
    : handler(nullptr),
      fieldIdx(0),
      valid(valid),
      implicit(implicit),
      range(range),
      inField(inField),
      inDefaultField(inDefaultField),
      inImplicitDefaultField(inImplicitDefaultField),
      inValidField(inValidField),
      hadDefaultField(false)
{
}

std::string HandlerInfo::name() const
{
	return handler == nullptr ? std::string{} : handler->name();
}

HandlerType HandlerInfo::type() const
{
	return handler == nullptr ? HandlerType::COMMAND : handler->type();
}

const State &HandlerInfo::state() const
{
	return handler == nullptr ? States::None : handler->state();
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
static HandlerInfo EmptyHandlerInfo{true, true, false, true, true, false, true};
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

/* Class StackImpl */

class StackImpl : public HandlerCallbacks {
private:
	/**
	 * Reference at an implementation of the ParserCallbacks instance to which
	 * certain handler callbacks are directed.
	 */
	ParserCallbacks &parser;

	/**
	 * Reference at the parser context.
	 */
	ParserContext &ctx;

	/**
	 * Map containing all registered command names and the corresponding
	 * state descriptors.
	 */
	const std::multimap<std::string, const State *> &states;

	/**
	 * Registry responsible for registering the tokens proposed by the
	 * Handlers in the parser.
	 */
	TokenRegistry tokenRegistry;

	/**
	 * Pointer at a TokenizedDataReader instance from which the data should
	 * currently be read.
	 */
	TokenizedDataReader *dataReader;

	/**
	 * Internal stack used for managing the currently active Handler instances.
	 */
	std::vector<HandlerInfo> stack;

	/**
	 * Return the reference in the Logger instance stored within the context.
	 */
	Logger &logger() { return ctx.getLogger(); }

	/**
	 * Used internally to get all expected command names for the current state.
	 * This function is used to build error messages.
	 *
	 * @return a set of strings containing the names of the expected commands.
	 */
	std::set<std::string> expectedCommands();

	/**
	 * Returns the targetState for a command with the given name that can be
	 * reached from the current state.
	 *
	 * @param name is the name of the requested command.
	 * @return nullptr if no target state was found, a pointer at the target
	 * state otherwise.
	 */
	const State *findTargetState(const std::string &name);

	/**
	 * Returns the targetState for a command with the given name that can be
	 * reached from the current state, also including the wildcard "*" state.
	 * Throws an exception if the given target state is not a valid identifier.
	 *
	 * @param name is the name of the requested command.
	 * @return nullptr if no target state was found, a pointer at the target
	 * state otherwise.
	 */
	const State *findTargetStateOrWildcard(const std::string &name);

	/**
	 * Tries to reconstruct the parser state from the Scope instance of the
	 * ParserContext given in the constructor. This functionality is needed for
	 * including files,as the Parser of the included file needs to be brought to
	 * an equivalent state as the one in the including file.
	 */
	void deduceState();

	/**
	 * Returns a reference at the current HandlerInfo instance (or a stub
	 * HandlerInfo instance if the stack is empty).
	 */
	HandlerInfo &currentInfo();

	/**
	 * Returns a reference at the last HandlerInfo instance (or a stub
	 * HandlerInfo instance if the stack has only one element).
	 */
	HandlerInfo &lastInfo();

	/**
	 * Ends all handlers that currently are not inside a field and already had
	 * a default field. This method is called whenever the data() and command()
	 * events are reached.
	 */
	void endOverdueHandlers();

	/**
	 * Ends the current handler and removes the corresponding element from the
	 * stack.
	 */
	void endCurrentHandler();

	/**
	 * Tries to start a default field for the current handler, if currently the
	 * handler is not inside a field and did not have a default field yet.
	 *
	 * @return true if the handler is inside a field, false if no field could
	 * be started.
	 */
	bool ensureHandlerIsInField();

	/**
	 * Returns true if all handlers on the stack are currently valid, or false
	 * if at least one handler is invalid.
	 *
	 * @return true if all handlers on the stack are valid.
	 */
	bool handlersValid();

public:
	StackImpl(ParserCallbacks &parser, ParserContext &ctx,
	          const std::multimap<std::string, const State *> &states);

	~StackImpl();

	const State &currentState() const;
	std::string currentCommandName() const;

	void commandStart(const Variant &name, const Variant::mapType &args,
	                  bool range);
	void annotationStart(const Variant &className, const Variant &args,
	                     bool range);
	void annotationEnd(const Variant &className, const Variant &elementName);
	void rangeEnd();
	void fieldStart(bool isDefault);
	void fieldEnd();
	void data(const TokenizedData &data);

	TokenId registerToken(const std::string &token) override;
	void unregisterToken(TokenId id) override;
	Variant readData() override;
	bool hasData();
	void pushTokens(const std::vector<SyntaxDescriptor> &tokens) override;
	void popTokens() override;
};

StackImpl::StackImpl(ParserCallbacks &parser, ParserContext &ctx,
                     const std::multimap<std::string, const State *> &states)
    : parser(parser),
      ctx(ctx),
      states(states),
      tokenRegistry(parser),
      dataReader(nullptr)
{
	// If the scope instance is not empty we need to deduce the current parser
	// state
	if (!ctx.getScope().isEmpty()) {
		deduceState();
	}
}

StackImpl::~StackImpl()
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
				        currentCommandName() +
				        "\" has not ended yet. Command was started here:",
				    info.handler->getLocation());
			}
		}

		// Remove the command from the stack
		endCurrentHandler();
	}
}

void StackImpl::deduceState()
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

	std::shared_ptr<Handler> handler = std::shared_ptr<Handler>{
	    ctor({ctx, *this, state, SourceLocation{}, HandlerType::COMMAND})};
	stack.emplace_back(handler);

	// Set the correct flags for this implicit handler
	HandlerInfo &info = currentInfo();
	info.implicit = true;
	info.fieldStart(true, false, true);
}

std::set<std::string> StackImpl::expectedCommands()
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

const State &StackImpl::currentState() const
{
	return stack.empty() ? States::None : stack.back().state();
}

std::string StackImpl::currentCommandName() const
{
	return stack.empty() ? std::string{} : stack.back().name();
}

const State *StackImpl::findTargetState(const std::string &name)
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

const State *StackImpl::findTargetStateOrWildcard(const std::string &name)
{
	// Try to find the target state with the given name, if none is found, try
	// find a matching "*" state.
	State const *targetState = findTargetState(name);
	if (targetState == nullptr) {
		return findTargetState("*");
	}
	return targetState;
}

HandlerInfo &StackImpl::currentInfo()
{
	return stack.empty() ? EmptyHandlerInfo : stack.back();
}
HandlerInfo &StackImpl::lastInfo()
{
	return stack.size() < 2U ? EmptyHandlerInfo : stack[stack.size() - 2];
}

void StackImpl::endCurrentHandler()
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

void StackImpl::endOverdueHandlers()
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

bool StackImpl::ensureHandlerIsInField()
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

bool StackImpl::handlersValid()
{
	for (auto it = stack.crbegin(); it != stack.crend(); it++) {
		if (!it->valid) {
			return false;
		}
	}
	return true;
}

void StackImpl::commandStart(const Variant &name, const Variant::mapType &args,
                             bool range)
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
		    ctor({ctx,
		          *this,
		          *targetState,
		          {name.asString(), name.getLocation()},
		          HandlerType::COMMAND})};
		stack.emplace_back(handler);

		// Fetch the HandlerInfo for the parent element and the current
		// element
		HandlerInfo &parentInfo = lastInfo();
		HandlerInfo &info = currentInfo();

		// Call the "start" method of the handler, store the result of the
		// start
		// method as the validity of the handler -- do not call the start
		// method
		// if the stack is currently invalid (as this may cause further,
		// unwanted errors)
		bool validStack = handlersValid();
		info.valid = false;
		if (validStack) {
			// Canonicalize the arguments (if this has not already been
			// done),
			// allow additional arguments and numeric indices
			Variant::mapType canonicalArgs = args;
			targetState->arguments.validateMap(canonicalArgs, loggerFork, true,
			                                   true);

			handler->setLogger(loggerFork);
			try {
				info.valid = handler->startCommand(canonicalArgs);
			}
			catch (LoggableException ex) {
				loggerFork.log(ex);
			}
			handler->resetLogger();
		}

		// We started the command within an implicit default field and it is
		// not
		// valid -- remove both the new handler and the parent field from
		// the
		// stack
		if (!info.valid && parentInfo.inImplicitDefaultField) {
			endCurrentHandler();
			endCurrentHandler();
			continue;
		}

		// If we ended up here, starting the command may or may not have
		// worked,
		// but after all, we cannot unroll the stack any further. Update the
		// "valid" flag, commit any potential error messages and return.
		info.valid = parentInfo.valid && info.valid;
		info.range = range;
		loggerFork.commit();
		return;
	}
}

void StackImpl::annotationStart(const Variant &className, const Variant &args,
                                bool range)
{
	// TODO
}

void StackImpl::annotationEnd(const Variant &className,
                              const Variant &elementName)
{
	// TODO
}

void StackImpl::rangeEnd()
{
	// TODO
}

void StackImpl::data(const TokenizedData &data)
{
	// TODO: Rewrite this function for token handling
	// TODO: This loop needs to be refactored out
	/*while (!data.atEnd()) {
	    // End handlers that already had a default field and are currently
	not
	    // active.
	    endOverdueHandlers();

	    const bool hasNonWhitespaceText = data.hasNonWhitespaceText();

	    // Check whether there is any command the data can be sent to -- if
	not,
	    // make sure the data actually is data
	    if (stack.empty()) {
	        if (hasNonWhitespaceText) {
	            throw LoggableException("No command here to receive data.",
	                                    data);
	        }
	        return;
	    }

	    // Fetch the current command handler information
	    HandlerInfo &info = currentInfo();

	    // Make sure the current handler has an open field
	    if (!ensureHandlerIsInField()) {
	        endCurrentHandler();
	        continue;
	    }

	    // If this field should not get any data, log an error and do not
	call
	    // the "data" handler
	    if (!info.inValidField) {
	        // If the "hadDefaultField" flag is set, we already issued an
	error
	        // message
	        if (!info.hadDefaultField) {
	            if (hasNonWhitespaceText) {
	                logger().error("Did not expect any data here", data);
	            }
	            return;
	        }
	    }

	    if (handlersValid() && info.inValidField) {
	        // Fork the logger and set it as temporary logger for the
	"start"
	        // method. We only want to keep error messages if this was not a
	try
	        // to implicitly open a default field.
	        LoggerFork loggerFork = logger().fork();
	        info.handler->setLogger(loggerFork);

	        // Pass the data to the current Handler instance
	        bool valid = false;
	        try {
	            // Create a fork of the TokenizedData and let the handler
	work
	            // on it
	            TokenizedData dataFork = data;
	            valid = info.handler->data(dataFork);

	            // If the data was validly handled by the handler, commit
	the
	            // change
	            if (valid) {
	                data = dataFork;
	            }
	        }
	        catch (LoggableException ex) {
	            loggerFork.log(ex);
	        }

	        // Reset the logger instance as soon as possible
	        info.handler->resetLogger();

	        // If placing the data here failed and we're currently in an
	        // implicitly opened field, just unroll the stack to the next
	field
	        // and try again
	        if (!valid && info.inImplicitDefaultField) {
	            endCurrentHandler();
	            continue;
	        }

	        // Commit the content of the logger fork. Do not change the
	valid
	        // flag.
	        loggerFork.commit();
	    }

	    // There was no reason to unroll the stack any further, so continue
	    return;
	}*/
}

void StackImpl::fieldStart(bool isDefault)
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
		    "Got field start, but there is no command for which to start "
		    "the "
		    "field.");
		return;
	}

	// If the handler already had a default field we cannot start a new
	// field
	// (the default field always is the last field) -- mark the command as
	// invalid
	if (info.hadDefaultField) {
		logger().error(std::string("Got field start, but command \"") +
		               currentCommandName() +
		               std::string("\" does not have any more fields"));
	}

	// Copy the isDefault flag to a local variable, the fieldStart method
	// will
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

void StackImpl::fieldEnd()
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

	// Only continue if the current handler stack is in a valid state, do
	// not
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

TokenId StackImpl::registerToken(const std::string &token)
{
	return tokenRegistry.registerToken(token);
}

void StackImpl::unregisterToken(TokenId id)
{
	tokenRegistry.unregisterToken(id);
}

void StackImpl::pushTokens(const std::vector<SyntaxDescriptor> &tokens)
{
	// TODO
}

void StackImpl::popTokens()
{
	// TODO
}

Variant StackImpl::readData()
{
	if (dataReader != nullptr) {
		TokenizedDataReaderFork dataReaderFork = dataReader->fork();
		Token token;

		// TODO: Use correct token set
		TokenSet tokens;

		// TODO: Use correct whitespace mode
		WhitespaceMode mode = WhitespaceMode::COLLAPSE;

		dataReaderFork.read(token, tokens, mode);
		if (token.id == Tokens::Data) {
			Variant res = Variant::fromString(token.content);
			res.setLocation(token.getLocation());
			return res;
		}
	}
	return Variant{};
}

bool StackImpl::hasData() { return readData() != nullptr; }

/* Class Stack */

Stack::Stack(ParserCallbacks &parser, ParserContext &ctx,
             const std::multimap<std::string, const State *> &states)
    : impl(new StackImpl(parser, ctx, states))
{
}

Stack::~Stack()
{
	// Do nothing here, stub needed because StackImpl is incomplete in hpp
}

const State &Stack::currentState() const { return impl->currentState(); }

std::string Stack::currentCommandName() const
{
	return impl->currentCommandName();
}

void Stack::commandStart(const Variant &name, const Variant::mapType &args,
                         bool range)
{
	impl->commandStart(name, args, range);
}

void Stack::annotationStart(const Variant &className, const Variant &args,
                            bool range)
{
	impl->annotationStart(className, args, range);
}

void Stack::annotationEnd(const Variant &className, const Variant &elementName)
{
	impl->annotationEnd(className, elementName);
}

void Stack::rangeEnd() { impl->rangeEnd(); }

void Stack::fieldStart(bool isDefault) { impl->fieldStart(isDefault); }

void Stack::fieldEnd() { impl->fieldEnd(); }

void Stack::data(const TokenizedData &data) { impl->data(data); }
}
}
