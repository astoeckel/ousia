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
	 * Returns true if the given character is in [A-Za-z].
	 */
	static bool isIdentifierStartCharacter(const char c)
	{
		return isAlphabetic(c);
	}

	/**
	 * Returns true if the given character is in [A-Za-z0-9_-].
	 */
	static bool isIdentifierCharacter(const char c)
	{
		return isAlphanumeric(c) || (c == '_') || (c == '-');
	}

	/**
	 * Returns true if the given string is in
	 * \code{.txt}
	 * [A-Za-z][A-Za-z0-9_-]*
	 * \endCode
	 *
	 * @param name is the string that should be tested.
	 * @return true if the string matches the regular expression given above,
	 * false otherwise.
	 */
	static bool isIdentifier(const std::string &name);

	/**
	 * Returns true if the given string is an identifier or an empty string.
	 */
	static bool isIdentifierOrEmpty(const std::string &name);

	/**
	 * Returns true if the given string is in
	 * \code{.txt}
	 * ([A-Za-z][A-Za-z0-9_-]*)(:[A-Za-z][A-Za-z0-9_-]*)*
	 * \endCode
	 *
	 * @param name is the string that should be tested.
	 * @return true if the string matches the regular expression given above,
	 * false otherwise.
	 */
	static bool isNamespacedIdentifier(const std::string &name);

	/**
	 * Returns true if the given characters form a valid user-defined token.
	 * This function returns true under the following circumstances:
	 * <ul>
	 *   <li>The given token is not empty</li>
	 *   <li>The given token starts and ends with a non-alphanumeric character
	 *       </li>
	 *   <li>The token is none of the following character sequences (which are
	 *       special in OSML):
	 *      <ul>
	 *        <li>'{', '}' or any combined repetition of these characters</li>
	 *        <li>'\', '{!', '<\', '\>'</li>
	 *        <li>'%', '%{', '}%'</li>
	 *      </ul>
	 *   </li>
	 * </ul>
	 */
	static bool isUserDefinedToken(const std::string &token);

	/**
	 * Returns true if the given character is a linebreak character.
	 */
	static bool isLinebreak(const char c) { return (c == '\n') || (c == '\r'); }

	/**
	 * Returns true if the given character is a whitespace character.
	 */
	static bool isWhitespace(const char c)
	{
		return (c == ' ') || (c == '\t') || isLinebreak(c);
	}

	/**
	 * Returns true if the given string has a non-whitespace character.
	 *
	 * @param s is the string that should be checked.
	 * @return true if the string contains a non-whitespace character.
	 */
	static bool hasNonWhitepaceChar(const std::string &s);

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
		return trim(s, s.size(), f);
	}

	/**
	 * Trims the given string or vector of chars by returning the start and end
	 * index.
	 *
	 * @param s is the container that should be trimmed.
	 * @param len is the number of elements in the container.
	 * @param f is a function that returns true for values at a certain index
	 * that should be removed.
	 * @return start and end index. Note that "end" points at the character
	 * beyond the end, thus "end" minus "start"
	 */
	template <class T, class Filter>
	static std::pair<size_t, size_t> trim(const T &s, size_t len, Filter f)
	{
		size_t start = 0;
		for (size_t i = 0; i < len; i++) {
			if (!f(i)) {
				start = i;
				break;
			}
		}

		size_t end = 0;
		for (ssize_t i = len - 1; i >= static_cast<ssize_t>(start); i--) {
			if (!f(i)) {
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
	 * Trims the given string and returns both the trimmed string and the start
	 * and end location.
	 *
	 * @tparam T is the string type that should be used.
	 * @param s is the container that should be trimmed.
	 * @param len is the number of elements in the container.
	 * @param start is an output parameter which is set to the offset at which
	 * the collapsed version of the string starts.
	 * @param end is an output parameter which is set to the offset at which
	 * the collapsed version of the string ends.
	 * @return start and end index. Note that "end" points at the character
	 * beyond the end, thus "end" minus "start"
	 * @param f is a function that returns true for values at a certain index
	 * that should be removed.
	 */
	template <class T, class Filter>
	static std::string trim(const T &s, size_t len, size_t &start, size_t &end,
	                        Filter f)
	{
		auto res = trim(s, len, f);
		start = res.first;
		end = res.second;
		return std::string(&s[start], end - start);
	}

	/**
	 * Removes whitespace at the beginning and the end of the given string.
	 *
	 * @param s is the string that should be trimmed.
	 * @return a trimmed copy of s.
	 */
	static std::string trim(const std::string &s)
	{
		std::pair<size_t, size_t> bounds =
		    trim(s, [&s](size_t i) { return isWhitespace(s[i]); });
		return s.substr(bounds.first, bounds.second - bounds.first);
	}

	/**
	 * Collapses the whitespaces in the given string (trims the string and
	 * replaces all whitespace characters by a single one).
	 *
	 * @param s is the string in which the whitespace should be collapsed.
	 * @return a copy of s with collapsed whitespace.
	 */
	static std::string collapse(const std::string &s)
	{
		size_t start;
		size_t end;
		return collapse(s, s.size(), start, end,
		                [&s](size_t i) { return isWhitespace(s[i]); });
	}

	/**
	 * Collapses the whitespaces in the given string (trims the string and
	 * replaces all whitespace characters by a single one).
	 *
	 * @param s is the string in which the whitespace should be collapsed.
	 * @param start is an output parameter which is set to the offset at which
	 * the collapsed version of the string starts.
	 * @param end is an output parameter which is set to the offset at which
	 * the collapsed version of the string ends.
	 * @return a copy of s with collapsed whitespace.
	 */
	static std::string collapse(const std::string &s, size_t &start,
	                            size_t &end)
	{
		return collapse(s, s.size(), start, end,
		                [&s](size_t i) { return isWhitespace(s[i]); });
	}

	/**
	 * Collapses the whitespaces in the given string (trims the string and
	 * replaces all whitespace characters by a single one).
	 *
	 * @tparam T is the string type that should be used.
	 * @tparam Filter is a filter function used for detecting the character
	 * indices that might be removed.
	 * @param s is the string in which the whitespace should be collapsed.
	 * @param len is the length of the input string
	 * @param start is an output parameter which is set to the offset at which
	 * the collapsed version of the string starts.
	 * @param end is an output parameter which is set to the offset at which
	 * the collapsed version of the string ends.
	 * @return a copy of s with collapsed whitespace.
	 */
	template <class T, class Filter>
	static std::string collapse(const T &s, size_t len, size_t &start,
	                            size_t &end, Filter f)
	{
		// Result vector
		std::vector<char> res;

		// Initialize the output arguments
		start = 0;
		end = 0;

		// Iterate over the input string and replace all whitespace sequences by
		// a single space
		bool hadWhitespace = false;
		for (size_t i = 0; i < len; i++) {
			const char c = s[i];
			if (f(i)) {
				hadWhitespace = !res.empty();
			} else {
				// Adapt the start and end position
				if (res.empty()) {
					start = i;
				}
				end = i + 1;

				// Insert a space character if there was a whitespace
				if (hadWhitespace) {
					res.push_back(' ');
					hadWhitespace = false;
				}

				// Insert the character
				res.push_back(c);
			}
		}

		// Return the result vector as string
		return std::string(res.data(), res.size());
	}

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

	/**
	 * Checks whether the given string starts with the given prefix.
	 *
	 * @param s is the string.
	 * @param prefix is the string which should be checked for being a prefix of
	 * s.
	 */
	static bool startsWith(const std::string &s, const std::string &prefix);

	/**
	 * Checks whether the given string ends with the given suffix.
	 *
	 * @param s is the string.
	 * @param suffix is the string which should be checked for being a suffix of
	 * s.
	 */
	static bool endsWith(const std::string &s, const std::string &suffix);

	/**
	 * Hash functional to be used for enum classes.
	 * See http://stackoverflow.com/a/24847480/2188211
	 */
	struct EnumHash {
		template <typename T>
		std::size_t operator()(T t) const
		{
			return static_cast<std::size_t>(t);
		}
	};
};
}

#endif /* _OUSIA_UTILS_H_ */
