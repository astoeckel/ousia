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
 * @file Tokenizer.hpp
 *
 * Tokenizer that can be reconfigured at runtime and is used for parsing the
 * plain text format.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_DYNAMIC_TOKENIZER_HPP_
#define _OUSIA_DYNAMIC_TOKENIZER_HPP_

#include <set>
#include <string>
#include <vector>

#include <core/common/Location.hpp>
#include <core/common/Token.hpp>

#include "TokenTrie.hpp"

namespace ousia {

// Forward declarations
class CharReader;
class TokenizedData;

/**
 * The Tokenizer is used to extract tokens and chunks of text from a
 * CharReader. It allows to register and unregister tokens while parsing. Note
 * that the Tokenizer always tries to extract the longest possible token from
 * the tokenizer. Tokens can be registered as primary or non-primary token. If
 * a Token is registered as a primary token, it is returned as a single Token
 * instance if it occurs. In the non-primary case the token is returned as part
 * of a segmented TokenizedData instance.
 */
class Tokenizer {
public:
	/**
	 * Internally used structure describing a registered token.
	 */
	struct TokenDescriptor {
		/**
		 * String describing the token.
		 */
		std::string string;

		/**
		 * Set to true if this token is primary.
		 */
		bool primary;

		/**
		 * Constructor of the TokenDescriptor class.
		 *
		 * @param string is the string representation of the registered token.
		 * @param primary specifies whether the token is a primary token that
		 * should be returned as a single token, or a secondary token, that
		 * should be returned as part of TokenizedData.
		 */
		TokenDescriptor(const std::string &string, bool primary)
		    : string(string), primary(primary)
		{
		}

		/**
		 * Default constructor.
		 */
		TokenDescriptor() : primary(false) {}

		/**
		 * Returns true if the TokenDescriptor represents a valid token.
		 */
		bool valid() { return !string.empty(); }
	};

private:
	/**
	 * Internally used token trie. This object holds all registered tokens.
	 */
	TokenTrie trie;

	/**
	 * Vector containing all registered token types.
	 */
	std::vector<TokenDescriptor> tokens;

	/**
	 * Next index in the tokens list where to search for a new token id.
	 */
	size_t nextTokenId;

	/**
	 * Templated function used internally to read the current token. The
	 * function is templated in order to force optimized code generation for
	 * both reading and peeking.
	 *
	 * @tparam read specifies whether the method should read the token or just
	 * peek.
	 * @param reader is the CharReader instance from which the data should be
	 * read.
	 * @param token is the token structure into which the token information
	 * should be written.
	 * @param data is a reference at the TokenizedData instance to which the
	 * token information should be appended.
	 * @return false if the end of the stream has been reached, true otherwise.
	 */
	template <bool read>
	bool next(CharReader &reader, Token &token, TokenizedData &data);

public:
	/**
	 * Constructor of the Tokenizer class.
	 */
	Tokenizer();

	/**
	 * Registers the given string as a token. Returns a unique identifier
	 * describing the registered token.
	 *
	 * @param token is the token string that should be registered.
	 * @param primary specifies whether the token is a primary token -- if true,
	 * the token will be returned as a single, standalone token. Otherwise the
	 * token will be returned as part of a "TokenizedData" structure.
	 * @return a unique identifier for the registered token or Tokens::Empty if
	 * an error occured.
	 */
	TokenId registerToken(const std::string &token, bool primary = true);

	/**
	 * Unregisters the token belonging to the given TokenId.
	 *
	 * @param type is the token type that should be unregistered. The
	 * TokenId must have been returned by registerToken.
	 * @return true if the operation was successful, false otherwise (e.g.
	 * because the token with the given TokenId was already unregistered).
	 */
	bool unregisterToken(TokenId id);

	/**
	 * Returns the token that was registered under the given TokenId id or
	 * an empty string if an invalid TokenId id is given.
	 *
	 * @param id is the TokenId for which the corresponding TokenDescriptor
	 * should be returned.
	 * @return the registered TokenDescriptor or an invalid TokenDescriptor if
	 * the given TokenId is invalid.
	 */
	const TokenDescriptor& lookupToken(TokenId id) const;

	/**
	 * Reads a new token from the CharReader and stores it in the given
	 * Token instance. If the token has the id Tokens::Data, use the "getData"
	 * method to fetch a reference at the underlying TokenizedData instance
	 * storing the data.
	 *
	 * @param reader is the CharReader instance from which the data should be
	 * read.
	 * @param token is a reference at the token instance into which the Token
	 * information should be written.
	 * @param data is a reference at the TokenizedData instance to which the
	 * token information should be appended.
	 * @return true if a token could be read, false if the end of the stream
	 * has been reached.
	 */
	bool read(CharReader &reader, Token &token, TokenizedData &data);

	/**
	 * The peek method does not advance the read position of the char reader,
	 * but reads the next token from the current char reader peek position.
	 *
	 * @param reader is the CharReader instance from which the data should be
	 * read.
	 * @param token is a reference at the token instance into which the Token
	 * information should be written.
	 * @param data is a reference at the TokenizedData instance to which the
	 * token information should be appended.
	 * @return true if a token could be read, false if the end of the stream
	 * has been reached.
	 */
	bool peek(CharReader &reader, Token &token, TokenizedData &data);
};
}

#endif /* _OUSIA_DYNAMIC_TOKENIZER_HPP_ */

