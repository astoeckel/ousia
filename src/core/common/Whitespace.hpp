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
 * Contains the WhitespaceMode enum used in various places, as well es functions
 * for trimming and collapsing whitespaces.
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

/**
 * Collection of functions for trimming or collapsing whitespace.
 */
class Whitespace {
	/**
	 * Removes whitespace at the beginning and the end of the given string.
	 *
	 * @param s is the string that should be trimmed.
	 * @return a trimmed copy of s.
	 */
	static std::string trim(const std::string &s);

	/**
	 * Trims the given string or vector of chars by returning the start and end
	 * index.
	 *
	 * @param s is the container that should be trimmed.
	 * @param f is a function that returns true for values that should be
	 * removed.
	 * @return start and end index. Note that "end" points at the character
	 * beyond the end, thus "end" minus "start"
	 */
	template <class T, class Filter>
	static std::pair<size_t, size_t> trim(const T &s, Filter f)
	{
		size_t start = 0;
		for (size_t i = 0; i < s.size(); i++) {
			if (!f(s[i])) {
				start = i;
				break;
			}
		}

		size_t end = 0;
		for (ssize_t i = s.size() - 1; i >= static_cast<ssize_t>(start); i--) {
			if (!f(s[i])) {
				end = i + 1;
				break;
			}
		}

		if (end < start) {
			start = 0;
			end = 0;
		}

		return std::pair<size_t, size_t>{start, end};
	}

	/**
	 * Collapses the whitespaces in the given string (trims the string and
	 * replaces all whitespace characters by a single one).
	 *
	 * @param s is the string in which the whitespace should be collapsed.
	 * @return a copy of s with collapsed whitespace.
	 */
	static std::string collapse(const std::string &s);
};

}

#endif /* _OUSIA_WHITESPACE_HPP_ */

