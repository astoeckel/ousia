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
 * @file ParserStack.hpp
 *
 * Helper classes for document or description parsers. Contains the ParserStack
 * class, which is an pushdown automaton responsible for accepting commands in
 * the correct order and calling specified handlers.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_HPP_
#define _OUSIA_PARSER_STACK_HPP_

#include <cstdint>

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <vector>

#include <core/common/Variant.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Argument.hpp>

#include "Parser.hpp"
#include "ParserContext.hpp"
#include "ParserState.hpp"

namespace ousia {

/**
 * Struct collecting all the data that is being passed to a Handler instance.
 */
struct HandlerData {
	/**
	 * Reference to the ParserContext instance that should be used to resolve
	 * references to nodes in the Graph.
	 */
	ParserContext &ctx;

	/**
	 * Contains the name of the tag that is being handled.
	 */
	const std::string name;

	/**
	 * Contains the current state of the state machine.
	 */
	const ParserState &state;

	/**
	 * Contains the state of the state machine when the parent node was handled.
	 */
	const ParserState &parentState;

	/**
	 * Current source code location.
	 */
	const SourceLocation location;

	/**
	 * Constructor of the HandlerData class.
	 *
	 * @param ctx is the parser context the handler should be executed in.
	 * @param name is the name of the string.
	 * @param state is the state this handler was called for.
	 * @param parentState is the state of the parent command.
	 * @param location is the location at which the handler is created.
	 */
	HandlerData(ParserContext &ctx, std::string name,
	            const ParserState &state, const ParserState &parentState,
	            const SourceLocation location)
	    : ctx(ctx),
	      name(std::move(name)),
	      state(state),
	      parentState(parentState),
	      location(location){};
};

/**
 * The handler class provides a context for handling an XML tag. It has to be
 * overridden and registered in the StateStack class to form handlers for
 * concrete XML tags.
 */
class Handler {
private:
	/**
	 * Structure containing the internal handler data.
	 */
	const HandlerData handlerData;

public:
	/**
	 * Constructor of the Handler class.
	 *
	 * @param data is a structure containing all data being passed to the
	 * handler.
	 */
	Handler(const HandlerData &handlerData) : handlerData(handlerData){};

	/**
	 * Virtual destructor.
	 */
	virtual ~Handler(){};

	/**
	 * Returns a reference at the ParserContext.
	 *
	 * @return a reference at the ParserContext.
	 */
	ParserContext &context() { return handlerData.ctx; }

	/**
	 * Returns the command name for which the handler was created.
	 *
	 * @return a const reference at the command name.
	 */
	const std::string &name() { return handlerData.name; }

	/**
	 * Returns a reference at the ParserScope instance.
	 *
	 * @return a reference at the ParserScope instance.
	 */
	ParserScope &scope() { return handlerData.ctx.getScope(); }

	/**
	 * Returns a reference at the Manager instance which manages all nodes.
	 *
	 * @return a referance at the Manager instance.
	 */
	Manager &manager() { return handlerData.ctx.getManager(); }

	/**
	 * Returns a reference at the Logger instance used for logging error
	 * messages.
	 *
	 * @return a reference at the Logger instance.
	 */
	Logger &logger() { return handlerData.ctx.getLogger(); }

	/**
	 * Returns a reference at the Project Node, representing the project into
	 * which the file is currently being parsed.
	 *
	 * @return a referance at the Project Node.
	 */
	Rooted<Project> project() { return handlerData.ctx.getProject(); }

	/**
	 * Reference at the ParserState descriptor for which this Handler was
	 * created.
	 *
	 * @return a const reference at the constructing ParserState descriptor.
	 */
	const ParserState &state() { return handlerData.state; }

	/**
	 * Reference at the ParserState descriptor of the parent state of the state
	 * for which this Handler was created. Set to ParserStates::None if there
	 * is no parent state.
	 *
	 * @return a const reference at the parent state of the constructing
	 * ParserState descriptor.
	 */
	const ParserState &parentState() { return handlerData.parentState; }

