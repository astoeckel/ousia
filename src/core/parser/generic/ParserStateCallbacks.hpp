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
 * @file ParserStateCallbacks.hpp
 *
 * Contains an interface defining the callbacks that can be directed from a
 * ParserStateHandler to the ParserStateStack, and from the ParserStateStack to
 * the actual parser.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STATE_CALLBACKS_HPP_
#define _OUSIA_PARSER_STATE_CALLBACKS_HPP_

#include <string>

#include <core/common/Whitespace.hpp>

namespace ousia {

/**
 * Interface defining a set of callback functions that can be directed from a
 * ParserStateHandler to the ParserStateStack and form the ParserStateStack
 * to the actual parser.
 */
class ParserStateCallbacks {
public:
	/**
	 * Sets the whitespace mode that specifies how (string data) should be
	 * processed.
	 *
	 * @param whitespaceMode specifies one of the three WhitespaceMode constants
	 * PRESERVE, TRIM or COLLAPSE.
	 */
	virtual void setWhitespaceMode(WhitespaceMode whitespaceMode) = 0;

	/**
	 * Sets the type as which the variant data should be parsed.
	 *
	 * @param type is one of the VariantType constants, specifying with which
	 * type the data that is passed to the ParserStateHandler in the "data"
	 * function should be handled.
	 */
	virtual void setDataType(VariantType type) = 0;

	/**
	 * Checks whether the given token is supported by the parser. The parser
	 * returns true, if the token is supported, false if this token cannot be
	 * registered. Note that parsers that do not support the registration of
	 * tokens at all should always return "true".
	 *
	 * @param token is the token that should be checked for support.
	 * @return true if the token is generally supported (or the parser does not
	 * support registering tokens at all), false if the token is not supported,
	 * because e.g. it is a reserved token or it interferes with other tokens.
	 */
	virtual bool supportsToken(const std::string &token) = 0;

	/**
	 * Registers the given token as token that should be reported to the handler
	 * using the "token" function.
	 *
	 * @param token is the token string that should be reported.
	 */
	virtual void registerToken(const std::string &token) = 0;

	/**
	 * Unregisters the given token, it will no longer be reported to the handler
	 * using the "token" function.
	 *
	 * @param token is the token string that should be unregistered.
	 */
	virtual void unregisterToken(const std::string &token) = 0;
};

}

#endif /* _OUSIA_PARSER_STATE_CALLBACKS_HPP_ */

