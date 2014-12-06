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

#include <cmath>
#include <sstream>

#include <core/Utils.hpp>

#include "Reader.hpp"

namespace ousia {
namespace variant {

/* Error Messages */

static const char *ERR_UNEXPECTED_CHAR = "Unexpected character";
static const char *ERR_UNEXPECTED_END = "Unexpected literal end";
static const char *ERR_UNTERMINATED = "Unterminated literal";
static const char *ERR_INVALID_ESCAPE = "Invalid escape sequence";
static const char *ERR_INVALID_INTEGER = "Invalid integer value";
static const char *ERR_TOO_LARGE = "Value too large to represent";

/* Class Number */

/**
 * Class used internally to represent a number (integer or double). The number
 * is represented by its components (base value a, nominator n, denominator d,
 * exponent e, sign s and exponent sign sE).
 */
class Number {
private:
	/**
	 * Reprsents the part of the number: Base value a, nominator n, exponent e.
	 */
	enum class Part { A, N, E };

	/**
	 * State used in the parser state machine
	 */
	enum class State {
		INIT,
		HAS_MINUS,
		LEADING_ZERO,
		LEADING_POINT,
		INT,
		HEX,
		POINT,
		EXP_INIT,
		EXP_HAS_MINUS,
		EXP
	};

	/**
	 * Returns the numeric value of the given ASCII character (returns 0 for
	 * '0', 1 for '1', 10 for 'A' and so on).
	 *
	 * @param c is the character for which the numeric value should be returned.
	 * @return the numeric value the character represents.
	 */
	static int charValue(char c)
	{
		if (c >= '0' && c <= '9') {
			return c & 0x0F;
		}
		if ((c >= 'A' && c <= 'O') || (c >= 'a' && c <= 'o')) {
			return (c & 0x0F) + 9;
		}
		return -1;
	}

	/**
	 * Appends the value of the character c to the internal number
	 * representation and reports any errors that might occur.
	 */
	bool appendChar(char c, int base, Part p, BufferedCharReader &reader,
	                Logger &logger)
	{
		// Check whether the given character is valid
		int v = charValue(c);
		if (v < 0 || v >= base) {
			logger.errorAt(ERR_UNEXPECTED_CHAR, reader);
			return false;
		}

		// Append the number to the specified part
		switch (p) {
			case Part::A:
				a = a * base + v;
				break;
			case Part::N:
				n = n * base + v;
				d = d * base;
				break;
			case Part::E:
				e = e * base + v;
				break;
		}

		// Check for any overflows
		if (a < 0 || n < 0 || d < 0 || e < 0) {
			logger.errorAt(ERR_TOO_LARGE, reader);
			return false;
		}
		return true;
	}

public:
	/**
	 * Sign and exponent sign.
	 */
	int8_t s, sE;

	/**
	 * Exponent
	 */
	int16_t e;

	/**
	 * Base value, nominator, denominator
	 */
	int64_t a, n, d;

	/**
	 * Constructor of the number class.
	 */
	Number() : s(1), sE(1), e(0), a(0), n(0), d(1) {}

	/**
	 * Returns the represented double value.
	 */
	double doubleValue()
	{
		return s * (a + ((double)n / (double)d)) * pow(10.0, (double)(sE * e));
	}

	/**
	 * Returns the represented integer value. Only a lossless operation, if the
	 * number is an integer (as can be checked via the isInt method), otherwise
	 * the exponent and the fractional value will be truncated.
	 */
	int64_t intValue() { return s * a; }

	/**
	 * Returns true, if the number is an integer (has no fractional or
	 * exponential part).
	 */
	bool isInt() { return (n == 0) && (d == 1) && (e == 0); }

