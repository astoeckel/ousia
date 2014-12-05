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

#include <iostream>

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
static const char *ERR_INVALID_INTEGER = "Sequence is not a valid integer";
static const char *ERR_INVALID_DOUBLE = "Sequence is not a valid number";

static const int STATE_INIT = 0;
static const int STATE_IN_STRING = 1;
static const int STATE_ESCAPE = 2;
static const int STATE_WHITESPACE = 3;

/**
 * Used internally to extract the regexp [0-9xXe.-]* from the given buffered
 * char reader.
 *
 * @param reader is the buffered char reader from which the sequence should be
 * extracted.
 */
static std::pair<bool, std::string> extractNumberSequence(
    BufferedCharReader &reader)
{
	bool isInteger = true;
	char c;
	std::stringstream res;
	while (reader.peek(&c)) {
		isInteger = isInteger && !(c == '.' || c == 'e' || c == 'E');
		if (Utils::isHexadecimal(c) || c == '.' || c == '-' || c == 'e' ||
		    c == 'E' || c == 'x' || c == 'X') {
			reader.consumePeek();
			res << c;
		} else {
			reader.resetPeek();
			break;
		}
	}
	return std::make_pair(isInteger, res.str());
}

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

static std::pair<bool, int64_t> parseExtractedInteger(
    BufferedCharReader &reader, Logger &logger, const std::string &val)
{
	try {
		size_t idx = 0;
		bool valid = false;
		int64_t res = 0;

		std::string prefix = val.substr(0, 2);
		if (prefix == "0x" || prefix == "0X") {
			// If the value starts with 0x or 0X parse a hexadecimal value
			std::string hex = val.substr(2);
			res = std::stoll(hex, &idx, 16);
			valid = idx == hex.length();
		} else {
			res = std::stoll(val, &idx, 10);
			valid = idx == val.length();
		}

		if (!valid) {
			return error(reader, logger, ERR_INVALID_INTEGER, 0L);
		}

		return std::make_pair(valid, res);
	}
	catch (std::invalid_argument ex) {
		return error(reader, logger, ERR_INVALID_INTEGER, 0L);
	}
}

static std::pair<bool, double> parseExtractedDouble(BufferedCharReader &reader,
                                                    Logger &logger,
                                                    const std::string &val)
{
	try {
		size_t idx = 0;
		double res = std::stod(val, &idx);
		if (idx != val.length()) {
			return error(reader, logger, ERR_INVALID_DOUBLE, 0.0);
		}
		return std::make_pair(true, res);
	}
	catch (std::invalid_argument ex) {
		return error(reader, logger, ERR_INVALID_DOUBLE, 0.0);
	}
}

std::pair<bool, int64_t> Reader::parseInteger(BufferedCharReader &reader,
                                              Logger &logger)
{
	// Skip all whitespace characters
	reader.consumeWhitespace();

	// Extract a number sequence, make sure it is an integer
	auto num = extractNumberSequence(reader);
	if (!num.first || num.second.empty()) {
		return error(reader, logger, ERR_INVALID_INTEGER, 0L);
	}

	return parseExtractedInteger(reader, logger, num.second);
}

std::pair<bool, double> Reader::parseDouble(BufferedCharReader &reader,
                                            Logger &logger)
{
	// Skip all whitespace characters
	reader.consumeWhitespace();

	// Extract a number sequence, parse it as double
	auto num = extractNumberSequence(reader);
	return parseExtractedDouble(reader, logger, num.second);
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
			reader.resetPeek();
			auto num = extractNumberSequence(reader);
			if (num.first) {
				auto res = parseExtractedInteger(reader, logger, num.second);
				return std::make_pair(
				    res.first,
				    Variant{static_cast<Variant::intType>(res.second)});
			} else {
				return parseExtractedDouble(reader, logger, num.second);
			}
		}

		// Parse an unescaped string in any other case
		auto res = parseUnescapedString(reader, logger, delims);
		return std::make_pair(res.first, res.second.c_str());
	}
	return error(reader, logger, ERR_UNEXPECTED_END, nullptr);
}
}
}

