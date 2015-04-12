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

#include <limits>

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

#define STACK_DEBUG_OUTPUT 0
#if STACK_DEBUG_OUTPUT
#include <iostream>
#endif

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
	 * TokenId of the close token.
	 */
	TokenId closeToken;

	/**
	 * Descriptor corresponding to the associated "close" token, set to nullptr
	 * if not needed.
	 */
	Rooted<Node> tokenDesciptor;

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
	 * Set to true once data was passed to the handler.
	 */
	bool hadData : 1;

	/**
	 * Set to false, if the handler is not greedy (true is the default value).
	 * If false, the handler will only be passed one piece of "data" at most.
	 */
	bool greedy : 1;

	/**
	 * Default constructor of the HandlerInfo class.
	 */
	HandlerInfo();

	/**
	 * Constructor of the HandlerInfo class, allows to set some flags manually.
	 */
	HandlerInfo(bool implicit, bool inField, bool inDefaultField,
	            bool inImplicitDefaultField);

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
      closeToken(Tokens::Empty),
      valid(true),
      implicit(false),
      range(false),
      inField(false),
      inDefaultField(false),
      inImplicitDefaultField(false),
      inValidField(false),
      hadDefaultField(false),
      hadData(false),
      greedy(true)
{
}

HandlerInfo::HandlerInfo(bool implicit, bool inField, bool inDefaultField,
                         bool inImplicitDefaultField)
    : handler(nullptr),
      fieldIdx(0),
      closeToken(Tokens::Empty),
      valid(true),
      implicit(implicit),
      range(false),
      inField(inField),
      inDefaultField(inDefaultField),
      inImplicitDefaultField(inImplicitDefaultField),
      inValidField(true),
      hadDefaultField(false),
      hadData(false),
      greedy(true)
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
static HandlerInfo EmptyHandlerInfo{true, true, true, true};

/**
 * Small helper class makeing sure the reference at some variable is reset once
 * the scope is left.
 */
template <class T>
struct GuardedTemporaryPointer {
	T **ptr;
	GuardedTemporaryPointer(T *ref, T **ptr) : ptr(ptr) { *ptr = ref; }

