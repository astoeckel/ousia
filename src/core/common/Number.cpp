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

#include <cmath>
#include <sstream>

#include "CharReader.hpp"
#include "Logger.hpp"
#include "Number.hpp"
#include "Utils.hpp"

namespace ousia {

// TODO: Invent common system for error messages which allows localization
// TODO: Possibly adapt the clang error logging system

static const char *ERR_UNEXPECTED_CHAR = "Unexpected character";
static const char *ERR_UNEXPECTED_END = "Unexpected end of number literal";
static const char *ERR_TOO_LARGE = "Value too large to represent";

static std::string unexpectedMsg(const char *expected, const char got)
{
	std::stringstream ss;
	ss << ERR_UNEXPECTED_CHAR << ": Expected " << expected << " but got \'"
	   << got << "\'";
	return ss.str();
}

/* Class Number */

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

double Number::doubleValue()
{
	return s * (a + ((double)n / (double)d)) * pow(10.0, (double)(sE * e));
}

int64_t Number::intValue() { return s * a; }

bool Number::appendChar(char c, int base, Part p, CharReader &reader,
                Logger &logger)
{
	// Check whether the given character is valid
	int v = charValue(c);
	if (v < 0 || v >= base) {
		logger.error(unexpectedMsg("digit", c), reader);
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
		logger.error(ERR_TOO_LARGE, reader);
		return false;
	}
	return true;
}

bool Number::parse(CharReader &reader, Logger &logger,
                   const std::unordered_set<char> &delims)
{
	State state = State::INIT;
	char c;

	// Consume the first whitespace characters
	reader.consumeWhitespace();

	// Iterate over the FSM to extract numbers
	while (reader.peek(c)) {
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
							logger.error(unexpectedMsg("digit", c), reader);
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
						validInteger = false;
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
						validInteger = false;
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
							logger.error(unexpectedMsg("digit", c), reader);
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
						logger.error(unexpectedMsg("digit", c), reader);
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

	// States in which ending is valid. Log an error in other states
	if (state == State::LEADING_ZERO || state == State::HEX ||
	    state == State::INT || state == State::POINT || state == State::EXP) {
		return true;
	}
	logger.error(ERR_UNEXPECTED_END, reader);
	return false;
}

bool Number::parse(const std::string &str, Logger &logger)
{
	// Create a char reader instance with the given string and call the actual
	// parse function
	CharReader reader(str);
	return parse(reader, logger);
}

bool Number::parseFixedLenInt(CharReader &reader, int len, int base, Logger &logger)
{
	char c;
	reader.consumePeek();
	for (int i = 0; i < len; i++) {
		if (!reader.peek(c)) {
			logger.error("Unexpected end of escape sequence", reader);
			return false;
		}
		if (!appendChar(c, base, Number::Part::A, reader, logger)) {
			return false;
		}
		reader.consumePeek();
	}
	return true;
}

}

