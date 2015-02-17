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

/**
 * @file Stack.hpp
 *
 * Helper classes for document or description parsers. Contains the
 * Stack class, which is an pushdown automaton responsible for
 * accepting commands in the correct order and calling specified handlers.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_STACK_HPP_
#define _OUSIA_PARSER_STACK_STACK_HPP_

#include <cstdint>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include <core/common/Variant.hpp>
#include <core/parser/Parser.hpp>

namespace ousia {

// Forward declarations
class ParserContext;
class Logger;

namespace parser_stack {

// Forward declarations
class Handler;
class State;

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
	HandlerInfo(bool valid, bool implicit, bool inField, bool inDefaultField,
	            bool inImplicitDefaultField, bool inValidField);

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
};

/**
 * The Stack class is a pushdown automaton responsible for turning a command
 * stream into a tree of Node instances. It does so by following a state
 * transition graph and creating a set of Handler instances, which are placed
 * on the stack.
 */
class Stack {
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
	 * Internal stack used for managing the currently active Handler instances.
	 */
	std::vector<HandlerInfo> stack;

	/**
	 * Return the reference in the Logger instance stored within the context.
	 */
	Logger &logger();

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
	/**
	 * Creates a new instance of the Stack class.
	 *
	 * @param ctx is the parser context the parser stack is working on.
	 * @param states is a map containing the command names and pointers at the
	 * corresponding State instances.
	 */
	Stack(ParserContext &ctx,
	      const std::multimap<std::string, const State *> &states);

	/**
	 * Destructor of the Stack class.
	 */
	~Stack();

	/**
	 * Returns the state the Stack instance currently is in.
	 *
	 * @return the state of the currently active Handler instance or STATE_NONE
	 * if no handler is on the stack.
	 */
	const State &currentState();

	/**
	 * Returns the command name that is currently being handled.
	 *
	 * @return the name of the command currently being handled by the active
	 * Handler instance or an empty string if no handler is currently active.
	 */
	std::string currentCommandName();

	/**
	 * Function that should be called whenever a new command is reached.
	 *
	 * @param name is the name of the command (including the namespace
	 * separator ':') and its corresponding location. Must be a string variant.
	 * @param args is a map containing the arguments that were passed to the
	 * command.
	 */
	void command(const Variant &name, const Variant::mapType &args);

	/**
	 * Function that shuold be called whenever character data is found in the
	 * input stream. May only be called if the currently is a command on the
	 * stack.
	 *
	 * @param data is a string variant containing the data that has been found.
	 */
	void data(const Variant &data);

	/**
	 * Function that should be called whenever a new field starts. Fields of the
	 * same command may not be separated by calls to data or annotations. Doing
	 * so will result in a LoggableException.
	 *
	 * @param isDefault should be set to true if the started field explicitly
	 * is the default field.
	 */
	void fieldStart(bool isDefault);

	/**
	 * Function that should be called whenever a field ends. Calling this
	 * function if there is no field to end will result in a LoggableException.
	 */
	void fieldEnd();

	/**
	 * Function that should be called whenever an annotation starts.
	 *
	 * @param name is the name of the annotation class.
	 * @param args is a map variant containing the arguments that were passed
	 * to the annotation.
	 */
	void annotationStart(const Variant &className, const Variant &args);

	/**
	 * Function that should be called whenever an annotation ends.
	 *
	 * @param name is the name of the annotation class that was ended.
	 * @param annotationName is the name of the annotation that was ended.
	 */
	void annotationEnd(const Variant &className, const Variant &elementName);

	/**
	 * Function that should be called whenever a previously registered token
	 * is found in the input stream.
	 *
	 * @param token is string variant containing the token that was encountered.
	 */
	void token(Variant token);
};
}
}

#endif /* _OUSIA_STACK_HPP_ */

