/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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
 * @file Whitespace.hpp
 *
 * Contains the WhitespaceMode enum used in various places.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_WHITESPACE_HPP_
#define _OUSIA_WHITESPACE_HPP_

#include <string>
#include <utility>

namespace ousia {

/**
 * Enum specifying the whitespace handling mode of the tokenizer and the
 * parsers.
 */
enum class WhitespaceMode {
	/**
     * Preserves all whitespaces as they are found in the source file.
     */
	PRESERVE,

	/**
     * Trims whitespace at the beginning and the end of the found text.
     */
	TRIM,

	/**
     * Whitespaces are trimmed and collapsed, multiple whitespace characters
     * are replaced by a single space character.
     */
	COLLAPSE
};

}

#endif /* _OUSIA_WHITESPACE_HPP_ */