	~GuardedTemporaryPointer() { *ptr = nullptr; }
};
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
	 * Collection of all currently enabled tokens.
	 */
	TokenStack tokenStack;

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
	 * Returns a reference at the current HandlerInfo instance (or a stub
	 * HandlerInfo instance if the stack is empty).
	 */
	const HandlerInfo &currentInfo() const;

	/**
	 * Returns a reference at the last HandlerInfo instance (or a stub
	 * HandlerInfo instance if the stack has only one element).
	 */
	HandlerInfo &lastInfo();

	/**
	 * Returns the maximum stack depth that can be unrolled.
	 *
	 * @return the number of elements that can currently be removed from the
	 * stack.
	 */
	size_t maxUnrollStackDepth() const;

	/**
	 * Returns the index of the next reachable handler on the stack having the
	 * given token as close token -- or in case Tokens::Empty is given, the
	 * index of the next reachable handler with a set close token.
	 *
	 * @param token is the TokenId of the close token that is being searched. If
	 * Tokens::Empty is passed, the first handler with a non-empty close token
	 * will be returned.
	 * @return the index of the corresponding handler on the stack or a negative
	 * value if no such handler could be found.
	 */
	ssize_t pendingCloseTokenHandlerIdx(TokenId token = Tokens::Empty) const;

	/**
	 * Returns a set containing the tokens that should currently be processed
	 * by the TokenizedData instance.
	 *
	 * @return a TokenSet instance containing all tokens that should currently
	 * be processed.
	 */
	TokenSet currentTokens() const;

	/**
	 * Returns the whitespace mode defined by the current command.
	 */
	WhitespaceMode currentWhitespaceMode() const;

	/**
	 * Ends a currently open field of the current handler.
	 */
	void endCurrentField();

	/**
	 * Ends the current handler and removes the corresponding element from the
	 * stack.
	 *
	 * @return true if a command was ended, false otherwise.
	 */
	bool endCurrentHandler();

	/**
	 * Ends all handlers that currently are not inside a field and already had
	 * a default field. Tries to start a default field for the current handler,
	 * if currently the handler is not inside a field and did not have a default
	 * field yet. This method is called whenever the data(), startAnnotation(),
	 * startToken(), startCommand(), annotationStart() or annotationEnd() events
	 * are reached.
	 *
	 * @param startImplicitDefaultField if set to true, starts a new default
	 * field.
	 * @param endHandlersWithoutDefaultField important if
	 * startImplicitDefaultField is set to false. If false, prevents this method
	 * from ending a handler if it potentially can have a default field, but did
	 * not have one yet.
	 * @param startImplicitDefaultFieldForNonGreedy is set to true, even starts
	 * an implicit default field for greedy handlers. Otherwise these handlers
	 * are ended.
	 * @return true if the current command is in a valid field.
	 */
	bool prepareCurrentHandler(bool startImplicitDefaultField = true,
	                           bool endHandlersWithoutDefaultField = true,
	                           bool endNonGreedyHandlers = true);

	/**
	 * Returns true if all handlers on the stack are currently valid, or false
	 * if at least one handler is invalid.
	 *
	 * @return true if all handlers on the stack are valid.
	 */
	bool handlersValid();

	/**
	 * Called whenever there is an actual data pending on the current
	 * TokenizedDataReader. Tries to feed this data to the current handler.
	 */
	bool handleData();

	/**
	 * Called whenever the annotationStart or annotationEnd methods are called.
	 *
	 * @param name is the class name of the annotation.
	 * @param args contains the arguments that are passed to the annotation.
	 * @param range is set to true if this is a ranged annotation.
	 * @param type specifies whether this is a start or end annotation.
	 */
	void handleAnnotationStartEnd(const Variant &name, Variant::mapType args,
	                              bool range, HandlerType type);

	/**
	 * Called by handleToken in order to process a list of potential "close"
	 * tokens.
	 *
	 * @param token is the matching token.
	 * @param descrs is the list with SyntaxDescriptors that should be used.
	 * @return true if something was closed, false otherwise.
	 */
	bool handleCloseTokens(const Token &token,
	                       const std::vector<SyntaxDescriptor> &descrs);

	/**
	 * Called by handleToken in order to process a list of potential "open"
	 * tokens.
	 *
	 * @param logger responsible for receiving error messages.
	 * @param token is the matching token.
	 * @param shortForm is set to true, if the descriptors describe a short form
	 * token instead of an open token.
	 * @param descrs is the list with SyntaxDescriptors that should be used.
	 * @return true if something was opened, false otherwise.
	 */
	bool handleOpenTokens(Logger &logger, const Token &token, bool shortForm,
	                      const std::vector<SyntaxDescriptor> &descrs);

	/**
	 * Called whenever there is a token waiting to be processed. If possible
	 * tries to end a current handler with this token or to start a new handler
	 * with the token.
	 *
	 * @param token is the token that should be handled.
	 */
	void handleToken(const Token &token);

	/**
	 * Called by the rangeEnd() and fieldEnd() methods to end the current ranged
	 * command.
	 *
	 * @param endRange specifies whether this should end the range of a
	 * command with range.
	 */
	void handleFieldEnd(bool endRange);

public:
	StackImpl(ParserCallbacks &parser, ParserContext &ctx,
	          const std::multimap<std::string, const State *> &states);

	~StackImpl();

	const State &currentState() const;
	std::string currentCommandName() const;

	void commandStart(const Variant &name, const Variant::mapType &args,
	                  bool range);
	void annotationStart(const Variant &className, const Variant::mapType &args,
	                     bool range);
	void annotationEnd(const Variant &className, const Variant::mapType &args);
	void rangeEnd();
	void fieldStart(bool isDefault);
	void fieldEnd();
	void data(const TokenizedData &data);

	TokenId registerToken(const std::string &token) override;
	void unregisterToken(TokenId id) override;
	bool readToken(Token &token);
	Variant readData() override;
	void pushTokens(const std::vector<SyntaxDescriptor> &tokens) override;
	void popTokens() override;
};

StackImpl::StackImpl(ParserCallbacks &parser, ParserContext &ctx,
                     const std::multimap<std::string, const State *> &states)
    : ctx(ctx), states(states), tokenRegistry(parser), dataReader(nullptr)
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

