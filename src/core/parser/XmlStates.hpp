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

#ifndef _OUSIA_XML_STATES_HPP_
#define _OUSIA_XML_STATES_HPP_

#include <cstdint>

#include <map>
#include <set>
#include <stack>
#include <vector>

namespace ousia {
namespace parser {

class Scope;
class Registry;
class Logger;

namespace xml {

/**
 * The State class represents all states the XML parser can be in. These states
 * mostly represent single tags.
 */
enum class State : uint8_t {
	/* Meta states */
	ALL = -1,

	/* Start state */
	NONE,

	/* Special commands */
	INCLUDE,
	INLINE,

	/* Document states */
	DOCUMENT,
	HEAD,
	BODY,

	/* Domain description states */
	DOMAIN,

	/* Type system states */
	TYPESYSTEM,
	TYPE,
	TYPE_ELEM
};

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
	virtual ~Handler();

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
	 * @param attrs contains the attributes that were specified for the command.
	 * TODO: Replace with StructInstance!
	 */
	virtual void start(char **attrs) = 0;

	/**
	 * Called whenever the command for which this handler
	 */
	virtual void end() = 0;

	/**
	 * Called whenever raw data (int the form of a string) is available for the
	 * Handler instance.
	 *
	 * TODO: Replace with std::string?
	 *
	 * @param data is a pointer at the character data that is available for the
	 * Handler instance.
	 */
	virtual void data(char *data, int len){};

	/**
	 * Called whenever a direct child element was created and has ended.
	 *
	 * @param handler is a reference at the child Handler instance.
	 */
	virtual void child(Handler *handler){};
};

/**
 * HandlerConstructor is a function pointer type used to create concrete
 * instances of the Handler class.
 */
using HandlerConstructor = Handler *(*)(const ParserContext &ctx,
                                        std::string name, State state,
                                        State parentState, bool isChild);

/**
 * The StateStack class is a pushdown automaton responsible for turning a
 * command stream into a tree of Node instances.
 */
class StateStack {
public:
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
		std::unique_ptr<Handler> handler;

		/**
		 * Value of the arbitraryChildren flag stored in the HandlerDescriptor
		 * class.
		 */
		const bool arbitraryChildren;

		HandlerInstance(std::unique_ptr<Handler> handler,
		                bool arbitraryChildren)
		    : handler(handler), arbitraryChildren(arbitraryChildren)
		{
		}
	}

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
		      ctor(constructor),
		      targetState(targetState),
		      arbitraryChildren(arbitraryChildren)
		{
		}

		HandlerInstance create(const ParserContext &ctx, std::string name,
		                       State parentState, bool isChild)
		{
			return HandlerInstance{
			    ctor(ctx, name, targetState, parentState, isChild),
			    arbitraryChildren};
		}
	};

private:
	/**
	 * Map containing all registered command names and the corresponding
	 * handler
	 * descriptor.
	 */
	const std::multimap<std::string, HandlerDescriptor> handlers;

	/**
	 * Reference at the parser context.
	 */
	const ParserContext &ctx;

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
	 * Creates a new instance of the StateStack class.
	 *
	 * @param handlers is a map containing the command names and the
	 * corresponding HandlerDescriptor instances.
	 */
	StateStack(const ParserContext &ctx,
	           std::multimap<std::string, HandlerDescriptor> handlers)
	    : handlers(std::move(handlers)),
	      ctx(ctx),
	      currentState(State::NONE),
	      arbitraryChildren(false);
};
}
}
}

#endif /* _OUSIA_XML_STATES_HPP_ */

