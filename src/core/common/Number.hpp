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

/**
 * @file Number.hpp
 *
 * Contains the Number class responsible for parsing integers and doubles of
 * various bases.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#include <cstdint>
#include <string>
#include <unordered_set>

namespace ousia {

// Forward declarations
class CharReader;
class Logger;

/* Class Number */

/**
 * Class used internally to represent a number (integer or double). The number
 * is represented by its components (base value a, nominator n, denominator d,
 * exponent e, sign s and exponent sign sE).
 */
class Number {
private:
	/**
	 * Represents the part of the number: Base value a, nominator n, exponent e.
	 */
	enum class Part { A, N, E };

	/**
	 * Sign of the number and the exponent.
	 */
	int8_t s, sE;

	/**
	 * Exponent.
	 */
	int16_t e;

	/**
	 * Base value, nominator, denominator
	 */
	int64_t a, n, d;

	/**
	 * Variable specifying whether the parsed number actually was an integer.
	 */
	bool validInteger;

	/**
	 * Appends the value of the character c to the internal number
	 * representation and reports any errors that might occur.
	 *
	 * @param c is the character that should be appended.
	 * @param base is the current base.
	 * @param p is the current number part.
	 * @param reader is the char reader which points at the current reading
	 * position.
	 */
	bool appendChar(char c, int base, Part p, CharReader &reader,
	                Logger &logger);

public:
	/**
	 * Constructor of the number class.
	 */
	Number() : s(1), sE(1), e(0), a(0), n(0), d(1), validInteger(true) {}

	/**
	 * Returns the represented double value.
	 *
	 * @return the double value the number is currently representing.
	 */
	double doubleValue();

	/**
	 * Returns the represented integer value. Only a lossless operation, if the
	 * number is an integer (as can be checked via the isInt method), otherwise
	 * the exponent and the fractional value will be truncated.
	 *
	 * @return the integer value (ignoring any exponent)
	 */
	int64_t intValue();

	/**
	 * Returns true, if the number was a valid integer.
	 *
	 * @return true if the number is an integer, false otherwise.
	 */
	bool isInt() { return validInteger; }

	/**
	 * Tries to parse the number from the given stream and loggs any errors to
	 * the given logger instance. Numbers are terminated by one of the given
	 * delimiters.
	 *
	 * @param reader is the char reader from which the number should be read.
	 * @param logger is the logger instance to which error messages should be
	 * written.
	 * @param delims is a set of characters at which parsing should stop. The
	 * reader is positioned at the delimiter.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parse(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> &delims = std::unordered_set<char>{});

	/**
	 * Tries to parse the number from the given string and loggs any errors to
	 * the given logger instance.
	 *
	 * @param str is the string from which the number should be read.
	 * @param logger is the logger instance to which error messages should be
	 * written.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parse(const std::string &str, Logger &logger);

	/**
	 * Parses an integer with a fixed length and the given base.
	 *
	 * @param reader is a reference at the char reader from which the number
	 * should be read.
	 * @param len is the length of the integer sequence.
	 * @param base is the base of the number.
	 * @param logger is the logger instance to which error messages should be
	 * written.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parseFixedLengthInteger(CharReader &reader, int len, int base,
	                      Logger &logger);
};
}

