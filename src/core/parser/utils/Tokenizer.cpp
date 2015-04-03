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

#include <memory>
#include <vector>

#include <core/common/CharReader.hpp>
#include <core/common/Exceptions.hpp>
#include <core/common/Utils.hpp>

#include "TokenizedData.hpp"
#include "Tokenizer.hpp"

namespace ousia {

namespace {

/* Internal class TokenMatch */

/**
 * Contains information about a matching token.
 */
struct TokenMatch {
	/**
	 * Token that was matched.
	 */
	Token token;

	/**
	 * Position at which this token starts in the TokenizedData instance.
	 */
	size_t dataStartOffset;

	/**
	 * Set to true if the matched token is a primary token.
	 */
	bool primary;

	/**
	 * Constructor of the TokenMatch class.
	 */
	TokenMatch() : dataStartOffset(0), primary(false) {}

	/**
	 * Returns true if this TokenMatch instance actually represents a match.
	 *
	 * @return true if the TokenMatch actually has a match.
	 */
	bool hasMatch() const { return token.id != Tokens::Empty; }

	/**
	 * Returns the length of the matched token.
	 *
	 * @return the length of the token string.
	 */
	size_t size() const { return token.content.size(); }
};

/* Internal class TokenLookup */

/**
 * The TokenLookup class is used to represent a thread in a running token
 * lookup.
 */
class TokenLookup {
private:
	/**
	 * Current node within the token trie.
	 */
	TokenTrie::Node const *node;

	/**
	 * Start offset within the source file.
	 */
	size_t start;

	/**
	 * Position at which this token starts in the TokenizedData instance.
	 */
	size_t dataStartOffset;

public:
	/**
	 * Constructor of the TokenLookup class.
	 *
	 * @param node is the current node.
	 * @param start is the start position in the source file.
	 * @param dataStartOffset is the current length of the TokenizedData buffer.
	 */
	TokenLookup(const TokenTrie::Node *node, size_t start,
	            size_t dataStartOffset)
	    : node(node), start(start), dataStartOffset(dataStartOffset)
	{
	}

