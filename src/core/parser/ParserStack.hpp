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

#include <core/variant/Variant.hpp>

#include "Parser.hpp"

namespace ousia {
namespace parser {

/**
 * The State type alias is used to
 */
using State = int16_t;

static const State STATE_ALL = -2;
static const State STATE_NONE = -1;

/**
 * The handler class provides a context for handling an XML tag. It has to be
 * overridden and registered in the StateStack class to form handlers for
 * concrete XML tags.
 */
class Handler {
private:
	Rooted<Node> node;

protected:
	void setNode(Handle<Node> node) { this->node = node; }

public:
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
	 * Constructor of the Handler class.
	 *
	 * @param ctx is the parser context the handler should be executed in.
	 * @param name is the name of the string.
	 * @param state is the state this handler was called for.
	 * @param parentState is the state of the parent command.
	 * @param isChild specifies whether this handler was called not for the
	 * command that was specified in the state machine but a child command.
	 */
	Handler(const ParserContext &ctx, std::string name, State state,
	        State parentState, bool isChild)
	    : ctx(ctx),
	      name(std::move(name)),
	      state(state),
	      parentState(parentState),
	      isChild(isChild){};

	/**
	 * Virtual destructor.
	 */
	virtual ~Handler(){};

	/**
	 * Returns the node instance that was created by the handler.
	 *
	 * @return the Node instance created by the handler. May be nullptr if no
	 * Node was created.
	 */
	Rooted<Node> getNode() { return node; }

	/**
	 * Called when the command that was specified in the constructor is
	 * instanciated.
	 *
	 * @param args is a map from strings to variants (argument name and value).
	 */
	virtual void start(const Variant &args) = 0;

	/**
	 * Called whenever the command for which this handler
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
	virtual void child(std::shared_ptr<Handler> handler){};
};

/**
 * HandlerConstructor is a function pointer type used to create concrete
 * instances of the Handler class.
 */
using HandlerConstructor = Handler *(*)(const ParserContext &ctx,
                                        std::string name, State state,
                                        State parentState, bool isChild);

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

	HandlerDescriptor(std::set<State> parentStates, HandlerConstructor ctor,
	                  State targetState, bool arbitraryChildren = false)
	    : parentStates(std::move(parentStates)),
	      ctor(ctor),
	      targetState(targetState),
	      arbitraryChildren(arbitraryChildren)
	{
	}

	/**
	 * Creates an instance of the concrete Handler class represented by the
	 * HandlerDescriptor and calls its start function.
	 */
	HandlerInstance create(const ParserContext &ctx, std::string name,
	                       State parentState, bool isChild,
	                       const Variant &args) const;
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
	const ParserContext &ctx;

	/**
	 * User specified data that will be passed to all handlers.
	 */
	void *userData;

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
	 * @param handlers is a map containing the command names and the
	 * corresponding HandlerDescriptor instances.
	 */
	ParserStack(const ParserContext &ctx,
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
		return stack.empty() ? STATE_NONE : stack.top().handler->state;
	}

	/**
	 * Returns the command name that is currently being handled.
	 *
	 * @return the name of the command currently being handled by the active
	 * Handler instance or an empty string if no handler is currently active.
	 */
	std::string currentName()
	{
		return stack.empty() ? std::string{} : stack.top().handler->name;
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
	 */
	void start(std::string name, const Variant &args);

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
};
}
}

#endif /* _OUSIA_PARSER_STACK_HPP_ */