const State &StackImpl::currentState() const
{
	return stack.empty() ? States::None : stack.back().state();
}

std::string StackImpl::currentCommandName() const
{
	return stack.empty() ? std::string{} : stack.back().name();
}

size_t StackImpl::maxUnrollStackDepth() const
{
	size_t res = 0;
	for (ssize_t i = stack.size() - 1; i >= 0; i--) {
		const HandlerInfo &info = stack[i];
		if (info.range || (info.inField && !info.inImplicitDefaultField)) {
			break;
		}
		res++;
	}
	return res;
}

ssize_t StackImpl::pendingCloseTokenHandlerIdx(TokenId token) const
{
	const ssize_t minIdx =
	    std::max<ssize_t>(0, stack.size() - maxUnrollStackDepth() - 1);
	for (ssize_t i = stack.size() - 1; i >= minIdx; i--) {
		const HandlerInfo &info = stack[i];
		if (info.closeToken != Tokens::Empty &&
		    (token == Tokens::Empty || token == info.closeToken)) {
			return i;
		}
	}
	return -1;
}

TokenSet StackImpl::currentTokens() const
{
	TokenSet res;
	if (currentInfo().state().supportsTokens) {
		res = tokenStack.tokens();
		ssize_t idx = pendingCloseTokenHandlerIdx();
		if (idx >= 0) {
			res.insert(stack[idx].closeToken);
		}
	}
	return res;
}

WhitespaceMode StackImpl::currentWhitespaceMode() const
{
	// TODO: Implement
	return WhitespaceMode::COLLAPSE;
}

HandlerInfo &StackImpl::currentInfo()
{
	return stack.empty() ? EmptyHandlerInfo : stack.back();
}

const HandlerInfo &StackImpl::currentInfo() const
{
	return stack.empty() ? EmptyHandlerInfo : stack.back();
}

HandlerInfo &StackImpl::lastInfo()
{
	return stack.size() < 2U ? EmptyHandlerInfo : stack[stack.size() - 2];
}

/* Stack helper functions */

void StackImpl::endCurrentField()
{
	if (!stack.empty()) {
		HandlerInfo &info = stack.back();
		if (!info.implicit && handlersValid() && info.inField) {
			if (info.inValidField) {
				info.handler->fieldEnd();
			}
			info.fieldEnd();
		}
	}
}

bool StackImpl::endCurrentHandler()
{
	if (!stack.empty()) {
		// Fetch the handler info for the current top-level element
		HandlerInfo &info = stack.back();

		// Do not call any callback functions while the stack is marked as
		// invalid or this is an elment marked as "implicit"
		if (!info.implicit && handlersValid()) {
			// End all currently open fields
			endCurrentField();

			// Call the "end" function of the corresponding Handler instance
			info.handler->end();
		}

		// Remove the element from the stack
		stack.pop_back();
		return true;
	}
	return false;
}

