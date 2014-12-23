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

#include <utf8.h>

#include "VariantReader.hpp"
#include "Utils.hpp"

namespace ousia {

// TODO: Use custom return value instead of std::pair

/* Error Messages */

static const char *ERR_UNEXPECTED_CHAR = "Unexpected character";
static const char *ERR_UNEXPECTED_END = "Unexpected end of literal";
static const char *ERR_UNTERMINATED = "Unterminated literal";
static const char *ERR_INVALID_ESCAPE = "Invalid escape sequence";
static const char *ERR_INVALID_INTEGER = "Invalid integer value";
static const char *ERR_TOO_LARGE = "Value too large to represent";

template <class T>
static std::pair<bool, T> error(CharReader &reader, Logger &logger,
                                const std::string &err, T res)
{
	logger.error(err, reader);
	return std::make_pair(false, std::move(res));
}

static std::string unexpectedMsg(const char *expected, const char got)
{
	std::stringstream ss;
	ss << ERR_UNEXPECTED_CHAR << ": Expected " << expected << " but got \'"
	   << got << "\'";
	return ss.str();
}

static std::string invalidMsg(const char *invalidType,
                              const std::string &invalidValue)
{
	std::stringstream ss;
	ss << "Invalid " << invalidType << " \"" << invalidValue << "\"";
	return ss.str();
}

template <class T>
static std::pair<bool, T> unexpected(CharReader &reader, Logger &logger,
                                     const char *expected, const char got,
                                     T res)
{
	return error(reader, logger, unexpectedMsg(expected, got), res);
}

/* Class Number */

/**
 * Class used internally to represent a number (integer or double). The number
 * is represented by its components (base value a, nominator n, denominator d,
 * exponent e, sign s and exponent sign sE).
 */
class Number {
private:
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

public:
	/**
	 * Reprsents the part of the number: Base value a, nominator n, exponent e.
	 */
	enum class Part { A, N, E };

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
	 * Appends the value of the character c to the internal number
	 * representation and reports any errors that might occur.
	 */
	bool appendChar(char c, int base, Part p, CharReader &reader,
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

	/**
	 * Tries to parse the number from the given stream and loggs any errors to
	 * the given logger instance. Numbers are terminated by one of the given
	 * delimiters.
	 */
	bool parse(CharReader &reader, Logger &logger,
	           const std::unordered_set<char> &delims);

	bool parseFixedLenInt(CharReader &reader, Logger &logger, int base,
	                      int len);
};

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

bool Number::parseFixedLenInt(CharReader &reader, Logger &logger, int base,
                              int len)
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

/* State machine states */

static const int STATE_INIT = 0;
static const int STATE_IN_STRING = 1;
static const int STATE_IN_COMPLEX = 2;
static const int STATE_ESCAPE = 4;
static const int STATE_WHITESPACE = 5;
static const int STATE_RESYNC = 6;
static const int STATE_EXPECT_COMMA = 7;
static const int STATE_HAS_KEY = 8;

/* Helper function for parsing arrays or objects */

// TODO: Refactor this to own class

enum class ComplexMode { ARRAY, OBJECT, BOTH };

static std::string idxKey(size_t idx)
{
	return std::string{"#"} + std::to_string(idx);
}

static Variant parseComplexResult(Variant::mapType &objectResult,
                                  Variant::arrayType &arrayResult, bool isArray,
                                  ComplexMode mode)
{
	// If the result is an array, simply return an array variant
	if (isArray && mode != ComplexMode::OBJECT) {
		return Variant{arrayResult};
	}

	// Otherwise add missing array keys to the resulting map
	for (size_t i = 0; i < arrayResult.size(); i++) {
		objectResult.insert(std::make_pair(idxKey(i), arrayResult[i]));
	}
	return Variant{objectResult};
}

static std::pair<bool, Variant> parseComplex(CharReader &reader, Logger &logger,
                                             char delim, ComplexMode mode)
{
	// Result for either objects or arrays
	Variant::mapType objectResult;
	Variant::arrayType arrayResult;

	// Auxiliary state variables
	bool hadError = false;
	bool isArray = true;

	// Determine the start state and set the actual delimiter
	int state = delim ? STATE_IN_COMPLEX : STATE_INIT;
	delim = delim ? delim : ']';

	// Current array element index
	size_t idx = 0;

	// Current key value
	Variant key;

	// Consume all whitespace
	reader.consumeWhitespace();

	// Iterate over the characters, use the parseGeneric function to read the
	// pairs
	char c;
	while (reader.peek(c)) {
		// Generically handle the end of the array
		if (state != STATE_INIT && c == delim) {
			reader.consumePeek();

			// Add final keys to the result
			if (state == STATE_HAS_KEY) {
				if (isArray) {
					arrayResult.push_back(key);
				} else {
					objectResult.insert(std::make_pair(idxKey(idx), key));
				}
				key = nullptr;
			}

			return std::make_pair(
			    !hadError,
			    parseComplexResult(objectResult, arrayResult, isArray, mode));
		} else if (Utils::isWhitespace(c)) {
			reader.consumePeek();
			continue;
		}

		switch (state) {
			case STATE_INIT:
				if (c != '[') {
					return error(reader, logger, ERR_UNEXPECTED_CHAR,
					             parseComplexResult(objectResult, arrayResult,
					                                isArray, mode));
				}
				state = STATE_IN_COMPLEX;
				reader.consumePeek();
				break;
			case STATE_IN_COMPLEX: {
				// Try to read an element using the parseGeneric function
				reader.resetPeek();
				auto elem = VariantReader::parseGenericToken(
				    reader, logger, {',', '=', delim}, true);

				// If the reader had no error, expect an comma, otherwise skip
				// to the next comma in the stream
				if (elem.first) {
					key = elem.second;
					state = STATE_HAS_KEY;
				} else {
					state = STATE_RESYNC;
					hadError = true;
				}
				break;
			}
			case STATE_HAS_KEY: {
				// When finding an equals sign, read the value corresponding to
				// the key
				if (c == '=') {
					// Abort if only arrays are allowed
					if (mode == ComplexMode::ARRAY) {
						logger.error(unexpectedMsg("\",\"", c), reader);
						hadError = true;
						state = STATE_RESYNC;
						break;
					}

					// Make sure the key is a valid identifier, if not, issue
					// an error
					std::string keyString = key.toString();
					if (!Utils::isIdentifier(keyString)) {
						logger.error(invalidMsg("identifier", keyString),
						             reader);
						hadError = true;
					}

					// This no longer is an array
					isArray = false;

					// Consume the equals sign and parse the value
					reader.consumePeek();
					auto elem = VariantReader::parseGenericToken(
					    reader, logger, {',', delim}, true);
					if (elem.first) {
						objectResult.insert(
						    std::make_pair(keyString, elem.second));
						idx++;
					} else {
						state = STATE_RESYNC;
						hadError = true;
						key = nullptr;
						break;
					}
					state = STATE_EXPECT_COMMA;
				} else if (c == ',') {
					// Simply add the previously read value to the result
					// array or the result object
					if (isArray) {
						arrayResult.push_back(key);
					} else {
						objectResult.insert(std::make_pair(idxKey(idx), key));
					}
					idx++;
					state = STATE_IN_COMPLEX;
					reader.consumePeek();
				} else {
					if (mode == ComplexMode::ARRAY) {
						logger.error(unexpectedMsg("\",\"", c), reader);
					} else {
						logger.error(unexpectedMsg("\",\" or \"=\"", c),
						             reader);
					}
					state = STATE_RESYNC;
					hadError = true;
				}
				key = nullptr;
				break;
			}
			case STATE_EXPECT_COMMA:
				if (c == ',') {
					state = STATE_IN_COMPLEX;
				} else {
					logger.error(unexpectedMsg("\",\"", c), reader);
					state = STATE_RESYNC;
					hadError = true;
				}
				reader.consumePeek();
				break;
			case STATE_RESYNC:
				// Just wait for another comma to arrive
				if (c == ',') {
					state = STATE_IN_COMPLEX;
				}
				reader.consumePeek();
				break;
		}
	}
	return error(reader, logger, ERR_UNEXPECTED_END,
	             parseComplexResult(objectResult, arrayResult, isArray, mode));
}

/* Class Reader */

static bool encodeUtf8(std::stringstream &res, CharReader &reader,
                       Logger &logger, int64_t v, bool latin1)
{
	// Encode the unicode codepoint as UTF-8
	uint32_t cp = static_cast<uint32_t>(v);
	if (latin1 && cp > 0xFF) {
		logger.error("Not a valid ISO-8859-1 (Latin-1) character, skipping",
		             reader);
		return false;
	}

	// Append the code point to the output stream
	try {
		utf8::append(cp, std::ostream_iterator<uint8_t>{res});
		return true;
	}
	catch (utf8::invalid_code_point ex) {
		logger.error("Invalid Unicode codepoint, skipping", reader);
	}
	return false;
}

std::pair<bool, std::string> VariantReader::parseString(
    CharReader &reader, Logger &logger, const std::unordered_set<char> *delims)
{
	// Initialize the internal state
	bool hadError = false;
	int state = STATE_INIT;
	char quote = 0;
	std::stringstream res;

	// Consume all whitespace
	reader.consumeWhitespace();

	// Statemachine whic iterates over each character in the stream
	// TODO: Combination of peeking and consumePeek is stupid as consumePeek is
	// the default (read and putBack would obviously be better, yet the latter
	// is not trivial to implement in the current CharReader).
	char c;
	while (reader.peek(c)) {
		switch (state) {
			case STATE_INIT:
				if (c == '"' || c == '\'') {
					quote = c;
					state = STATE_IN_STRING;
					break;
				} else if (delims && delims->count(c)) {
					return error(reader, logger, ERR_UNEXPECTED_END, res.str());
				}
				return unexpected(reader, logger, "\" or \'", c, res.str());
			case STATE_IN_STRING:
				if (c == quote) {
					reader.consumePeek();
					return std::make_pair(!hadError, res.str());
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
					case 'x': {
						// Parse Latin-1 sequence \xXX
						Number n;
						hadError =
						    !(n.parseFixedLenInt(reader, logger, 16, 2) &&
						      encodeUtf8(res, reader, logger, n.intValue(),
						                 true)) ||
						    hadError;
						break;
					}
					case 'u': {
						// Parse Unicode sequence \uXXXX
						Number n;
						hadError =
						    !(n.parseFixedLenInt(reader, logger, 16, 4) &&
						      encodeUtf8(res, reader, logger, n.intValue(),
						                 false)) ||
						    hadError;
						break;
					}
					default:
						if (Utils::isNumeric(c)) {
							// Parse Latin-1 sequence \000
							reader.resetPeek();
							Number n;
							hadError =
							    !(n.parseFixedLenInt(reader, logger, 8, 3) &&
							      encodeUtf8(res, reader, logger, n.intValue(),
							                 true)) ||
							    hadError;
						} else {
							logger.error(ERR_INVALID_ESCAPE, reader);
							hadError = true;
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

std::pair<bool, std::string> VariantReader::parseToken(
    CharReader &reader, Logger &logger, const std::unordered_set<char> &delims)
{
	std::stringstream res;
	char c;

	// Consume all whitespace
	reader.consumeWhitespace();

	// Copy all characters, skip whitespace at the end
	int state = STATE_WHITESPACE;
	while (reader.peek(c)) {
		bool whitespace = Utils::isWhitespace(c);
		if (delims.count(c) || (state == STATE_IN_STRING && whitespace)) {
			reader.resetPeek();
			return std::make_pair(state == STATE_IN_STRING, res.str());
		} else if (!whitespace) {
			state = STATE_IN_STRING;
			res << c;
		}
		reader.consumePeek();
	}
	return std::make_pair(state == STATE_IN_STRING, res.str());
}

std::pair<bool, std::string> VariantReader::parseUnescapedString(
    CharReader &reader, Logger &logger, const std::unordered_set<char> &delims)
{
	std::stringstream res;
	std::stringstream buf;
	char c;

	// Consume all whitespace
	reader.consumeWhitespace();

	// Copy all characters, skip whitespace at the end
	int state = STATE_IN_STRING;
	while (reader.peek(c)) {
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

std::pair<bool, int64_t> VariantReader::parseInteger(
    CharReader &reader, Logger &logger, const std::unordered_set<char> &delims)
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

std::pair<bool, double> VariantReader::parseDouble(
    CharReader &reader, Logger &logger, const std::unordered_set<char> &delims)
{
	Number n;
	bool res = n.parse(reader, logger, delims);
	return std::make_pair(res, n.doubleValue());
}

std::pair<bool, Variant::arrayType> VariantReader::parseArray(
    CharReader &reader, Logger &logger, char delim)
{
	auto res = parseComplex(reader, logger, delim, ComplexMode::ARRAY);
	return std::make_pair(res.first, res.second.asArray());
}

std::pair<bool, Variant::mapType> VariantReader::parseObject(CharReader &reader,
                                                             Logger &logger,
                                                             char delim)
{
	auto res = parseComplex(reader, logger, delim, ComplexMode::OBJECT);
	return std::make_pair(res.first, res.second.asMap());
}

std::pair<bool, Variant> VariantReader::parseGeneric(
    CharReader &reader, Logger &logger, const std::unordered_set<char> &delims)
{
	Variant::arrayType arr;
	char c;
	bool hadError = false;

	// Parse generic tokens until the end of the stream or the delimiter is
	// reached
	while (reader.peek(c) && !delims.count(c)) {
		reader.resetPeek();
		auto res = parseGenericToken(reader, logger, delims);
		hadError = hadError || !res.first;
		arr.push_back(res.second);
	}
	reader.resetPeek();

	// The resulting array should not be empty
	if (arr.empty()) {
		return error(reader, logger, ERR_UNEXPECTED_END, nullptr);
	}

	// If there only one element was extracted, return this element instead of
	// an array
	if (arr.size() == 1) {
		return std::make_pair(!hadError, arr[0]);
	} else {
		return std::make_pair(!hadError, Variant{arr});
	}
}

std::pair<bool, Variant> VariantReader::parseGenericToken(
    CharReader &reader, Logger &logger, const std::unordered_set<char> &delims,
    bool extractUnescapedStrings)
{
	char c;

	// Skip all whitespace characters, read a character and abort if at the end
	reader.consumeWhitespace();
	if (!reader.peek(c) || delims.count(c)) {
		reader.resetPeek();
		return error(reader, logger, ERR_UNEXPECTED_END, nullptr);
	}

	// Parse a string if a quote is reached
	if (c == '"' || c == '\'') {
		auto res = parseString(reader, logger);
		return std::make_pair(res.first, res.second.c_str());
	}

	// Try to parse everything that looks like a number as number
	if (Utils::isNumeric(c) || c == '-') {
		// Try to parse the number
		Number n;
		CharReaderFork readerFork = reader.fork();
		LoggerFork loggerFork = logger.fork();
		if (n.parse(readerFork, loggerFork, delims)) {
			readerFork.commit();
			loggerFork.commit();
			if (n.isInt()) {
				return std::make_pair(
				    true, Variant{static_cast<Variant::intType>(n.intValue())});
			} else {
				return std::make_pair(true, n.doubleValue());
			}
		}
	}

	// Try to parse an object
	if (c == '[') {
		return parseComplex(reader, logger, 0, ComplexMode::BOTH);
	}

	// Otherwise parse a single token
	std::pair<bool, std::string> res;
	if (extractUnescapedStrings) {
		res = parseUnescapedString(reader, logger, delims);
	} else {
		res = parseToken(reader, logger, delims);
	}

	// Handling for special primitive values
	if (res.first) {
		if (res.second == "true") {
			return std::make_pair(true, Variant{true});
		}
		if (res.second == "false") {
			return std::make_pair(true, Variant{false});
		}
		if (res.second == "null") {
			return std::make_pair(true, Variant{nullptr});
		}
	}

	// Check whether the parsed string is a valid identifier -- if yes, flag it
	// as "magic" string
	if (Utils::isIdentifier(res.second)) {
		Variant v;
		v.setMagic(res.second.c_str());
		return std::make_pair(res.first, v);
	} else {
		return std::make_pair(res.first, Variant{res.second.c_str()});
	}
}
}

