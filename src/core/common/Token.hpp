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
 * @file Token.hpp
 *
 * Definition of the TokenId id and constants for some special tokens.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_TOKEN_HPP_
#define _OUSIA_TOKEN_HPP_

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>

#include <core/common/Location.hpp>

namespace ousia {

/**
 * The TokenId is used to give each token id a unique id.
 */
using TokenId = uint32_t;

/**
 * Type used for storing token lengths.
 */
using TokenLength = uint16_t;

/**
 * Type used for storing token sets.
 */
using TokenSet = std::unordered_set<TokenId>;

/**
 * Namespace containing constants for TokenId instances with special meaning.
 */
namespace Tokens {
/**
 * Token which is not a token.
 */
constexpr TokenId Empty = std::numeric_limits<TokenId>::max();

/**
 * Token which represents data (represented as TokenizedData).
 */
constexpr TokenId Data = std::numeric_limits<TokenId>::max() - 1;

/**
 * Token which represents a newline token.
 */
constexpr TokenId Newline = std::numeric_limits<TokenId>::max() - 2;

/**
 * Token which represents a paragraph token -- issued if two consecutive
 * newlines occur with optionally any amout of whitespace between them. The
 * paragraph token is not repeated until more text is reached.
 */
constexpr TokenId Paragraph = std::numeric_limits<TokenId>::max() - 3;

/**
 * Token which represents a section token -- issued if three or more
 * consecutive newlines occur with optionally any amout of whitespace between
 * them. The section token is not repeated until more text is reached.
 */
constexpr TokenId Section = std::numeric_limits<TokenId>::max() - 4;

/**
 * Token which represents an indentation token -- issued if the indentation of
 * this line is larger than the indentation of the previous line.
 */
constexpr TokenId Indent = std::numeric_limits<TokenId>::max() - 5;

/**
 * Token which represents an dedentation -- issued if the indentation of
 * this line is smaller than the indentation of the previous line.
 */
constexpr TokenId Dedent = std::numeric_limits<TokenId>::max() - 6;

/**
 * Maximum token id to be used. Tokens allocated for users should not surpass
 * this value.
 */
constexpr TokenId MaxTokenId = std::numeric_limits<TokenId>::max() - 255;
}

/**
 * The Token structure describes a token discovered by the Tokenizer or read
 * from the TokenizedData struct.
 */
struct Token {
	/**
	 * Id of the id of this token.
	 */
	TokenId id;

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
	Token() : id(Tokens::Empty) {}

	/**
	 * Constructor of a "data" token with no explicit content.
	 *
	 * @param location is the location of the extracted string content in the
	 * source file.
	 */
	Token(const SourceLocation &location) : id(Tokens::Data), location(location)
	{
	}

	/**
	 * Constructor of the Token struct.
	 *
	 * @param id represents the token id.
	 * @param content is the string content that has been extracted.
	 * @param location is the location of the extracted string content in the
	 * source file.
	 */
	Token(TokenId id, const std::string &content,
	      const SourceLocation &location)
	    : id(id), content(content), location(location)
	{
	}

	/**
	 * Constructor of the a "data" Token with the given string data and
	 * location.
	 *
	 * @param content is the string content that should be stored in the token.
	 * @param location is the location of the content within the source file.
	 */
	Token(const std::string &content,
	      const SourceLocation &location = SourceLocation{})
	    : id(Tokens::Data), content(content), location(location)
	{
	}

	/**
	 * Constructor of the Token struct, only initializes the token id
	 *
	 * @param id is the id corresponding to the id of the token.
	 */
	Token(TokenId id) : id(id) {}

	/**
	 * Returns true if this token is special.
	 *
	 * @return true if the TokenId indicates that this token is a "special"
	 * token.
	 */
	bool isSpecial() const { return isSpecial(id); }

	/**
	 * Returns true if the given token id is special.
	 *
	 * @param id is the token id that should be checked for being special.
	 * @return true if the TokenId indicates that this token is a "special"
	 * token.
	 */
	static bool isSpecial(TokenId id) {return id > Tokens::MaxTokenId; }

	/**
	 * Returns the name of the token -- which is either its content or the name
	 * of the special token (if it is one).
	 *
	 * @return the human readable name of this token instance.
	 */
	std::string name() const;

	/**
	 * Returns the name of the special token or an empty string if it is not a
	 * special token.
	 *
	 * @param id
	 */
	static const char* specialName(TokenId id);

	/**
	 * The getLocation function allows the tokens to be directly passed as
	 * parameter to Logger or LoggableException instances.
	 *
	 * @return a reference at the location field
	 */
	const SourceLocation &getLocation() const { return location; }
};
}

#endif /* _OUSIA_TOKENS_HPP_ */
