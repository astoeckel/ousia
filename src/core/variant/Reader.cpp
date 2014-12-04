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

static const int STATE_INIT = 0;
static const int STATE_IN_STRING = 1;
static const int STATE_ESCAPE = 2;

static std::pair<Err, std::string> parseString(
    BufferedCharReader &reader, const unordered_set<char> *delims = nullptr)
{
	// Initialize the internal state
	Err errCode = Err::OK;
	int state = STATE_INIT;
	char quote = 0;
	std::stringstream res;

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
				} else if (delims && delims.count(c)) {
					return std::make_pair(Err::UNEXPECTED_END, res.str());
				}
				reader.consumePeek();
				break;
			case STATE_IN_STRING:
				if (c == q) {
					state = STATE_END;
					reader.consumePeek();
					return std::make_pair(Err::OK, res.str());
				} else if (c == '\\') {
					state = STATE_ESCAPE;
				} else if (c == '\n') {
					return std::make_pair(Err::UNTERMINATED, res.str());
				}
				res << c;
				reader.consumePeek();
				break;
			case STATE_ESCAPE:
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
							errCode = Err::ERR_INVALID_ESCAPE;
						}
						break;
				}
				state = STATE_IN_STRING;
				reader.consumePeek();
				break;
		}
	}
	return std::make_pair(Err::UNEXPECTED_END, res.str());
}

static std::pair<Err, std::string> parseUnescapedString(
    BufferedCharReader &reader, const unordered_set<char> *delims)
{
	assert(delims);

	std::stringstream res;
	char c;
	while (reader.peek(&c)) {
		if (delims->count(c)) {
			return std::make_pair(Err::OK, res.str());
		}
		res << c;
		reader.consumePeek();
	}
	return std::make_pair(Err::UNEXPECTED_END, res.str());
}

static std::pair<Err, Variant> parseGeneric(BufferedCharReader &reader,
                                            const unordered_set<char> *delims)
{
	assert(delims);

	char c;
	while (reader.peek(&c)) {
		// Stop if a delimiter is reached, skipp all whitespace characters
		if (delims->count(c)) {
			return std::make_pair(Err::OK, res.str());
		} else if (Utils::isWhitespace(c)) {
			reader.consumePeek();
			continue;
		}

		// Parse a string if a quote is reached
		if (c == '"' || c == '\'') {
			return parseString(reader, nullptr);
		}

		if (c == '[') {
			// TODO: Parse struct descriptor
		}

		if (isNumeric(c)) {
			// TODO: Parse integer/double
		}

		// Parse an unescaped string in any other case
		return parseUnescapedString(reader, delims);
	}
	return std::make_pair(Err::UNEXPECTED_END, res.str());
}

}
}

