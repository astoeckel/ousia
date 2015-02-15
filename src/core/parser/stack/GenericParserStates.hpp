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
 * @file GenericParserStates.hpp
 *
 * Contains a multimap which maps between tag/command names to the corresponding
 * state descriptors. This multimap is used to initialize the push down
 * automaton residing inside the "Stack" class.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_GENERIC_PARSER_STATES_HPP_
#define _OUSIA_PARSER_STACK_GENERIC_PARSER_STATES_HPP_

#include <string>
#include <map>

namespace ousia {
namespace parser_stack {

// Forward declarations
class State;

/**
 * Map between tagnames and references to the corresponding State instances.
 */
extern const std::multimap<std::string, const State *> GenericParserStates;
}
}

#endif /* _OUSIA_PARSER_STACK_GENERIC_PARSER_STATES_HPP_ */

