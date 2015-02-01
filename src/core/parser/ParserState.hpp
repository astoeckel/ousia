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
 * @file ParserState.hpp
 *
 * Defines the ParserState class used within the ParserStack pushdown
 * automaton and the ParserStateBuilder class for convenient construction of
 * such classes.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STATE_HPP_
#define _OUSIA_PARSER_STATE_HPP_

#include <unordered_set>

#include <core/common/Rtti.hpp>
#include <core/common/Argument.hpp>

namespace ousia {

// Forward declarations
class ParserStateBuilder;
class ParserState;
class HandlerData;
class Handler;
using HandlerConstructor = Handler *(*)(const HandlerData &handlerData);

/**
 * Set of pointers of parser states -- used for specifying a set of parent
 * states.
 */
using ParserStateSet = std::unordered_set<const ParserState *>;

/**
 * Class used for the complete specification of a ParserState. Stores possible
 * parent states, state handlers and arguments to be passed to that state.
 */
struct ParserState {
	/**
	 * Vector containing all possible parent states.
	 */
	ParserStateSet parents;

	/**
	 * Descriptor of the arguments that should be passed to the handler.
	 */
	Arguments arguments;

	/**
	 * Rtti types that are reported as supported when including or importing new
	 * files while in this state. This value is passed as "supportedTypes" to
	 * either the "import" or "include" function.
	 */
	RttiSet supportedTypes;

	/**
	 * Pointer at a function which creates a new concrete Handler instance for
	 * the elements described by this state. May be nullptr in which case no
	 * handler instance is created.
	 */
	HandlerConstructor elementHandler;

	/**
	 * Pointer at a function which creates a new concrete Handler instance for
	 * all child elements for which no matching state is defined. May be nullptr
	 * in which case no such elements are allowed.
	 */
	HandlerConstructor childHandler;

	/**
	 * Default constructor, initializes the handlers with nullptr.
	 */
	ParserState();

	/**
	 * Constructor taking values for all fields. Use the ParserStateBuilder
	 * class for a more convenient construction of ParserState instances.
	 *
	 * @param parents is a vector containing all possible parent states.
	 * @param arguments is a descriptor of arguments that should be passed to
	 * the handler.
	 * @param supportedTypes is a set of Rtti types that are reported as
	 * supported when including or importing new files while in this state.
	 * @param elementHandler is a pointer at a function which creates a new
	 * concrete Handler instance for the elements described by this state. May
	 * be nullptr in which case no handler instance is created.
	 * @param childHandler is a pointer at a function which creates a new
	 * concrete Handler instance for all child elements for which no matching
	 * state is defined. May be nullptr in which case no such elements are
	 * allowed.
	 */
	ParserState(ParserStateSet parents, Arguments arguments = Arguments{},
	            RttiSet supportedTypes = RttiSet{},
	            HandlerConstructor elementHandler = nullptr,
	            HandlerConstructor childHandler = nullptr);

	/**
	 * Creates this ParserState from the given ParserStateBuilder instance.
	 */
	ParserState(const ParserStateBuilder &builder);
};

/**
 * The ParserStateBuilder class is a class used for conveniently building new
 * ParserState instances.
 */
class ParserStateBuilder {
private:
	/**
	 * ParserState instance that is currently being built by the
	 * ParserStateBuilder.
	 */
	ParserState state;

public:
	/**
	 * Copies the ParserState instance and uses it as internal state. Overrides
	 * all changes made by the ParserStateBuilder.
	 *
	 * @param state is the state that should be copied.
	 * @return a reference at this ParserStateBuilder instance for method
	 * chaining.
	 */
	ParserStateBuilder &copy(const ParserState &state);

	/**
	 * Adds the given ParserState to the list of supported parent states.
	 *
	 * @param parent is a pointer at the parent ParserState instance that should
	 * be added as possible parent state.
	 * @return a reference at this ParserStateBuilder instance for method
	 * chaining.
	 */
	ParserStateBuilder &parent(const ParserState *parent);

	/**
	 * Adds the ParserState instances in the given ParserStateSet to the list of
	 * supported parent states.
	 *
	 * @param parents is a set of pointers at ParserState instances that should
	 * be added as possible parent states.
	 * @return a reference at this ParserStateBuilder instance for method
	 * chaining.
	 */
	ParserStateBuilder &parents(const ParserStateSet &parents);

	/**
	 * Sets the arguments that should be passed to the parser state handler to
	 * those given as argument.
	 *
	 * @param arguments is the Arguments instance describing the Arguments that
	 * should be parsed to a Handler for this ParserState.
	 * @return a reference at this ParserStateBuilder instance for method
	 * chaining.
	 */
	ParserStateBuilder &arguments(const Arguments &arguments);

	/**
	 * Adds the type described by the given Rtti pointer to the set of supported
	 * types. These "supported types" describe a set of Rtti types that are
	 * reported as supported when including or importing new files while in this
	 * state.
	 *
	 * @param type is the type that should be added to the SupportedTypes list.
	 * @return a reference at this ParserStateBuilder instance for method
	 * chaining.
	 */
	ParserStateBuilder &supportedType(const Rtti *type);

	/**
	 * Adds the type described by the given Rtti pointer to the set of supported
	 * types. These "supported types" describe a set of Rtti types that are
	 * reported as supported when including or importing new files while in this
	 * state.
	 *
	 * @param type is the type that should be added to the SupportedTypes list.
	 * @return a reference at this ParserStateBuilder instance for method
	 * chaining.
	 */
	ParserStateBuilder &supportedTypes(const RttiSet &types);

	/**
	 * Sets the constructor for the element handler. The constructor creates a
	 * new concrete Handler instance for the elements described by this state.
	 * May be nullptr in which case no handler instance is created (this is
	 * the default value).
	 *
	 * @param elementHandler is the HandlerConstructor that should create a
	 * new Handler instance.
	 * @return a reference at this ParserStateBuilder instance for method
	 * chaining.
	 */
	ParserStateBuilder &elementHandler(HandlerConstructor elementHandler);

	/**
	 * Sets the constructor for the child handler. The constructor creates a new
	 * concrete Handler instance for all child elements for which no matching
	 * state is defined. May be nullptr in which case no such elements are
	 * allowed.
	 *
	 * @param childHandler is the HandlerConstructor that should point at the
	 * constructor of the Handler instance for child elements.
	 * @return a reference at this ParserStateBuilder instance for method
	 * chaining.
	 */
	ParserStateBuilder &childHandler(HandlerConstructor childHandler);

	/**
	 * Returns a reference at the internal ParserState instance that was built
	 * using the ParserStateBuilder.
	 *
	 * @return the built ParserState.
	 */
	const ParserState &build() const;
};

/**
 * The ParserStates namespace contains all the global state constants used
 * in the ParserStack class.
 */
namespace ParserStates {
/**
 * State representing all states.
 */
extern const ParserState All;

/**
 * State representing the initial state.
 */
extern const ParserState None;
}

}

#endif /* _OUSIA_PARSER_STATE_HPP_ */

