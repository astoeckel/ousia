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

#ifndef _OUSIA_UTILS_H_
#define _OUSIA_UTILS_H_

#include <sstream>
#include <string>
#include <vector>

namespace ousia {

class Utils {
public:
	/**
	 * Returns true if the given character is in [A-Za-z]
	 */
	static bool isAlphabetic(const char c)
	{
		return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
	}

	/**
	 * Returns true if the given character is in [0-9]
	 */
	static bool isNumeric(const char c) { return (c >= '0') && (c <= '9'); }

	/**
	 * Returns true if the given character is in [0-9A-Fa-f]
	 */
	static bool isHexadecimal(const char c)
	{
		return ((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')) ||
		       ((c >= 'a') && (c <= 'f'));
	}

	/**
	 * Returns true if the given character is in [A-Za-z0-9]
	 */
	static bool isAlphanumeric(const char c)
	{
		return isAlphabetic(c) || isNumeric(c);
	}

	/**
	 * Returns true if the given character is in [A-Za-z_][A-Za-z0-9_-]*
	 */
	static bool isIdentifier(const std::string &name);

	/**
	 * Returns true if the given character is a whitespace character.
	 */
	static bool isWhitespace(const char c)
	{
		return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
	}

	/**
	 * Removes whitespace at the beginning and the end of the given string.
	 */
	static std::string trim(const std::string &s);

	/**
	 * Turns the elements of a collection into a string separated by the
	 * given delimiter.
	 *
	 * @param es is an iterable container of elements that can be appended to an
	 * output stream (the << operator must be implemented).
	 * @param delim is the delimiter that should be used to separate the items.
	 * @param start is a character sequence that should be prepended to the
	 * result.
	 * @param end is a character sequence that should be appended to the result.
	 */
	template <class T>
	static std::string join(T es, const std::string &delim,
	                        const std::string &start = "",
	                        const std::string &end = "")
	{
		std::stringstream res;
		bool first = true;
		res << start;
		for (const auto &e : es) {
			if (!first) {
				res << delim;
			}
			res << e;
			first = false;
		}
		res << end;
		return res.str();
	}

	/**
	 * Splits the given string at the delimiter and returns an array of
	 * substrings without the delimiter.
	 *
	 * @param s is the string that should be splitted.
	 * @param delim is the delimiter at which the string should be splitted.
	 * @return a vector of strings containing the splitted sub-strings.
	 */
	static std::vector<std::string> split(const std::string &s, char delim);

	/**
	 * Converts the given string to lowercase (only works for ANSI characters).
	 *
	 * @param s is the string that should be converted to lowercase.
	 * @return s in lowercase.
	 */
	static std::string toLower(std::string s);

	/**
	 * Reads the file extension of the given filename.
	 *
	 * @param filename is the filename from which the extension should be
	 * extracted.
	 * @return the extension, excluding any leading dot. The extension is
	 * defined as the substring after the last dot in the given string, if the
	 * dot is after a slash or backslash. The extension is converted to
	 * lowercase.
	 */
	static std::string extractFileExtension(const std::string &filename);
};
}

#endif /* _OUSIA_UTILS_H_ */

