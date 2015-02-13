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
	 * Extracts a single token from the given CharReader instance. Skips any
	 * whitespace character until a non-whitespace character is reached. Stops
	 * if another whitespace character is read or one of the given delimiters
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
	static std::pair<bool, std::string> parseToken(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> &delims);

	/**
	 * Extracts an unescaped string from the given CharReader instance. Skips
	 * any whitespace character one of the given delimiters is reached. Strips
	 * whitespace at the end of the string.
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
	 * Parses a bool from the given CharReader instance (the strings "true" or
	 * "false").
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the bool.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 */
	static std::pair<bool, Variant::boolType> parseBool(CharReader &reader,
	                                                    Logger &logger);

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
	static std::pair<bool, Variant::arrayType> parseArray(CharReader &reader,
	                                                      Logger &logger,
	                                                      char delim = 0);

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
	static std::pair<bool, Variant::mapType> parseObject(CharReader &reader,
	                                                     Logger &logger,
	                                                     char delim = 0);

	/**
	 * Parses a Cardinality. A Cardinality is specified as a list of Ranges,
	 * separated by commas and enclosed in curly braces. The ranges can be
	 * either
	 *
	 * * simple unsigned integers
	   * a pair of unsigned integers with a hyphen in between (specifying the
	     range from the left to the right integer)
	 * * an unsigned integer preceded by a <, specifying the range from 0 to the
	 *   integer.
	 * * an unsigned integer preceded by a >, specifying the range from the
	 *   integer to infinity.
	 * * A Kleene-Star (*), specifying the range from 0 to infinity.
	 *
	 * Consider the following examples:
	 *
	 * * {3}
	 * * {0-1}, which is equivalent to {<1}
	 * * {>0}
	 * * {*}
	 *
	 * Note that the given Ranges will be merged internally to be non-redundant,
	 * as in the following examples:
	 *
	 * * {3-9, 7-10} will be optimized to {3-10}
	 * * {> 1, 8} will be optimized to {> 1}
	 * * {*, > 0} will be optimized to {*}
	 *
	 * @param reader is a reference to the CharReader instance which is
	 * the source for the character data. The reader will be positioned after
	 * the number or at the terminating delimiting character.
	 * @param logger is the logger instance that should be used to log error
	 * messages and warnings.
	 */
	static std::pair<bool, Variant::cardinalityType> parseCardinality(
	    CharReader &reader, Logger &logger);

	/**
	 * Tries to parse the most specific item from the given stream until one of
	 * the given delimiters is reached or a meaningful literal (possibly an
	 * array of literals) has been read. The resulting variant represents the
	 * value that has been read.
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
	 * @param extractUnescapedStrings if set to true, interprets non-primitive
	 * literals as unescaped strings, which may also contain whitespace
	 * characters. Otherwise string literals are only generated until the next
	 * whitespace character.
	 */
	static std::pair<bool, Variant> parseGenericToken(
	    CharReader &reader, Logger &logger,
	    const std::unordered_set<char> &delims,
	    bool extractUnescapedStrings = false);

	/**
	 * Tries to parse the most specific item from the given string. The
	 * resulting variant represents the value that has been read. If the end of
	 * the string was not reached while parsing an element, the result is
	 * returned as string.
	 *
	 * @param str is the string from which the value should be read.
	 * @param logger is the logger instance to which errors or warnings will be
	 * written.
	 * @param sourceId is an optional descriptor of the source file from which
	 * the element is being read.
	 * @param offs is the by offset in the source file at which the string
	 * starts.
	 * @return a pair indicating whether the operation was successful and the
	 * extracted variant value. Note that the variant value most times contains
	 * some meaningful data that can be worked with even if the operation was
	 * not successful (e.g. if a syntax error is encountered while reading an
	 * array, the successfully read elements will still be in the returned
	 * variant.) Information on why the operation has failed is passed to the
	 * logger.
	 */
	static std::pair<bool, Variant> parseGenericString(
	    const std::string &str, Logger &logger,
	    SourceId sourceId = InvalidSourceId, size_t offs = 0);
};
}

#endif /* _OUSIA_VARIANT_READER_HPP_ */

