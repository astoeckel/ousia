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
 * @file WhitespaceHandler.hpp
 *
 * Contains the WhitespaceHandler classes which are used in multiple places to
 * trim, compact or preserve whitespaces while at the same time maintaining the
 * position information associated with the input strings.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_WHITESPACE_HANDLER_HPP_
#define _OUSIA_WHITESPACE_HANDLER_HPP_

#include <string>
#include <vector>

#include "WhitespaceHandler.hpp"

namespace ousia {

/**
 * WhitespaceHandler is a based class that can be used to collect text on a
 * character-by-character basis. Note that this class and its descendants are
 * hoped to be inlined by the compiler (and used in conjunction with templates),
 * thus they are fully defined inside this header.
 */
class WhitespaceHandler {
public:
	/**
	 * Start position of the extracted text.
	 */
	size_t textStart;

	/**
	 * End position of the extracted text.
	 */
	size_t textEnd;

	/**
	 * Buffer containing the extracted text.
	 */
	std::vector<char> textBuf;

	/**
	 * Constructor of the TextHandlerBase base class. Initializes the start and
	 * end position with zeros.
	 */
	WhitespaceHandler() : textStart(0), textEnd(0) {}

	/**
	 * Returns true if this whitespace handler has found any text and a text
	 * token could be emitted.
	 *
	 * @return true if the internal data buffer is non-empty.
	 */
	bool hasText() { return !textBuf.empty(); }

	/**
	 * Returns the content of the WhitespaceHandler as string.
	 */
	std::string toString()
	{
		return std::string(textBuf.data(), textBuf.size());
	}
};

/**
 * The PreservingWhitespaceHandler class preserves all characters unmodified,
 * including whitepace characters.
 */
class PreservingWhitespaceHandler : public WhitespaceHandler {
public:
	/**
	 * Appends the given character to the internal text buffer, does not
	 * eliminate whitespace.
	 *
	 * @param c is the character that should be appended to the internal buffer.
	 * @param start is the start byte offset of the given character.
	 * @param end is the end byte offset of the given character.
	 */
	void append(char c, size_t start, size_t end)
	{
		if (textBuf.empty()) {
			textStart = start;
		}
		textEnd = end;
		textBuf.push_back(c);
	}
};

/**
 * The TrimmingTextHandler class trims all whitespace characters at the begin
 * and the end of a text section but leaves all other characters unmodified,
 * including whitepace characters.
 */
class TrimmingWhitespaceHandler : public WhitespaceHandler {
public:
	/**
	 * Buffer used internally to temporarily store all whitespace characters.
	 * They are only added to the output buffer if another non-whitespace
	 * character is reached.
	 */
	std::vector<char> whitespaceBuf;

	/**
	 * Appends the given character to the internal text buffer, eliminates
	 * whitespace characters at the begin and end of the text.
	 *
	 * @param c is the character that should be appended to the internal buffer.
	 * @param start is the start byte offset of the given character.
	 * @param end is the end byte offset of the given character.
	 */
	void append(char c, size_t start, size_t end)
	{
		// Handle whitespace characters
		if (Utils::isWhitespace(c)) {
			if (!textBuf.empty()) {
				whitespaceBuf.push_back(c);
			}
			return;
		}

		// Set the start and end offset correctly
		if (textBuf.empty()) {
			textStart = start;
		}
		textEnd = end;

		// Store the character
		if (!whitespaceBuf.empty()) {
			textBuf.insert(textBuf.end(), whitespaceBuf.begin(),
			               whitespaceBuf.end());
			whitespaceBuf.clear();
		}
		textBuf.push_back(c);
	}
};

/**
 * The CollapsingTextHandler trims characters at the beginning and end of the
 * text and reduced multiple whitespace characters to a single blank.
 */
class CollapsingWhitespaceHandler : public WhitespaceHandler {
public:
	/**
	 * Flag set to true if a whitespace character was reached.
	 */
	bool hasWhitespace = false;

	/**
	 * Appends the given character to the internal text buffer, eliminates
	 * redundant whitespace characters.
	 *
	 * @param c is the character that should be appended to the internal buffer.
	 * @param start is the start byte offset of the given character.
	 * @param end is the end byte offset of the given character.
	 */
	void append(char c, size_t start, size_t end)
	{
		// Handle whitespace characters
		if (Utils::isWhitespace(c)) {
			if (!textBuf.empty()) {
				hasWhitespace = true;
			}
			return;
		}

		// Set the start and end offset correctly
		if (textBuf.empty()) {
			textStart = start;
		}
		textEnd = end;

		// Store the character
		if (hasWhitespace) {
			textBuf.push_back(' ');
			hasWhitespace = false;
		}
		textBuf.push_back(c);
	}
};

/**
 * Function that can be used to append the given buffer (e.g. a string or a
 * vector) to the whitespace handler.
 *
 * @tparam WhitespaceHandler is one of the WhitespaceHandler classes.
 * @tparam Buffer is an iterable type.
 * @param handler is the handler to which the characters of the Buffer should be
 * appended.
 * @param buf is the buffer from which the characters should be read.
 * @param start is the start byte offset. Each character is counted as one byte.
 */
template <typename WhitespaceHandler, typename Buffer>
inline void appendToWhitespaceHandler(WhitespaceHandler &handler, Buffer buf,
                                      size_t start)
{
	for (auto elem : buf) {
		handler.append(elem, start++);
	}
}
}

#endif /* _OUSIA_WHITESPACE_HANDLER_HPP_ */

