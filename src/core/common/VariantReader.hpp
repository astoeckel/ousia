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
 * @file VariantReader.hpp
 *
 * Provides parsers for various micro formats. These formats include integers,
 * doubles, strings, JSON and the Ousía struct notation.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_VARIANT_READER_HPP_
#define _OUSIA_VARIANT_READER_HPP_

#include <cstdint>
#include <unordered_set>
#include <utility>

#include "CharReader.hpp"
#include "Logger.hpp"
#include "Variant.hpp"

namespace ousia {

class VariantReader {
private:
	/**
	 * Parses a string which may either be enclosed by " or ', unescapes
	 * entities in the string as specified for JavaScript.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the terminating quote character or at the terminating delimiting
	 * character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 * @param delims is an optional set of delimiters after which parsing has to
	 * be stopped (the delimiters may occur inside the actual string, but not
	 * outside). If nullptr is given, no delimiter is used and a complete string
	 * is read.
	 */
	static std::pair<bool, std::string> parseString(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> *delims);

public:
	/**
	 * Parses a string which may either be enclosed by " or ', unescapes
	 * entities in the string as specified for JavaScript.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the terminating quote character or at the terminating delimiting
	 * character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 * @param delims is a set of delimiters after which parsing has to
	 * be stopped (the delimiters may occur inside the actual string, but not
	 * outside).
	 */
	static std::pair<bool, std::string> parseString(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> &delims)
	{
		return parseString(reader, logger, &delims);
	}

	/**
	 * Parses a string which may either be enclosed by " or ', unescapes
	 * entities in the string as specified for JavaScript.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the terminating quote character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 */
	static std::pair<bool, std::string> parseString(CharReader &reader,
	                                                Logger &logger)
	{
		return parseString(reader, logger, nullptr);
	}

	/**
	 * Extracts an unescaped string from the given CharReader instance.
	 * This function just reads text until one of the given delimiter
	 * characters is reached.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned at the
	 * terminating delimiting character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 * @param delims is a set of characters which will terminate the string.
	 * These characters are not included in the result.
	 */
	static std::pair<bool, std::string> parseUnescapedString(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> &delims);

	/**
	 * Parses an integer from the given CharReader instance until one of the
	 * given delimiter characters is reached.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the number or at the terminating delimiting character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 * @param delims is a set of characters which will terminate the integer.
	 * These characters are not included in the result.
	 */
	static std::pair<bool, int64_t> parseInteger(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> &delims);

	/**
	 * Parses an double from the given CharReader instance until one of the
	 * given delimiter characters is reached.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the number or at the terminating delimiting character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 * @param delims is a set of characters which will terminate the double.
	 * These characters are not included in the result.
	 */
	static std::pair<bool, double> parseDouble(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> &delims);

	/**
	 * Parses an array of values.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the number or at the terminating delimiting character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 * @param delim is the terminating character. If nonzero, the parse function
	 * assumes that it is already inside the array and will not wait for a '['
	 * character.
	 */
	static std::pair<bool, Variant::arrayType> parseArray(
	    CharReader &reader, Logger &logger, char delim = 0);

	/**
	 * Parses an object definition.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the number or at the terminating delimiting character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 * @param delim is the terminating character. If nonzero, the parse function
	 * assumes that it is already inside the array and will not wait for a '['
	 * character.
	 */
	static std::pair<bool, Variant::mapType> parseObject(
	    CharReader &reader, Logger &logger, char delim = 0);

	/**
	 * Tries to parse the most specific item from the given stream until one of
	 * the given delimiters is reached or a meaningful literal has been read.
	 * The resulting variant represents the value that has been read.
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned
	 * at the terminating delimiting character.
	 * @param delims is a set of characters which will terminate the string.
	 * These characters are not included in the result. May not be nullptr.
	 */
	static std::pair<bool, Variant> parseGeneric(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> &delims);
};
}

#endif /* _OUSIA_VARIANT_READER_HPP_ */

