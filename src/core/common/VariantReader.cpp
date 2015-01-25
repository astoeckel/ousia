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

#include "Number.hpp"
#include "VariantReader.hpp"
#include "Utils.hpp"

namespace ousia {

/* Error Messages */

// TODO: Invent common system for error messages which allows localization
// TODO: Possibly adapt the clang error logging system

static const char *ERR_UNEXPECTED_CHAR = "Unexpected character";
static const char *ERR_UNEXPECTED_END = "Unexpected end of literal";
static const char *ERR_UNTERMINATED = "Unterminated literal";
static const char *ERR_INVALID_ESCAPE = "Invalid escape sequence";
static const char *ERR_INVALID_INTEGER = "Invalid integer value";

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

/* State machine states */

static const int STATE_INIT = 0;
static const int STATE_IN_STRING = 1;
static const int STATE_IN_COMPLEX = 2;
static const int STATE_ESCAPE = 4;
static const int STATE_WHITESPACE = 5;
static const int STATE_RESYNC = 6;
static const int STATE_EXPECT_COMMA = 7;
static const int STATE_HAS_KEY = 8;
static const int STATE_HAS_START = 9;

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
						hadError = !(n.parseFixedLengthInteger(reader, 2, 16,
						                                       logger) &&
						             encodeUtf8(res, reader, logger,
						                        n.intValue(), true)) ||
						           hadError;
						break;
					}
					case 'u': {
						// Parse Unicode sequence \uXXXX
						Number n;
						hadError = !(n.parseFixedLengthInteger(reader, 4, 16,
						                                       logger) &&
						             encodeUtf8(res, reader, logger,
						                        n.intValue(), false)) ||
						           hadError;
						break;
					}
					default:
						if (Utils::isNumeric(c)) {
							// Parse Latin-1 sequence \000
							reader.resetPeek();
							Number n;
							hadError = !(n.parseFixedLengthInteger(reader, 3, 8,
							                                       logger) &&
							             encodeUtf8(res, reader, logger,
							                        n.intValue(), true)) ||
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

static const std::unordered_set<char> cardDelims{' ', ',', '}', '-'};

std::pair<bool, Variant::cardinalityType> VariantReader::parseCardinality(
    CharReader &reader, Logger &logger)
{
	// first we consume all whitespaces.
	reader.consumeWhitespace();
	// then we expect curly braces.
	char c;
	if (!reader.read(c) || c != '{') {
		return unexpected(reader, logger, "{", c, Variant::cardinalityType{});
	}

	Variant::cardinalityType card{};

	reader.consumePeek();
	reader.consumeWhitespace();

	// which should in turn be followed by ranges.
	while (reader.peek(c)) {
		if (Utils::isNumeric(c)) {
			// in case of a numeric character we want to read an integer.
			reader.resetPeek();
			Number n;
			if (!n.parse(reader, logger, cardDelims) || !n.isInt() ||
			    n.intValue() < 0) {
				return error(reader, logger, "Invalid number for cardinality!",
				             Variant::cardinalityType{});
			}
			unsigned int start = (unsigned int)n.intValue();
			// if we have that we might either find a } or , making this a
			// range or a - leading us to expect another integer.
			reader.consumePeek();
			reader.consumeWhitespace();
			if (!reader.peek(c)) {
				error(reader, logger, ERR_UNEXPECTED_END,
				      Variant::cardinalityType{});
			}
			switch (c) {
				case '}':
				case ',':
					reader.resetPeek();
					break;
				case '-': {
					// get another integer.
					reader.consumePeek();
					reader.consumeWhitespace();
					if (!reader.peek(c)) {
						error(reader, logger, ERR_UNEXPECTED_END,
						      Variant::cardinalityType{});
					}
					Number n2;
					if (!n2.parse(reader, logger, cardDelims) || !n2.isInt() ||
					    n2.intValue() < 0) {
						return error(reader, logger,
						             "Invalid number for cardinality!",
						             Variant::cardinalityType{});
					}

					unsigned int end = (unsigned int)n2.intValue();
					card.merge({start, end});
					break;
				}
				default:
					return unexpected(reader, logger, "}, , or -", c,
					                  Variant::cardinalityType{});
			}
		} else {
			switch (c) {
				case '*':
					// in case of a Kleene star we can construct the
					// cardinality right away.
					card.merge(Variant::rangeType::typeRangeFrom(0));
					break;
				case '<':
				case '>': {
					// in case of an open range we expect a number.
					reader.consumePeek();
					reader.consumeWhitespace();
					Number n;
					if (!n.parse(reader, logger, cardDelims)) {
						return error(reader, logger,
						             "Expected number in an open range "
						             "specifier!",
						             Variant::cardinalityType{});
					}
					if (!n.isInt() || n.intValue() < 0) {
						return error(reader, logger,
						             "Invalid number for cardinality!",
						             Variant::cardinalityType{});
					}
					if (c == '<') {
						card.merge(Variant::rangeType{
						    0, (unsigned int)n.intValue() - 1});
					} else {
						card.merge(Variant::rangeType::typeRangeFrom(
						    (unsigned int)n.intValue() + 1));
					}
					break;
				}
				default:
					return unexpected(reader, logger,
					                  "Unsigned integer, *, < or >", c,
					                  Variant::cardinalityType{});
			}
		}
		// after we have parsed a range, read all whitespaces.
		reader.consumePeek();
		reader.consumeWhitespace();
		// ... and check if we are at the end.
		if (!reader.read(c)) {
			error(reader, logger, ERR_UNEXPECTED_END,
			      Variant::cardinalityType{});
		}
		switch (c) {
			case '}':
				return std::make_pair(true, card);
			case ',':
				reader.consumePeek();
				reader.consumeWhitespace();
				break;

			default:
				return unexpected(reader, logger, "} or ,", c,
				                  Variant::cardinalityType{});
		}
	}

	return error(reader, logger, ERR_UNEXPECTED_END,
	             Variant::cardinalityType{});
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
		reader.resetPeek();
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
		reader.resetPeek();
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
		return std::make_pair(res.first, Variant::fromString(res.second));
	}
}

std::pair<bool, Variant> VariantReader::parseGenericString(
    const std::string &str, Logger &logger)
{
	CharReader reader{str};
	LoggerFork loggerFork = logger.fork();
	std::pair<bool, Variant> res =
	    parseGenericToken(reader, loggerFork, std::unordered_set<char>{}, true);
	if (reader.atEnd()) {
		loggerFork.commit();
		return res;
	}
	return std::make_pair(true, Variant::fromString(str));
}
}

