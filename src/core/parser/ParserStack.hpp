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

namespace ousia {

/**
 * The State type alias is used to
 */
using State = int16_t;

static const State STATE_ALL = -2;
static const State STATE_NONE = -1;

/**
 * Struct collecting all the data that is being passed to a Handler instance.
 */
struct HandlerData {
	/**
	 * Reference to the ParserContext instance that should be used to resolve
	 * references to nodes in the Graph.
	 */
	const ParserContext &ctx;

	/**
	 * Contains the name of the tag that is being handled.
	 */
	const std::string name;

	/**
	 * Contains the current state of the state machine.
	 */
	const State state;

	/**
	 * Contains the state of the state machine when the parent node was handled.
	 */
	const State parentState;

	/**
	 * Set to true if the tag that is being handled is not the tag that was
	 * specified in the state machine but a child tag of that tag.
	 */
	const bool isChild;

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
	 * @param isChild specifies whether this handler was called not for the
	 * command that was specified in the state machine but a child command.
	 * @param location is the location at which the handler is created.
	 */
	HandlerData(const ParserContext &ctx, std::string name, State state,
	            State parentState, bool isChild, const SourceLocation location)
	    : ctx(ctx),
	      name(std::move(name)),
	      state(state),
	      parentState(parentState),
	      isChild(isChild),
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

	const std::string &name() { return handlerData.name; }

	ParserScope &scope() { return handlerData.ctx.getScope(); }

	Manager &manager() { return handlerData.ctx.getManager(); }

	Logger &logger() { return handlerData.ctx.getLogger(); }

	Rooted<Project> project() { return handlerData.ctx.getProject(); }

	State state() { return handlerData.state; }

	State parentState() { return handlerData.parentState; }

	SourceLocation location() { return handlerData.location; }

	bool isChild() { return handlerData.isChild; }

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

	/**
	 * Called whenever a direct child element was created and has ended.
	 *
	 * @param handler is a reference at the child Handler instance.
	 */
	virtual void child(std::shared_ptr<Handler> handler);
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

struct HandlerDescriptor;

/**
 * Used internlly by StateStack to store Handler instances and parameters
 * from HandlerDescriptor that are not stored in the Handler instance
 * itself. Instances of the HandlerInstance class can be created using the
 * HandlerDescriptor "create" method.
 */
struct HandlerInstance {
	/**
	 * Pointer at the actual handler instance.
	 */
	std::shared_ptr<Handler> handler;

	/**
	 * Pointer pointing at the descriptor from which the handler instance was
	 * derived.
	 */
	const HandlerDescriptor *descr;

	HandlerInstance(Handler *handler, const HandlerDescriptor *descr)
	    : handler(handler), descr(descr)
	{
	}
};

/**
 * Used internally by StateStack to store the pushdown automaton
 * description.
 */
struct HandlerDescriptor {
	/**
	 * The valid parent states.
	 */
	const std::set<State> parentStates;

	/**
	 * Pointer at a function which creates a new concrete Handler instance.
	 */
	const HandlerConstructor ctor;

	/**
	 * The target state for the registered handler.
	 */
	const State targetState;

	/**
	 * Set to true if this handler instance allows arbitrary children as
	 * tags.
	 */
	const bool arbitraryChildren;

	/**
	 * Reference at an argument descriptor that should be used for validating
	 * the incomming arguments.
	 */
	const Arguments arguments;

	/**
	 * Constructor of the HandlerDescriptor class.
	 *
	 * @param parentStates is a set of states in which a new handler of this
	 * type may be instantiated.
	 * @param ctor is a function pointer pointing at a function that
	 * instantiates the acutal Handler instance.
	 * @param targetState is the state the ParserStack switches to after
	 * instantiating an in instance of the described Handler instances.
	 * @param arbitraryChildren allows the Handler instance to handle any child
	 * node.
	 * @param arguments is an optional argument descriptor used for validating
	 * the arguments that are passed to the instantiation of a handler function.
	 */
	HandlerDescriptor(std::set<State> parentStates, HandlerConstructor ctor,
	                  State targetState, bool arbitraryChildren = false,
	                  Arguments arguments = Arguments::None)
	    : parentStates(std::move(parentStates)),
	      ctor(ctor),
	      targetState(targetState),
	      arbitraryChildren(arbitraryChildren),
	      arguments(std::move(arguments))
	{
	}

	/**
	 * Creates an instance of the concrete Handler class represented by the
	 * HandlerDescriptor and calls its start function.
	 */
	HandlerInstance create(const ParserContext &ctx, std::string name,
	                       State parentState, bool isChild,
	                       Variant::mapType &args,
	                       const SourceLocation &location) const;
};

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
	 * Current location in the source code.
	 */
	SourceLocation location;

	/**
	 * Map containing all registered command names and the corresponding
	 * handler
	 * descriptor.
	 */
	const std::multimap<std::string, HandlerDescriptor> &handlers;

	/**
	 * Internal stack used for managing the currently active Handler instances.
	 */
	std::stack<HandlerInstance> stack;

	/**
	 * Used internally to get all expected command names for the given state
	 * (does not work if the current Handler instance allows arbitrary
	 * children). This function is used to build error messages.
	 *
	 * @param state is the state for which all expected command names should be
	 * returned.
	 */
	std::set<std::string> expectedCommands(State state);

public:
	/**
	 * Creates a new instance of the ParserStack class.
	 *
	 * @param ctx is the parser context the parser stack is working on.
	 * @param handlers is a map containing the command names and the
	 * corresponding HandlerDescriptor instances.
	 */
	ParserStack(ParserContext &ctx,
	            const std::multimap<std::string, HandlerDescriptor> &handlers)
	    : ctx(ctx), handlers(handlers){};

	/**
	 * Returns the state the ParserStack instance currently is in.
	 *
	 * @return the state of the currently active Handler instance or STATE_NONE
	 * if no handler is on the stack.
	 */
	State currentState()
	{
		return stack.empty() ? STATE_NONE : stack.top().handler->state();
	}

	/**
	 * Returns the command name that is currently being handled.
	 *
	 * @return the name of the command currently being handled by the active
	 * Handler instance or an empty string if no handler is currently active.
	 */
	std::string currentName()
	{
		return stack.empty() ? std::string{} : stack.top().handler->name();
	}

	/**
	 * Returns whether the current command handler allows arbitrary children.
	 *
	 * @return true if the handler allows arbitrary children, false otherwise.
	 */
	bool currentArbitraryChildren()
	{
		return stack.empty() ? false : stack.top().descr->arbitraryChildren;
	}

	/**
	 * Function that should be called whenever a new command starts.
	 *
	 * @param name is the name of the command.
	 * @param args is a map from strings to variants (argument name and value).
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
	void start(std::string name, const Variant::mapType &args,
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

