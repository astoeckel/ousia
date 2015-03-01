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
 * Contains the TokenRegistry class used for registering all possible tokens
 * during the parsing process.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_TOKEN_REGISTRY_HPP_
#define _OUSIA_PARSER_STACK_TOKEN_REGISTRY_HPP_

#include <string>
#include <unordered_map>

#include "Callbacks.hpp"

namespace ousia {
namespace parser_stack {

/**
 * The TokenRegistry class is used for registering all possible tokens during
 * the Parsing process. The TokenRegistry class acts as an adapter between the
 * parser which allocates TokenId for each unique token and the Handler classes
 * which may register tokens multiple times and expect the same TokenId to be
 * returned for the same token.
 */
class TokenRegistry : public ParserCallbacks {
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

	/* No copy construction */
	TokenRegistry(const TokenRegistry &) = delete;

	/* No assignment */
	TokenRegistry &operator=(const TokenRegistry &) = delete;

	TokenId registerToken(const std::string &token) override;
	void unregisterToken(TokenId id) override;
};
}
}

#endif /* _OUSIA_PARSER_STACK_TOKEN_REGISTRY_HPP_ */

