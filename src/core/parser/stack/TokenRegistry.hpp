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
 * @file TokenRegistry.hpp
 *
 * Contains the TokenRegistry class used for registering all user defined tokens
 * during the parsing process.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_TOKEN_REGISTRY_HPP_
#define _OUSIA_PARSER_STACK_TOKEN_REGISTRY_HPP_

#include <string>
#include <unordered_map>

#include <core/common/Token.hpp>

namespace ousia {
namespace parser_stack {

// Forward declarations
class ParserCallbacks;

/**
 * The TokenRegistry class is used for registering all user defined tokens
 * during the Parsing process. The TokenRegistry class acts as an adapter
 * between the parser which allocates a TokenId for each unique token and the
 * Handler classes which may register the same token multiple times and expect
 * the same TokenId to be returned for the same token.
 */
class TokenRegistry  {
private:
	/**
	 * Reference at the ParserCallback instance the tokens are relayed to.
	 */
	ParserCallbacks &parser;

	/**
	 * Store containing all TokenId instances for all registered tokens. The map
	 * maps from the token strings to the corresponding TokenId and a reference
	 * count.
	 */
	std::unordered_map<std::string, std::pair<TokenId, size_t>> tokens;

	/**
	 * Reverse map containing the string corresponding to a TokenId.
	 */
	std::unordered_map<TokenId, std::string> tokenIds;

public:
	/**
	 * Constructor of the TokenRegistry class.
	 *
	 * @param parser is the underlying parser implementing the ParserCallbacks
	 * interface to which all calls are relayed.
	 */
	TokenRegistry(ParserCallbacks &parser) : parser(parser) {}

	/**
	 * Destructor of the TokenRegistry class, removes all registered tokens from
	 * the parser.
	 */
	~TokenRegistry();

	/* No copy construction */
	TokenRegistry(const TokenRegistry &) = delete;

	/* No assignment */
	TokenRegistry &operator=(const TokenRegistry &) = delete;

	/**
	 * Registers the given string token in the underlying parser and returns the
	 * TokenId of that token. If the same token string is given multiple times,
	 * the same TokenId is returned. The token is only registered once in the
	 * parser.
	 *
	 * @param token is the token that should be registered.
	 * @return the TokenId associated with this token.
	 */
	TokenId registerToken(const std::string &token);

	/**
	 * Unregisters the token with the given TokenId from the parser. Note that
	 * the token will only be unregistered if unregisterToken() has been called
	 * as many times as registerToken() for the same token.
	 *
	 * @param id is the id of the token returned by registerToken() that should
	 * be unregistered.
	 */
	void unregisterToken(TokenId id);
};
}
}

#endif /* _OUSIA_PARSER_STACK_TOKEN_REGISTRY_HPP_ */

