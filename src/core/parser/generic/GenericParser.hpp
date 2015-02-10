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
 * @file GenericParser.hpp
 *
 * The GenericParser class builds an abstraction layer that separates the
 * underlying document format (e.g. osdm or osdmx) from the actual process of
 * building the document model. It provides a set of genric functions that
 * should be called by the inheriting concrete parser class, e.g. indicating a
 * command with parameters, the start/end of a field or the start/end of an
 * annotation. The GenericParser maintains an internal stack of
 * ParserStateHandlers and relays the commands to the elements of this stack.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_GENERIC_PARSER_HPP_
#define _OUSIA_GENERIC_PARSER_HPP_

#include <core/parser/Parseer.hpp>

namespace ousia {

class GenericParser : public Parser {



};

}

#endif _OUSIA_GENERIC_PARSER_HPP_