bool StackImpl::prepareCurrentHandler(bool startImplicitDefaultField,
                                      bool endHandlersWithoutDefaultField,
                                      bool endNonGreedyHandlers)
{
	// Repeat until a valid handler is found on the stack
	while (!stack.empty()) {
		// Fetch the handler for the current top-level element
		HandlerInfo &info = currentInfo();

		// If the current Handler is in a field, there is nothing to be done,
		// abort. Exception: If the handler is not greedy and currently is in
		// its default field, then continue.
		if (info.inField) {
			if (!info.greedy && info.hadData && info.inImplicitDefaultField) {
				endCurrentField();
				continue;
			} else {
				return true;
			}
		}

		// If the current field already had a default field or is not valid,
		// end it and repeat
		bool canHaveImplicitDefaultField =
		    info.type() == HandlerType::COMMAND ||
		    info.type() == HandlerType::TOKEN ||
		    (info.type() == HandlerType::ANNOTATION_START && info.range);
		if (info.hadDefaultField || (!info.greedy && endNonGreedyHandlers) ||
		    (!startImplicitDefaultField && endHandlersWithoutDefaultField) ||
		    !info.valid || !canHaveImplicitDefaultField) {
			// We cannot end the command if it is marked as "range" command
			if (info.range) {
				return false;
			}

			// End the current handler
			endCurrentHandler();
			continue;
		}

		// Try to start a new default field, abort if this did not work
		if (startImplicitDefaultField) {
			bool isDefault = true;
			if (!info.handler->fieldStart(isDefault, !info.range,
			                              info.fieldIdx)) {
				endCurrentHandler();
				continue;
			}

			// Mark the field as started and return
			info.fieldStart(true, !info.range, true);
			return true;
		}
		return false;
	}
	return false;
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

bool StackImpl::handleData()
{
	// Repeat until we found some handle willingly consuming the data
	while (true) {
		// Prepare the stack -- make sure all overdue handlers are ended and
		// we currently are in an open field
		if (stack.empty() || !prepareCurrentHandler(true, true, false)) {
			throw LoggableException("Did not expect any data here");
		}

		// Fetch the current handler information
		HandlerInfo &info = currentInfo();

		// If this field should not get any data, log an error and do not
		// call the "data" handler
		if (!info.inValidField) {
			if (!info.hadDefaultField) {
				logger().error("Did not expect any data here");
			}
			return true;
		}

		// If we're currently in an invalid subtree, just eat the data and abort
		if (!handlersValid()) {
			return true;
		}

		// Check whether "readData" still returns data and not an empty string
		// (because a token has now become valid)
		if (!readData().isString()) {
			return false;
		}

		// Fork the logger and set it as temporary logger for the "data"
		// method. We only want to keep error messages if this was not a
		// try to implicitly open a default field.
		LoggerFork loggerFork = logger().fork();
		info.handler->setLogger(loggerFork);

		// Pass the data to the current Handler instance
		bool valid = false;
		try {
			valid = info.handler->data();
		}
		catch (LoggableException ex) {
			loggerFork.log(ex);
		}

		// Update the "hadData" flag
		info.hadData = info.hadData || valid;

		// Reset the logger instance of the handler as soon as possible
		info.handler->resetLogger();

		// If placing the data here failed and we're currently in an
		// implicitly opened field, just unroll the stack to the next field
		// and try again
		if (!valid && info.inImplicitDefaultField) {
			endCurrentHandler();
			continue;
		}

		// Commit the content of the logger fork. Do not change the valid flag.
		loggerFork.commit();
		return true;
	}
}

/* Token management */

static void logTokenNote(const std::string &preamble,
                         const SyntaxDescriptor &descr, Logger &logger)
{
	std::string type = "";
	if (descr.isAnnotation()) {
		type = " annotation";
	} else if (descr.isFieldDescriptor()) {
		type = " field";
	} else if (descr.isStruct()) {
		type = " structure";
	}
	logger.note(preamble + " \"" + descr.descriptor->getName() + "\"" + type +
	                ", specified here",
	            *(descr.descriptor));
}

static void logTokenNote(const std::string &preamble,
                         const std::vector<SyntaxDescriptor> &descrs,
                         Logger &logger)
{
	for (const auto &descr : descrs) {
		logTokenNote(preamble, descr, logger);
	}
}

static void strayTokenError(const Token &token, TokenDescriptor &descr,
                            Logger &logger)
{
	logger.error("Stray \"" + token.name() + "\" token", token);
	logger.note("This token must be used in one of the following contexts:",
	            SourceLocation{}, MessageMode::NO_CONTEXT);
	logTokenNote("To close a", descr.close, logger);
	logTokenNote("To open a", descr.open, logger);
	logTokenNote("As a short form of", descr.shortForm, logger);

	return;
}

static void checkTokensAreUnambiguous(const Token &token,
                                      const TokenDescriptor &descr,
                                      Logger &logger)
{
	// Some helper functions and constants
	constexpr ssize_t MAX_DEPTH = std::numeric_limits<ssize_t>::max();
	static const SyntaxDescriptor EMPTY_DESCR(
	    Tokens::Empty, Tokens::Empty, Tokens::Empty, nullptr, MAX_DEPTH, true);
	static auto get = [](size_t i, const std::vector<SyntaxDescriptor> &descrs)
	    -> const SyntaxDescriptor &
	{
		return (i < descrs.size()) ? descrs[i] : EMPTY_DESCR;
	};

	// Check whether there is any ambiguity -- e.g. there are two tokens with
	// the same depth (the effort they need to be created). The shortForm and
	// open lists are assumed to be sorted by depth.
	ssize_t errorDepth = MAX_DEPTH;
	size_t i = 0;
	size_t j = 0;
	while (errorDepth == MAX_DEPTH &&
	       (i < descr.open.size() || j < descr.shortForm.size())) {
		const SyntaxDescriptor &di1 = get(i, descr.open);
		const SyntaxDescriptor &di2 = get(i + 1, descr.open);
		const SyntaxDescriptor &dj1 = get(j, descr.shortForm);
		const SyntaxDescriptor &dj2 = get(j + 1, descr.shortForm);

		if (di1.depth != MAX_DEPTH &&
		    (di1.depth == di2.depth || di1.depth == dj1.depth ||
		     di1.depth == dj2.depth)) {
			errorDepth = di1.depth;
		}
		if (dj1.depth != MAX_DEPTH &&
		    (dj1.depth == dj2.depth || di2.depth == dj1.depth)) {
			errorDepth = dj1.depth;
		}

		i = i + ((di1.depth <= dj1.depth) ? 1 : 0);
		j = j + ((di1.depth >= dj1.depth) ? 1 : 0);
	}

	// Issue an error message if an ambiguity exists
	if (errorDepth != MAX_DEPTH) {
		logger.error("Token \"" + token.name() + "\" is ambiguous!");
		logger.note(
		    "The token could be ambiguously used in one of the following "
		    "contexts: ",
		    SourceLocation{}, MessageMode::NO_CONTEXT);
		for (size_t i = 0;
		     i < descr.open.size() && descr.open[i].depth <= errorDepth; i++) {
			if (descr.open[i].depth == errorDepth) {
				logTokenNote("To start a", descr.open[i], logger);
				break;
			}
		}
		for (size_t i = 0; i < descr.shortForm.size() &&
		                       descr.shortForm[i].depth <= errorDepth;
		     i++) {
			if (descr.shortForm[i].depth == errorDepth) {
				logTokenNote("As a short form of a", descr.shortForm[i],
				             logger);
				break;
			}
		}
	}
}

bool StackImpl::handleCloseTokens(const Token &token,
                                  const std::vector<SyntaxDescriptor> &descrs)
{
	// Abort if the stack is empty -- nothing can be ended in that case
	if (stack.empty()) {
		return false;
	}

	// Check whether any of the given token descriptors can be ended -- select
	// the one that needs the fewest unrolling
	const size_t maxStackDepth = maxUnrollStackDepth();
	const HandlerInfo &info = currentInfo();
	size_t idx = 0;
	EndTokenResult bestRes = EndTokenResult();
	for (size_t i = 0; i < descrs.size(); i++) {
		// Try to end the handler
		const EndTokenResult res =
		    info.handler->endToken(descrs[i].descriptor, maxStackDepth);

		// Abort if the "endToken" function ended a transparent field -- in this
		// case this method has already been successful
		if (res.depth == 0 && res.found) {
			return true;
		}

		// Otherwise check whether the result is positive and smaller than any
		// previous result
		if (res.found && (!bestRes.found || res.depth < bestRes.depth)) {
			idx = i;
			bestRes = res;
		}
	}

	// Abort if no descriptor can be ended
	if (!bestRes.found) {
		return false;
	}

	// End as many handlers as indicated by the "depth" counter, repeat the
	// process if needed
	for (size_t i = 0; i < bestRes.depth; i++) {
		endCurrentHandler();
	}
	if (!stack.empty() && bestRes.repeat) {
		currentInfo().handler->endToken(descrs[idx].descriptor, 0);
	}
	return true;
}

bool StackImpl::handleOpenTokens(Logger &logger, const Token &token,
                                 bool shortForm,
                                 const std::vector<SyntaxDescriptor> &descrs)
{
	// Make sure we currently are in a field
	if (!currentInfo().inField) {
		throw LoggableException("Cannot start a command here", token);
	}

	// Iterate over all given descriptors
	for (const SyntaxDescriptor &descr : descrs) {
		// Find the special target state TODO: Is there some better solution?
		const State *state = findTargetState("*");
		if (state == nullptr) {
			throw LoggableException("Cannot handle start tokens here", token);
		}

		// Instantiate the handler and push it onto the stack
		HandlerConstructor ctor = state->elementHandler ? state->elementHandler
		                                                : EmptyHandler::create;
		std::shared_ptr<Handler> handler{
		    ctor({ctx, *this, *state, token, HandlerType::TOKEN})};
		stack.emplace_back(handler);

		// Call the startAnnotation method of the newly created handler, store
		// the valid flag
		HandlerInfo &info = currentInfo();
		info.valid = false;
		info.greedy = (!shortForm) || descr.greedyShortForm;
		try {
			info.valid = handler->startToken(descr.descriptor, info.greedy);
		}
		catch (LoggableException ex) {
			logger.log(ex);
		}

		// End the handler again if an error occured
		if (!info.valid) {
			endCurrentHandler();
		}

		// If this is not a short form token and the "close" descriptor is
		// given, mark the current handler as "range" handler
		if (!shortForm && descr.close != Tokens::Empty &&
		    !Token::isSpecial(descr.close)) {
			info.closeToken = descr.close;
			info.tokenDesciptor = descr.descriptor;
			info.range = true;
		}
		return true;
	}
	return false;
}

void StackImpl::handleToken(const Token &token)
{
	// If the token matches one from the "pendingCloseTokens" list, then just
	// end the corresponding handler
	const ssize_t pendingCloseIndex = pendingCloseTokenHandlerIdx(token.id);
	if (pendingCloseIndex >= 0) {
		for (ssize_t i = stack.size() - 1; i >= pendingCloseIndex; i--) {
			endCurrentHandler();
		}
		return;
	}

	// Fetch the TokenDescriptor
	TokenDescriptor descr = tokenStack.lookup(token.id);

	// First try to close pending open tokens, issue an error if this does not
	// work and no shortForm or open tokens are declared and the token is not
	// a special whitespace token
	if (handleCloseTokens(token, descr.close)) {
		return;
	} else if (descr.shortForm.empty() && descr.open.empty()) {
		if (!Token::isSpecial(token.id)) {
			strayTokenError(token, descr, logger());
		}
		return;
	}

	// Make sure the given open token descriptors are unambiguous
	checkTokensAreUnambiguous(token, descr, logger());

	// Now try to handle open or short form tokens. Iterate until the stack can
	// no longer be unwound.
	while (!stack.empty()) {
		LoggerFork loggerFork = logger().fork();

		// TODO: Instead of using hadError flag here implement a "hasError"
		// method for LoggerFork
		bool hadError = false;
		try {
			// Try to open an implicit default field and try to start the token
			// as short form or as start token
			prepareCurrentHandler();
			if (handleOpenTokens(loggerFork, token, true, descr.shortForm) ||
			    handleOpenTokens(loggerFork, token, false, descr.open)) {
				return;
			}
		}
		catch (LoggableException ex) {
			hadError = true;
			loggerFork.log(ex);
		}

		// Neither of the above worked, try to unroll the stack
		HandlerInfo &info = currentInfo();
		if (info.inImplicitDefaultField && !stack.empty()) {
			endCurrentHandler();
		} else {
			// Commit all encountered errors
			loggerFork.commit();

			// If there was no other error message already, issue a "stray
			// token" error
			if (!hadError) {
				strayTokenError(token, descr, logger());
			}
			return;
		}
	}

	// Issue an error, because the token was not handled
	strayTokenError(token, descr, logger());
}

void StackImpl::handleFieldEnd(bool endRange)
{
	// Throw away all overdue handlers
	prepareCurrentHandler(false);

	// Close all implicit default fields
	while (!stack.empty()) {
		HandlerInfo &info = currentInfo();
		if (!info.inImplicitDefaultField || info.range) {
			break;
		}
		endCurrentHandler();
	}

	// Fetch the information attached to the current handler
	HandlerInfo &info = currentInfo();
	if (stack.empty() || (!info.inField && !endRange) ||
	    (!info.range && endRange)) {
		if (endRange) {
			logger().error(
			    "Got end of range, but there is no command here to end");
		} else {
			logger().error("Got field end, but there is no field here to end");
		}
		return;
	}

	// Only continue if the current handler stack is in a valid state, do not
	// call the fieldEnd function if something went wrong before
	if (handlersValid()) {
		// End the current field if it is valid
		if (info.inValidField) {
			info.handler->fieldEnd();
			info.fieldEnd();
		}

		// End the complete command if this is a range command, start the
		// default field for once if range command did not have a default field
		if (info.range && endRange) {
			if (!info.hadDefaultField) {
				bool isDefault = true;
				bool valid =
				    info.handler->fieldStart(isDefault, false, info.fieldIdx);
				info.fieldStart(true, false, valid);
			}
			endCurrentHandler();
			return;
		}
	}

	// This command no longer is in a field
	info.fieldEnd();
}

void StackImpl::handleAnnotationStartEnd(const Variant &name,
                                         Variant::mapType args, bool range,
                                         HandlerType type)
{
	// Prepare the stack -- make sure all overdue handlers are ended and
	// we currently are in an open field
	if (stack.empty() || !prepareCurrentHandler()) {
		throw LoggableException("Did not expect an annotation start here");
	}

	// Find the special target state TODO: Is there some better solution?
	const State *state = findTargetState("*");
	if (state == nullptr || !currentInfo().state().supportsAnnotations) {
		throw LoggableException("Cannot handle annotations here");
	}

	// If we're currently in an invalid subtree, just eat the data and abort
	if (!handlersValid()) {
		return;
	}

	// Instantiate the handler and push it onto the stack
	HandlerConstructor ctor =
	    state->elementHandler ? state->elementHandler : EmptyHandler::create;
	std::shared_ptr<Handler> handler{ctor(
	    {ctx, *this, *state, {name.asString(), name.getLocation()}, type})};
	stack.emplace_back(handler);

	// Call the startAnnotation method of the newly created handler, store the
	// valid flag
	HandlerInfo &info = currentInfo();
	info.valid = false;
	try {
		info.valid = handler->startAnnotation(args);
	}
	catch (LoggableException ex) {
		logger().log(ex);
	}
	info.range = range;

	// End the handler directly if this is an annotation end
	if (type == HandlerType::ANNOTATION_END) {
		endCurrentHandler();
	}
}

/* Class StackImpl public functions */

void StackImpl::commandStart(const Variant &name, const Variant::mapType &args,
                             bool range)
{
	// Call prepareCurrentHandler once to end all overdue commands
	prepareCurrentHandler();

	// Make sure the given identifier is valid (preventing "*" from being
	// malicously passed to this function)
	if (!Utils::isNamespacedIdentifier(name.asString())) {
		throw LoggableException(std::string("Invalid identifier \"") +
		                            name.asString() + std::string("\""),
		                        name);
	}

	while (true) {
		// Prepare the stack -- make sure all overdue handlers are ended and
		// we currently are in an open field
		prepareCurrentHandler();

		// Try to find a target state for the given command, if none can be
		// found and the current command does not have an open field, then try
		// to create an empty default field, otherwise this is an exception
		const State *targetState = findTargetStateOrWildcard(name.asString());
		if (targetState == nullptr) {
			HandlerInfo &info = currentInfo();
			if ((info.inImplicitDefaultField || !info.inField) &&
			    endCurrentHandler()) {
				continue;
			} else {
				throw buildInvalidCommandException(name.asString(),
				                                   expectedCommands());
			}
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
				info.valid = handler->startCommand(canonicalArgs);
			}
			catch (LoggableException ex) {
				loggerFork.log(ex);
			}
			handler->resetLogger();
		}

		// We started the command within an implicit default field and it is
		// not valid -- remove both the new handler and the parent field from
		// the stack
		if (!info.valid && parentInfo.inImplicitDefaultField) {
			// Only continue if the parent handler could actually be removed
			if (endCurrentHandler() && endCurrentHandler()) {
				continue;
			}
		}

		// If we ended up here, starting the command may or may not have
		// worked, but after all, we cannot unroll the stack any further. Update
		// the "valid" flag, commit any potential error messages and return.
		info.valid = parentInfo.valid && info.valid;
		info.range = range;
		loggerFork.commit();
		return;
	}
}

