/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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
 * @file DynamicTokenizer.hpp
 *
 * Tokenizer that can be reconfigured at runtime used for parsing the plain
 * text format.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_DYNAMIC_TOKENIZER_HPP_
#define _OUSIA_DYNAMIC_TOKENIZER_HPP_

#include <core/common/Location.hpp>

namespace ousia {

// Forward declarations
class CharReader;
class TokenDescriptor;

/**
 * The DynamicToken structure describes a token discovered by the Tokenizer.
 */
struct DynamicToken {
	/**
	 * Pointer pointing at the TokenDescriptor instance this token corresponds
	 * to. May be one of the special TokenDescriptors defined as static members
	 * of the DynamicTokenizer class.
	 */
	TokenDescriptor const *descriptor;

	/**
	 * String that was matched.
	 */
	std::string str;

	/**
	 * Location from which the string was extracted.
	 */
	SourceLocation location;
};

/**
 * Enum specifying the whitespace handling of the DynamicTokenizer class when
 * reading non-token text.
 */
enum class WhitespaceMode {
	/**
	 * Preserves all whitespaces as they are found in the source file.
	 */
	PRESERVE,

	/**
	 * Trims whitespace at the beginning and the end of the found text.
	 */
	TRIM,

	/**
	 * Whitespaces are trimmed and collapsed, multiple whitespace characters
	 * are replaced by a single space character.
	 */
	COLLAPSE
};

/**
 * The DynamicTokenizer is used to extract tokens and chunks of text from a
 * CharReader. It allows to register and unregister tokens while parsing and
 * to modify the handling of whitespace characters.
 */
class DynamicTokenizer {
private:
	/**
	 * Reference at the char reader.
	 */
	CharReader &reader;

	/**
	 * Flag defining whether whitespaces should be preserved or not.
	 */
	WhitespaceMode whitespaceMode;

	/**
	 * Vector containing all registered token descriptors.
	 */
	std::vector<std::unique_ptr<TokenDescriptor>> descriptors;

public:
	/**
	 * Constructor of the DynamicTokenizer class.
	 *
	 * @param reader is the CharReader that should be used for reading the
	 * tokens.
	 * @param preserveWhitespaces should be set to true if all whitespaces
	 * should be preserved (for preformated environments).
	 */
	DynamicTokenizer(CharReader &reader)
	    : reader(reader),
	      preserveWhitespaces(preserveWhitespaces),
	      location(reader.getSourceId()),
	      empty(true),
	      hasWhitespace(false)
	{
	}

	/**
	 * Destructor of the DynamicTokenizer class.
	 */
	~DynamicTokenizer();

	/**
	 * Registers the given string as a token. Returns a const pointer at a
	 * TokenDescriptor that will be used to reference the newly created token.
	 *
	 * @param token is the token string that should be registered.
	 * @return a pointer at a TokenDescriptor which is representative for the
	 * newly registered token. Returns nullptr if a token with this string
	 * was already registered.
	 */
	const TokenDescriptor* registerToken(const std::string &token);

	/**
	 * Unregisters the token belonging to the given TokenDescriptor.
	 *
	 * @param descr is a TokenDescriptor that was previously returned by
	 * registerToken.
	 * @return true if the operation was successful, false otherwise (e.g.
	 * because the given TokenDescriptor was already unregistered).
	 */
	bool unregisterToken(const TokenDescriptor *descr);

	/**
	 * Sets the whitespace mode.
	 *
	 * @param whitespaceMode defines how whitespace should be treated in text
	 * tokens.
	 */
	void setWhitespaceMode(WhitespaceMode mode);

	/**
	 * Returns the current value of the whitespace mode.
	 *
	 * @return the whitespace mode.
	 */
	WhitespaceMode getWhitespaceMode();

	/**
	 * Reads a new token from the CharReader and stores it in the given
	 * DynamicToken instance.
	 *
	 * @param token is a reference at the token instance into which the Token
	 * information should be written.
	 * @return true if a token could be read, false if the end of the stream
	 * has been reached.
	 */
	bool read(DynamicToken &token);

	/**
	 * TokenDescriptor representing an empty token.
	 */
	static const *TokenDescriptor Empty;

	/**
	 * TokenDescriptor representing generic text.
	 */
	static const *TokenDescriptor Text;

};

}

#endif /* _OUSIA_DYNAMIC_TOKENIZER_HPP_ */