	/**
	 * Tries to extend the current path in the token trie with the given
	 * character. If a complete token is matched, stores the match in the given
	 * TokenMatch reference and returns true.
	 *
	 * @param c is the character that should be appended to the current prefix.
	 * @param lookups is a list to which new TokeLookup instances are added --
	 * which could potentially be expanded in the next iteration.
	 * @param match is the Token instance to which the matching token
	 * should be written.
	 * @param tokens is a reference at the internal token list of the
	 * Tokenizer.
	 * @param end is the end byte offset of the current character.
	 * @param sourceId is the source if of this file.
	 * @return true if a token was matched, false otherwise.
	 */
	bool advance(char c, std::vector<TokenLookup> &lookups, TokenMatch &match,
	             const std::vector<Tokenizer::TokenDescriptor> &tokens,
	             SourceOffset end, SourceId sourceId)
	{
		// Set to true once a token has been matched
		bool res = false;

		// Check whether we can continue the current token path, if not, abort
		auto it = node->children.find(c);
		if (it == node->children.end()) {
			return res;
		}

		// Check whether the new node represents a complete token and whether it
		// is longer than the current token. If yes, replace the current token.
		node = it->second.get();
		if (node->id != Tokens::Empty) {
			const Tokenizer::TokenDescriptor &descr = tokens[node->id];
			match.token = Token(node->id, descr.string,
			                    SourceLocation(sourceId, start, end));
			match.dataStartOffset = dataStartOffset;
			match.primary = descr.primary;
			res = true;
		}

		// If this state can possibly be advanced, store it in the states list.
		if (!node->children.empty()) {
			lookups.emplace_back(*this);
		}
		return res;
	}
};
}

/* Class Tokenizer */

Tokenizer::Tokenizer() : nextTokenId(0) {}

template <bool tRead>
bool Tokenizer::next(CharReader &reader, Token &token,
                     TokenizedData &data) const
{
	// If we're in the read mode, reset the char reader peek position to the
	// current read position
	if (tRead) {
		reader.resetPeek();
	}

	// Prepare the lookups in the token trie
	const TokenTrie::Node *root = trie.getRoot();
	TokenMatch bestMatch;
	std::vector<TokenLookup> lookups;
	std::vector<TokenLookup> nextLookups;

	// Peek characters from the reader and try to advance the current token tree
	// cursor
	char c;
	const size_t initialDataSize = data.size();
	size_t charStart = reader.getPeekOffset();
	const SourceId sourceId = reader.getSourceId();
	while (reader.peek(c)) {
		const size_t charEnd = reader.getPeekOffset();
		const size_t dataStartOffset = data.size();

		// If we do not have a match yet, start a new lookup from the root
		if (!bestMatch.hasMatch() || !bestMatch.primary) {
			lookups.emplace_back(root, charStart, dataStartOffset);
		}

		// Try to advance all other lookups with the new character
		TokenMatch match;
		for (TokenLookup &lookup : lookups) {
			// Continue if the current lookup
			if (!lookup.advance(c, nextLookups, match, tokens, charEnd,
			                    sourceId)) {
				continue;
			}

			// Replace the best match with longest token
			if (match.size() > bestMatch.size()) {
				bestMatch = match;
			}

			// If the matched token is a non-primary token -- mark the match in
			// the TokenizedData list
			if (!match.primary) {
				data.mark(match.token.id, data.size() - match.size() + 1,
				          match.size());
			}
		}

		// If a token has been found and the token is a primary token, check
		// whether we have to abort, otherwise if we have a non-primary match,
		// reset it once it can no longer be advanced
		if (bestMatch.hasMatch() && nextLookups.empty()) {
			if (bestMatch.primary) {
				break;
			} else {
				bestMatch = TokenMatch{};
			}
		}

		// Record all incomming characters
		data.append(c, charStart, charEnd);

		// Swap the lookups and the nextLookups list
		lookups = std::move(nextLookups);
		nextLookups.clear();

		// Advance the offset
		charStart = charEnd;
	}

	// If we found data, emit a corresponding data token
	if (data.size() > initialDataSize &&
	    (!bestMatch.hasMatch() || !bestMatch.primary ||
	     bestMatch.dataStartOffset > initialDataSize)) {
		// If we have a "bestMatch" wich starts after text data has started,
		// trim the TokenizedData to this offset
		if (bestMatch.dataStartOffset > initialDataSize && bestMatch.primary) {
			data.trim(bestMatch.dataStartOffset);
		}

		// Create a token containing the data location
		bestMatch.token = Token{data.getLocation()};
	} else if (bestMatch.hasMatch() && bestMatch.primary &&
	           bestMatch.dataStartOffset == initialDataSize) {
		data.trim(initialDataSize);
	}

	// Move the read/peek cursor to the end of the token, abort if an error
	// happens while doing so
	if (bestMatch.hasMatch()) {
		// Make sure we have a valid location
		if (bestMatch.token.location.getEnd() == InvalidSourceOffset) {
			throw OusiaException{"Token end position offset out of range"};
		}

		// Seek to the end of the current token
		const size_t end = bestMatch.token.location.getEnd();
		if (tRead) {
			reader.seek(end);
		} else {
			reader.seekPeekCursor(end);
		}

		token = bestMatch.token;
	} else {
		token = Token{};
	}
	return bestMatch.hasMatch();
}

bool Tokenizer::read(CharReader &reader, Token &token,
                     TokenizedData &data) const
{
	return next<true>(reader, token, data);
}

bool Tokenizer::peek(CharReader &reader, Token &token,
                     TokenizedData &data) const
{
	return next<false>(reader, token, data);
}

TokenId Tokenizer::registerToken(const std::string &token, bool primary)
{
	// Abort if an empty token should be registered
	if (token.empty()) {
		return Tokens::Empty;
	}

	// Search for a new slot in the tokens list
	TokenId type = Tokens::Empty;
	for (size_t i = nextTokenId; i < tokens.size(); i++) {
		if (!tokens[i].valid()) {
			tokens[i] = TokenDescriptor(token, primary);
			type = i;
			break;
		}
	}

	// No existing slot was found, add a new one -- make sure we do not
	// override the special token type handles
	if (type == Tokens::Empty) {
		type = tokens.size();
		if (type >= Tokens::MaxTokenId) {
			throw OusiaException{"Token type ids depleted!"};
		}
		tokens.emplace_back(token, primary);
	}
	nextTokenId = type + 1;

	// Try to register the token in the trie -- if this fails, remove it from
	// the tokens list
	if (!trie.registerToken(token, type)) {
		tokens[type] = TokenDescriptor();
		nextTokenId = type;
		return Tokens::Empty;
	}
	return type;
}

bool Tokenizer::unregisterToken(TokenId id)
{
	// Unregister the token from the trie, abort if an invalid type is given
	if (id < tokens.size() && trie.unregisterToken(tokens[id].string)) {
		tokens[id] = TokenDescriptor();
		nextTokenId = id;
		return true;
	}
	return false;
}

static Tokenizer::TokenDescriptor EmptyTokenDescriptor;

const Tokenizer::TokenDescriptor &Tokenizer::lookupToken(TokenId id) const
{
	if (id < tokens.size()) {
		return tokens[id];
	}
	return EmptyTokenDescriptor;
}

/* Explicitly instantiate all possible instantiations of the "next" member
   function */
template bool Tokenizer::next<false>(CharReader &, Token &,
                                     TokenizedData &) const;
template bool Tokenizer::next<true>(CharReader &, Token &,
                                    TokenizedData &) const;
}

