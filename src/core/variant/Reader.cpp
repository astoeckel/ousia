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

#include <cassert>
#include <sstream>

#include <core/Utils.hpp>

#include "Reader.hpp"

namespace ousia {
namespace variant {

static const char *ERR_UNEXPECTED_CHARACTER = "Unexpected character";
static const char *ERR_UNEXPECTED_END = "Unexpected end";
static const char *ERR_UNTERMINATED = "Unterminated literal";
static const char *ERR_INVALID_ESCAPE = "Invalid escape sequence";

static const int STATE_INIT = 0;
static const int STATE_IN_STRING = 1;
static const int STATE_ESCAPE = 2;
static const int STATE_WHITESPACE = 3;

template <class T>
static std::pair<bool, T> error(BufferedCharReader &reader, Logger &logger,
                                const char *err, T res)
{
	logger.errorAt(err, reader);
	return std::make_pair(false, std::move(res));
}

std::pair<bool, std::string> Reader::parseString(
    BufferedCharReader &reader, Logger &logger,
    const std::unordered_set<char> *delims)
{
	// Initialize the internal state
	int state = STATE_INIT;
	char quote = 0;
	std::stringstream res;

	// Consume all whitespace
	reader.consumeWhitespace();

	// Statemachine whic iterates over each character in the stream
	// TODO: Combination of peeking and consumePeek is stupid as consumePeek is
	// the default (read and putBack would obviously be better, yet the latter
	// is not trivial to implement in the current BufferedCharReader).
	char c;
	while (reader.peek(&c)) {
		switch (state) {
			case STATE_INIT:
				if (c == '"' || c == '\'') {
					quote = c;
					state = STATE_IN_STRING;
					break;
				} else if (delims && delims->count(c)) {
					return error(reader, logger, ERR_UNEXPECTED_END, res.str());
				}
				return error(reader, logger, ERR_UNEXPECTED_CHARACTER,
				             res.str());
			case STATE_IN_STRING:
				if (c == quote) {
					reader.consumePeek();
					return std::make_pair(true, res.str());
				} else if (c == '\\') {
					state = STATE_ESCAPE;
					reader.consumePeek();
					break;
				} else if (c == '\n') {
					return error(reader, logger, ERR_UNTERMINATED, res.str());
				}
				res << c;
				reader.consumePeek();
				break;
			case STATE_ESCAPE:
				// Handle all possible special escape characters
				switch (c) {
					case 'b':
						res << '\b';
						break;
					case 'f':
						res << '\f';
						break;
					case 'n':
						res << '\n';
						break;
					case 'r':
						res << '\r';
						break;
					case 't':
						res << '\t';
						break;
					case 'v':
						res << '\v';
						break;
					case '\'':
						res << '\'';
						break;
					case '"':
						res << '"';
						break;
					case '\\':
						res << '\\';
						break;
					case '\n':
						break;
					case 'x':
						// TODO: Parse Latin-1 sequence hex XX
						break;
					case 'u':
						// TODO: Parse 16-Bit unicode character hex XXXX
						break;
					default:
						if (Utils::isNumeric(c)) {
							// TODO: Parse octal 000 sequence
						} else {
							logger.errorAt(ERR_INVALID_ESCAPE, reader);
						}
						break;
				}

				// Switch back to the "normal" state
				state = STATE_IN_STRING;
				reader.consumePeek();
				break;
		}
	}
	return error(reader, logger, ERR_UNEXPECTED_END, res.str());
}

std::pair<bool, std::string> Reader::parseUnescapedString(
    BufferedCharReader &reader, Logger &logger,
    const std::unordered_set<char> &delims)
{
	std::stringstream res;
	std::stringstream buf;
	char c;

	// Consume all whitespace
	reader.consumeWhitespace();

	// Copy all characters, skip whitespace at the end
	int state = STATE_IN_STRING;
	while (reader.peek(&c)) {
		if (delims.count(c)) {
			reader.resetPeek();
			return std::make_pair(true, res.str());
		} else if (Utils::isWhitespace(c)) {
			// Do not add whitespace to the output buffer
			state = STATE_WHITESPACE;
			buf << c;
		} else {
			// If we just hat a sequence of whitespace, append it to the output
			// buffer and continue
			if (state == STATE_WHITESPACE) {
				res << buf.str();
				buf.str(std::string{});
				buf.clear();
				state = STATE_IN_STRING;
			}
			res << c;
		}
		reader.consumePeek();
	}
	return std::make_pair(true, res.str());
}

std::pair<bool, Variant> Reader::parseGeneric(
    BufferedCharReader &reader, Logger &logger,
    const std::unordered_set<char> &delims)
{
	char c;

	// Skip all whitespace characters
	reader.consumeWhitespace();

	while (reader.peek(&c)) {
		// Stop if a delimiter is reached
		if (delims.count(c)) {
			return error(reader, logger, ERR_UNEXPECTED_END, nullptr);
		}

		// Parse a string if a quote is reached
		if (c == '"' || c == '\'') {
			auto res = parseString(reader, logger);
			return std::make_pair(res.first, res.second.c_str());
		}

		if (c == '[') {
			// TODO: Parse struct descriptor
		}

		if (Utils::isNumeric(c)) {
			// TODO: Parse integer/double
		}

		// Parse an unescaped string in any other case
		auto res = parseUnescapedString(reader, logger, delims);
		return std::make_pair(res.first, res.second.c_str());
	}
	return error(reader, logger, ERR_UNEXPECTED_END, nullptr);
}
}
}

