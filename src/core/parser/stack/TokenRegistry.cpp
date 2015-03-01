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

#include "Callbacks.hpp"
#include "TokenRegistry.hpp"

namespace ousia {
namespace parser_stack {

TokenRegistry::~TokenRegistry()
{
	for (const auto &tid: tokenIds) {
		parser.unregisterToken(tid.first);
	}
}

TokenId TokenRegistry::registerToken(const std::string &token)
{
	// Check whether the given token is already registered
	auto it = tokens.find(token);
	if (it != tokens.end()) {
		// Increment the reference count
		size_t &refCount = it->second.second;
		refCount++;

		// Return the token id
		return it->second.first;
	}

	// Register the token in the parser
	TokenId id = parser.registerToken(token);
	tokens[token] = std::pair<TokenId, size_t>(id, 1);
	tokenIds[id] = token;
	return id;
}

void TokenRegistry::unregisterToken(TokenId id)
{
	// Lookup the token corresponding to the given token id
	auto tokenIt = tokenIds.find(id);
	if (tokenIt != tokenIds.end()) {
		const std::string &token = tokenIt->second;
		// Lookup the reference count for the corresponding token
		auto idIt = tokens.find(token);
		if (idIt != tokens.end()) {
			// Decrement the reference count, abort if the refCount is larger
			// than zero
			size_t &refCount = idIt->second.second;
			refCount--;
			if (refCount > 0) {
				return;
			}

			// Unregister the token from the parser
			parser.unregisterToken(id);

			// Unregister the token from the internal tokens map
			tokens.erase(token);
		}
		// Unregister the token from the internal id map
		tokenIds.erase(id);
	}
}
}
}