void StackImpl::annotationStart(const Variant &className,
                                const Variant::mapType &args, bool range)
{
	handleAnnotationStartEnd(className, args, range,
	                         HandlerType::ANNOTATION_START);
}

void StackImpl::annotationEnd(const Variant &className,
                              const Variant::mapType &args)
{
	handleAnnotationStartEnd(className, args, false,
	                         HandlerType::ANNOTATION_END);
}

void StackImpl::rangeEnd() { handleFieldEnd(true); }

void StackImpl::data(const TokenizedData &data)
{
	// Fetch a reader for the given tokenized data instance.
	TokenizedDataReader reader = data.reader();

	// Use the GuardedTemporaryPointer to make sure that the member variable
	// dataReader is resetted to nullptr once this scope is left.
	GuardedTemporaryPointer<TokenizedDataReader> ptr(&reader, &dataReader);

	// Close all handlers that did already had or cannot have a default field
	// and are not currently inside a field (repeat this after each chunk of
	// data/text)
	prepareCurrentHandler(false, false, false);

	// Peek a token from the reader, repeat until all tokens have been read
	Token token;

	while (readToken(token)) {
		// Handle the token as text data or as actual token
		if (token.id == Tokens::Data) {
			// Only consume the data if reading was sucessful -- sometimes (as
			// it turns out) -- there is no data
			if (handleData()) {
				reader.consumePeek();
			}
		} else {
			handleToken(token);
			reader.consumePeek();
		}
		prepareCurrentHandler(false, false, false);
	}
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
	// field (the default field always is the last field) -- mark the command as
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
			valid =
			    info.handler->fieldStart(defaultField, false, info.fieldIdx);
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

void StackImpl::fieldEnd() { handleFieldEnd(false); }

/* Class StackImpl HandlerCallbacks */

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
	tokenStack.pushTokens(tokens);
}