	/**
	 * Tries to parse the number from the given stream and loggs any errors to
	 * the given logger instance. Numbers are terminated by one of the given
	 * delimiters.
	 */
	bool parse(BufferedCharReader &reader, Logger &logger,
	           const std::unordered_set<char> &delims)
	{
		State state = State::INIT;
		char c;

		// Consume the first whitespace characters
		reader.consumeWhitespace();

		// Iterate over the FSM to extract numbers
		while (reader.peek(&c)) {
			// Abort, once a delimiter or whitespace is reached
			if (Utils::isWhitespace(c) || delims.count(c)) {
				reader.resetPeek();
				break;
			}

			// The character is not a whitespace character and not a delimiter
			switch (state) {
				case State::INIT:
				case State::HAS_MINUS:
					switch (c) {
						case '-':
							// Do not allow multiple minus signs
							if (state == State::HAS_MINUS) {
								logger.errorAt(ERR_UNEXPECTED_CHAR, reader);
								return false;
							}
							state = State::HAS_MINUS;
							s = -1;
							break;
						case '0':
							// Remember a leading zero for the detection of "0x"
							state = State::LEADING_ZERO;
							break;
						case '.':
							// Remember a leading point as ".eXXX" is invalid
							state = State::LEADING_POINT;
							break;
						default:
							state = State::INT;
							if (!appendChar(c, 10, Part::A, reader, logger)) {
								return false;
							}
							break;
					}
					break;
				case State::LEADING_ZERO:
					if (c == 'x' || c == 'X') {
						state = State::HEX;
						break;
					}
				// fallthrough
				case State::INT:
					switch (c) {
						case '.':
							state = State::POINT;
							break;
						case 'e':
						case 'E':
							state = State::EXP_INIT;
							break;
						default:
							state = State::INT;
							if (!appendChar(c, 10, Part::A, reader, logger)) {
								return false;
							}
							break;
					}
					break;
				case State::HEX:
					if (!appendChar(c, 16, Part::A, reader, logger)) {
						return false;
					}
					break;
				case State::LEADING_POINT:
				case State::POINT:
					switch (c) {
						case 'e':
						case 'E':
							if (state == State::LEADING_POINT) {
								logger.errorAt(ERR_UNEXPECTED_CHAR, reader);
								return false;
							}
							state = State::EXP_INIT;
							break;
						default:
							state = State::POINT;
							if (!appendChar(c, 10, Part::N, reader, logger)) {
								return false;
							}
							break;
					}
					break;
				case State::EXP_HAS_MINUS:
				case State::EXP_INIT:
					if (c == '-') {
						if (state == State::EXP_HAS_MINUS) {
							logger.errorAt(ERR_UNEXPECTED_CHAR, reader);
							return false;
						}
						state = State::EXP_HAS_MINUS;
						sE = -1;
					} else {
						state = State::EXP;
						if (!appendChar(c, 10, Part::E, reader, logger)) {
							return false;
						}
					}
					break;
				case State::EXP:
					if (!appendChar(c, 10, Part::E, reader, logger)) {
						return false;
					}
					break;
			}
			reader.consumePeek();
		}

		// States in which ending is valid, in other states, log an error
		if (state == State::LEADING_ZERO || state == State::HEX ||
		    state == State::INT || state == State::POINT ||
		    state == State::EXP) {
			return true;
		}
		logger.errorAt(ERR_UNEXPECTED_END, reader);
		return false;
	}
};

/* Class Reader */

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
				return error(reader, logger, ERR_UNEXPECTED_CHAR, res.str());
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

std::pair<bool, int64_t> Reader::parseInteger(
    BufferedCharReader &reader, Logger &logger,
    const std::unordered_set<char> &delims)
{
	Number n;
	if (n.parse(reader, logger, delims)) {
		// Only succeed if the parsed number is an integer, otherwise this is an
		// error
		if (n.isInt()) {
			return std::make_pair(true, n.intValue());
		} else {
			return error(reader, logger, ERR_INVALID_INTEGER, n.intValue());
		}
	}
	return std::make_pair(false, n.intValue());
}

std::pair<bool, double> Reader::parseDouble(
    BufferedCharReader &reader, Logger &logger,
    const std::unordered_set<char> &delims)
{
	Number n;
	bool res = n.parse(reader, logger, delims);
	return std::make_pair(res, n.doubleValue());
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

		// Try to parse a number if a character in [0-9-] is reached
		if (Utils::isNumeric(c) || c == '-') {
			reader.resetPeek();
			Number n;
			if (n.parse(reader, logger, delims)) {
				if (n.isInt()) {
					return std::make_pair(
					    true,
					    Variant{static_cast<Variant::intType>(n.intValue())});
				} else {
					return std::make_pair(true, n.doubleValue());
				}
			} else {
				return std::make_pair(false, n.doubleValue());
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

