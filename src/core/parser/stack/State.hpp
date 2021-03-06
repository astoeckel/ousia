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
 * @file State.hpp
 *
 * Defines the State class used within the ParserStack pushdown
 * automaton and the StateBuilder class for convenient construction of
 * such classes.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STATE_HPP_
#define _OUSIA_PARSER_STATE_HPP_

#include <unordered_set>

#include <core/common/Rtti.hpp>
#include <core/common/Argument.hpp>
#include <core/common/Whitespace.hpp>

namespace ousia {
namespace parser_stack {

// Forward declarations
class StateBuilder;
class State;
class HandlerData;
class Handler;
using HandlerConstructor = Handler *(*)(const HandlerData &handlerData);

/**
 * Set of pointers of parser states -- used for specifying a set of parent
 * states.
 */
using StateSet = std::unordered_set<const State *>;

/**
 * Class used for the complete specification of a State. Stores possible
 * parent states, state handlers and arguments to be passed to that state.
 */
class State {
public:
	/**
	 * Vector containing all possible parent states.
	 */
	StateSet parents;

	/**
	 * Descriptor of the arguments that should be passed to the handler.
	 */
	Arguments arguments;

	/**
	 * Set containing the types of the nodes that may be created in this
	 * State. This information is needed for Parsers to reconstruct the
	 * current State from a given ParserScope when a file is included.
	 */
	RttiSet createdNodeTypes;

	/**
	 * Pointer at a function which creates a new concrete Handler instance for
	 * the elements described by this state. May be nullptr in which case no
	 * handler instance is created.
	 */
	HandlerConstructor elementHandler;

	/**
	 * Set to true if this handler does support annotations. This is almost
	 * always false (e.g. all description handlers), except for document
	 * element handlers.
	 */
	bool supportsAnnotations : 1;

	/**
	 * Set to true if this handler does support tokens. This is almost
	 * always false (e.g. all description handlers), except for document
	 * element handlers.
	 */
	bool supportsTokens : 1;

	/**
	 * Default constructor, initializes the handlers with nullptr and the
	 * supportsAnnotations and supportsTokens flags with false.
	 */
	State();

	/**
	 * Constructor taking values for all fields. Use the StateBuilder
	 * class for a more convenient construction of State instances.
	 *
	 * @param parents is a vector containing all possible parent states.
	 * @param arguments is a descriptor of arguments that should be passed to
	 * the handler.
	 * @param createdNodeTypes is a set containing the types of the nodes tha
	 * may be created in this State. This information is needed for
	 * Parsers to reconstruct the current State from a given ParserScope
	 * when a file is included.
	 * @param elementHandler is a pointer at a function which creates a new
	 * concrete Handler instance for the elements described by this state. May
	 * be nullptr in which case no handler instance is created.
	 * @param supportsAnnotations specifies whether annotations are supported
	 * here at all.
	 * @param supportsTokens specified whether tokens are supported here at all.
	 */
	State(StateSet parents, Arguments arguments = Arguments{},
	      RttiSet createdNodeTypes = RttiSet{},
	      HandlerConstructor elementHandler = nullptr,
	      bool supportsAnnotations = false, bool supportsTokens = false);

	/**
	 * Creates this State from the given StateBuilder instance.
	 */
	State(const StateBuilder &builder);
};

/**
 * The StateBuilder class is a class used for conveniently building new
 * State instances.
 */
class StateBuilder {
private:
	/**
	 * State instance that is currently being built by the
	 * StateBuilder.
	 */
	State state;

public:
	/**
	 * Copies the State instance and uses it as internal state. Overrides
	 * all changes made by the StateBuilder.
	 *
	 * @param state is the state that should be copied.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &copy(const State &state);

	/**
	 * Sets the possible parent states to the single given parent element.
	 *
	 * @param parent is a pointer at the parent State instance that should
	 * be the possible parent state.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &parent(const State *parent);

	/**
	 * Sets the State instances in the given StateSet as the list of
	 * supported parent states.
	 *
	 * @param parents is a set of pointers at State instances that should
	 * be the possible parent states.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &parents(const StateSet &parents);

	/**
	 * Sets the arguments that should be passed to the parser state handler to
	 * those given as argument.
	 *
	 * @param arguments is the Arguments instance describing the Arguments that
	 * should be parsed to a Handler for this State.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &arguments(const Arguments &arguments);

	/**
	 * Sets the Node types this state may produce to the given Rtti descriptor.
	 *
	 * @param type is the Rtti descriptor of the Type that may be produced by
	 * this state.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &createdNodeType(const Rtti *type);

	/**
	 * Sets the Node types this state may produce to the given Rtti descriptors.
	 *
	 * @param types is a set of Rtti descriptors of the Types that may be
	 * produced by this state.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &createdNodeTypes(const RttiSet &types);

	/**
	 * Sets the constructor for the element handler. The constructor creates a
	 * new concrete Handler instance for the elements described by this state.
	 * May be nullptr in which case no handler instance is created (this is
	 * the default value).
	 *
	 * @param elementHandler is the HandlerConstructor that should create a
	 * new Handler instance.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &elementHandler(HandlerConstructor elementHandler);

	/**
	 * Sets the state of the "supportsAnnotations" flags (default value is
	 * false)
	 *
	 * @param supportsAnnotations should be set to true, if annotations are
	 * supported for the handlers associated with this document.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &supportsAnnotations(bool supportsAnnotations);

	/**
	 * Sets the state of the "supportsTokens" flag (default value is false).
	 *
	 * @param supportsTokens should be set to true, if the elementHandler
	 * registered for this state is capable of handling tokens.
	 * @return a reference at this StateBuilder instance for method
	 * chaining.
	 */
	StateBuilder &supportsTokens(bool supportsTokens);

	/**
	 * Returns a reference at the internal State instance that was built
	 * using the StateBuilder.
	 *
	 * @return the built State.
	 */
	const State &build() const;
};

/**
 * Class used to deduce the State a Parser is currently in based on the
 * types of the Nodes that currently are on the ParserStack. Uses dynamic
 * programming in order to solve this problem.
 */
class StateDeductor {
public:
	/**
	 * Type containing the dynamic programming table.
	 */
	using Table = std::vector<std::unordered_map<const State *, bool>>;

private:
	/**
	 * Dynamic programming table.
	 */
	Table tbl;

	/**
	 * Signature given in the constructor.
	 */
	const std::vector<const Rtti *> signature;

	/**
	 * List of states that should be checked for being active.
	 */
	const std::vector<const State *> states;

	/**
	 * Used internally to check whether the given parser stack s may have been
	 * active for signature element d.
	 *
	 * @param d is the signature element.
	 * @param s is the parser state.
	 * @return true if the the given State may have been active.
	 */
	bool isActive(size_t d, const State *s);

public:
	/**
	 * Constructor of the StateDeductor class.
	 *
	 * @param signature a Node type signature describing the types of the nodes
	 * which currently reside on e.g. the ParserScope stack.
	 * @param states is a list of states that should be checked.
	 */
	StateDeductor(std::vector<const Rtti *> signature,
	              std::vector<const State *> states);

	/**
	 * Selects all active states from the given states. Only considers those
	 * states that may have produced the last signature element.
	 *
	 * @return a list of states that may actually have been active.
	 */
	std::vector<const State *> deduce();
};

/**
 * The States namespace contains all the global state constants used
 * in the ParserStack class.
 */
namespace States {
/**
 * State representing all states.
 */
extern const State All;

/**
 * State representing the initial state.
 */
extern const State None;
}
}
}

#endif /* _OUSIA_PARSER_STATE_HPP_ */

