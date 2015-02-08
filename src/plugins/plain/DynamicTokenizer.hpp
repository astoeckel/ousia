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

#include <set>
#include <string>
#include <vector>

#include <core/common/Location.hpp>

#include "TokenTrie.hpp"

namespace ousia {

// Forward declarations
class CharReader;

/**
 * The DynamicToken structure describes a token discovered by the Tokenizer.
 */
struct DynamicToken {
	/**
	 * Id of the type of this token.
	 */
	TokenTypeId type;

	/**
	 * String that was matched.
	 */
	std::string content;

	/**
	 * Location from which the string was extracted.
	 */
	SourceLocation location;

	/**
	 * Default constructor.
	 */
	DynamicToken() : type(EmptyToken) {}

	/**
	 * Constructor of the DynamicToken struct.
	 *
	 * @param id represents the token type.
	 * @param content is the string content that has been extracted.
	 * @param location is the location of the extracted string content in the
	 * source file.
	 */
	DynamicToken(TokenTypeId type, const std::string &content,
	             SourceLocation location)
	    : type(type), content(content), location(location)
	{
	}

	/**
	 * Constructor of the DynamicToken struct, only initializes the token type
	 *
	 * @param type is the id corresponding to the type of the token.
	 */
	DynamicToken(TokenTypeId type) : type(type) {}
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
 * to modify the handling of whitespace characters. Note that the
 * DynamicTokenizer always tries to extract the longest possible token from the
 * tokenizer.
 */
class DynamicTokenizer {
private:
	/**
	 * Internally used token trie. This object holds all registered tokens.
	 */
	TokenTrie trie;

	/**
	 * Flag defining whether whitespaces should be preserved or not.
	 */
	WhitespaceMode whitespaceMode;

	/**
	 * Vector containing all registered token types.
	 */
	std::vector<std::string> tokens;

	/**
	 * Next index in the tokens list where to search for a new token id.
	 */
	size_t nextTokenTypeId;

	/**
	 * Templated function used internally to read the current token. The
	 * function is templated in order to force code generation for all six
	 * combiations of whitespace modes and reading/peeking.
	 *
	 * @tparam TextHandler is the type to be used for the textHandler instance.
	 * @tparam read specifies whether the function should start from and advance
	 * the read pointer of the char reader.
	 * @param reader is the CharReader instance from which the data should be
	 * read.
	 * @param token is the token structure into which the token information
	 * should be written.
	 * @return false if the end of the stream has been reached, true otherwise.
	 */
	template <typename TextHandler, bool read>
	bool next(CharReader &reader, DynamicToken &token);

public:
	/**
	 * Constructor of the DynamicTokenizer class.
	 *
	 * @param whitespaceMode specifies how whitespace should be handled.
	 */
	DynamicTokenizer(WhitespaceMode whitespaceMode = WhitespaceMode::COLLAPSE);

	/**
	 * Registers the given string as a token. Returns a const pointer at a
	 * TokenDescriptor that will be used to reference the newly created token.
	 *
	 * @param token is the token string that should be registered.
	 * @return a unique identifier for the registered token or EmptyToken if
	 * an error occured.
	 */
	TokenTypeId registerToken(const std::string &token);

	/**
	 * Unregisters the token belonging to the given TokenTypeId.
	 *
	 * @param type is the token type that should be unregistered. The
	 *TokenTypeId
	 * must have been returned by registerToken.
	 * @return true if the operation was successful, false otherwise (e.g.
	 * because the given TokenDescriptor was already unregistered).
	 */
	bool unregisterToken(TokenTypeId type);

	/**
	 * Returns the token that was registered under the given TokenTypeId id or
	 *an
	 * empty string if an invalid TokenTypeId id is given.
	 *
	 * @param type is the TokenTypeId id for which the corresponding token
	 *string
	 * should be returned.
	 * @return the registered token string or an empty string if the given type
	 * was invalid.
	 */
	std::string getTokenString(TokenTypeId type);

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
	 * @param reader is the CharReader instance from which the data should be
	 * read.
	 * @param token is a reference at the token instance into which the Token
	 * information should be written.
	 * @return true if a token could be read, false if the end of the stream
	 * has been reached.
	 */
	bool read(CharReader &reader, DynamicToken &token);

	/**
	 * The peek method does not advance the read position of the char reader,
	 * but reads the next token from the current char reader peek position.
	 *
	 * @param reader is the CharReader instance from which the data should be
	 * read.
	 * @param token is a reference at the token instance into which the Token
	 * information should be written.
	 * @return true if a token could be read, false if the end of the stream
	 * has been reached.
	 */
	bool peek(CharReader &reader, DynamicToken &token);
};
}

#endif /* _OUSIA_DYNAMIC_TOKENIZER_HPP_ */