	/**
	 * Returns the current location in the source file.
	 *
	 * @return the current location in the source file.
	 */
	SourceLocation location() { return handlerData.location; }

	/**
	 * Called when the command that was specified in the constructor is
	 * instanciated.
	 *
	 * @param args is a map from strings to variants (argument name and value).
	 */
	virtual void start(Variant::mapType &args) = 0;

	/**
	 * Called whenever the command for which this handler is defined ends.
	 */
	virtual void end() = 0;

	/**
	 * Called whenever raw data (int the form of a string) is available for the
	 * Handler instance. In the default handler an exception is raised if the
	 * received data contains non-whitespace characters.
	 *
	 * @param data is a pointer at the character data that is available for the
	 * Handler instance.
	 * @param field is the field number (the interpretation of this value
	 * depends on the format that is being parsed).
	 */
	virtual void data(const std::string &data, int field);
};

/**
 * HandlerConstructor is a function pointer type used to create concrete
 * instances of the Handler class.
 *
 * @param handlerData is the data that should be passed to the new handler
 * instance.
 * @return a newly created handler instance.
 */
using HandlerConstructor = Handler *(*)(const HandlerData &handlerData);

/**
 * The ParserStack class is a pushdown automaton responsible for turning a
 * command stream into a tree of Node instances.
 */
class ParserStack {
private:
	/**
	 * Reference at the parser context.
	 */
	ParserContext &ctx;

	/**
	 * Map containing all registered command names and the corresponding
	 * state descriptors.
	 */
	const std::multimap<std::string, const ParserState *> &states;

	/**
	 * Internal stack used for managing the currently active Handler instances.
	 */
	std::stack<std::shared_ptr<Handler>> stack;

	/**
	 * Used internally to get all expected command names for the given state
	 * (does not work if the current Handler instance allows arbitrary
	 * children). This function is used to build error messages.
	 *
	 * @param state is the state for which all expected command names should be
	 * returned.
	 */
	std::set<std::string> expectedCommands(const ParserState &state);

public:
	/**
	 * Creates a new instance of the ParserStack class.
	 *
	 * @param ctx is the parser context the parser stack is working on.
	 * @param states is a map containing the command names and pointers at the
	 * corresponding ParserState instances.
	 */
	ParserStack(ParserContext &ctx,
	            const std::multimap<std::string, const ParserState *> &states);

	/**
	 * Returns the state the ParserStack instance currently is in.
	 *
	 * @return the state of the currently active Handler instance or STATE_NONE
	 * if no handler is on the stack.
	 */
	const ParserState &currentState();

	/**
	 * Returns the command name that is currently being handled.
	 *
	 * @return the name of the command currently being handled by the active
	 * Handler instance or an empty string if no handler is currently active.
	 */
	std::string currentCommandName();

	/**
	 * Function that should be called whenever a new command starts.
	 *
	 * @param name is the name of the command.
	 * @param args is a map from strings to variants (argument name and value).
	 * Note that the passed map will be modified.
	 * @param location is the location in the source file at which the command
	 * starts.
	 */
	void start(std::string name, Variant::mapType &args,
	           const SourceLocation &location = SourceLocation{});

	/**
	 * Function that should be called whenever a new command starts.
	 *
	 * @param name is the name of the command.
	 * @param args is a map from strings to variants (argument name and value).
	 * @param location is the location in the source file at which the command
	 * starts.
	 */
	void start(std::string name,
	           const Variant::mapType &args = Variant::mapType{},
	           const SourceLocation &location = SourceLocation{});

	/**
	 * Function called whenever a command ends.
	 */
	void end();

	/**
	 * Function that should be called whenever data is available for the
	 * command.
	 *
	 * @param data is the data that should be passed to the handler.
	 * @param field is the field number (the interpretation of this value
	 * depends on the format that is being parsed).
	 */
	void data(const std::string &data, int field = 0);

	/**
	 * Returns a reference to the parser context the parser stack is currently
	 * working on.
	 *
	 * @return a reference to the parser context.
	 */
	ParserContext &getContext() { return ctx; }
};
}

#endif /* _OUSIA_PARSER_STACK_HPP_ */