void StackImpl::popTokens() { tokenStack.popTokens(); }

bool StackImpl::readToken(Token &token)
{
	if (dataReader != nullptr) {
		dataReader->resetPeek();
		return dataReader->peek(token, currentTokens(),
		                        currentWhitespaceMode());
	}
	return false;
}

Variant StackImpl::readData()
{
	Token token;
	if (readToken(token) && token.id == Tokens::Data) {
		// TODO: Introduce function for string variant with location
		Variant res = Variant::fromString(token.content);
		res.setLocation(token.getLocation());
		return res;
	}
	return Variant{};
}

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
#if STACK_DEBUG_OUTPUT
	std::cout << "STACK: commandStart " << name << " " << args << " " << range
	          << std::endl;
#endif
	impl->commandStart(name, args, range);
}

void Stack::annotationStart(const Variant &className,
                            const Variant::mapType &args, bool range)
{
#if STACK_DEBUG_OUTPUT
	std::cout << "STACK: annotationStart " << className << " " << args << " "
	          << range << std::endl;
#endif
	impl->annotationStart(className, args, range);
}

void Stack::annotationEnd(const Variant &className,
                          const Variant::mapType &args)
{
#if STACK_DEBUG_OUTPUT
	std::cout << "STACK: annotationEnd " << className << " " << args
	          << std::endl;
#endif
	impl->annotationEnd(className, args);
}

void Stack::rangeEnd()
{
#if STACK_DEBUG_OUTPUT
	std::cout << "STACK: rangeEnd" << std::endl;
#endif
	impl->rangeEnd();
}

void Stack::fieldStart(bool isDefault)
{
#if STACK_DEBUG_OUTPUT
	std::cout << "STACK: fieldStart " << isDefault << std::endl;
#endif
	impl->fieldStart(isDefault);
}

void Stack::fieldEnd()
{
#if STACK_DEBUG_OUTPUT
	std::cout << "STACK: fieldEnd" << std::endl;
#endif
	impl->fieldEnd();
}

void Stack::data(const TokenizedData &data)
{
#if STACK_DEBUG_OUTPUT
	std::cout << "STACK: data" << std::endl;
#endif
	impl->data(data);
}

void Stack::data(const std::string &str)
{
#if STACK_DEBUG_OUTPUT
	std::cout << "STACK: data (string) " << str << std::endl;
#endif
	data(TokenizedData(str));
}
}
}
