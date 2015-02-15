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
#include <core/common/WhitespaceHandler.hpp>

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
	 * Current length of the data within the text handler. The text buffer needs
	 * to be trimmed to this length if this token matches.
	 */
	size_t textLength;

	/**
	 * End location of the current text handler. This location needs to be used
	 * for the text token that is emitted before the actual token.
	 */
	size_t textEnd;

	/**
	 * Constructor of the TokenMatch class.
	 */
	TokenMatch() : textLength(0), textEnd(0) {}

	/**
	 * Returns true if this TokenMatch instance actually represents a match.
	 */
	bool hasMatch() { return token.type != EmptyToken; }
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
	 * Current length of the data within the text handler. The text buffer needs
	 * to be trimmed to this length if this token matches.
	 */
	size_t textLength;

	/**
	 * End location of the current text handler. This location needs to be used
	 * for the text token that is emitted before the actual token.
	 */
	size_t textEnd;

public:
	/**
	 * Constructor of the TokenLookup class.
	 *
	 * @param node is the current node.
	 * @param start is the start position.
	 * @param textLength is the text buffer length of the previous text token.
	 * @param textEnd is the current end location of the previous text token.
	 */
	TokenLookup(const TokenTrie::Node *node, size_t start, size_t textLength,
	            size_t textEnd)
	    : node(node), start(start), textLength(textLength), textEnd(textEnd)
	{
	}

	/**
	 * Tries to extend the current path in the token trie with the given
	 * character. If a complete token is matched, stores this match in the
	 * tokens list (in case it is longer than any previous token).
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
	 */
	void advance(char c, std::vector<TokenLookup> &lookups, TokenMatch &match,
	             const std::vector<std::string> &tokens, SourceOffset end,
	             SourceId sourceId)
	{
		// Check whether we can continue the current token path with the given
		// character without visiting an already visited node
		auto it = node->children.find(c);
		if (it == node->children.end()) {
			return;
		}

		// Check whether the new node represents a complete token a whether it
		// is longer than the current token. If yes, replace the current token.
		node = it->second.get();
		if (node->type != EmptyToken) {
			const std::string &str = tokens[node->type];
			size_t len = str.size();
			if (len > match.token.content.size()) {
				match.token =
				    Token{node->type, str, {sourceId, start, end}};
				match.textLength = textLength;
				match.textEnd = textEnd;
			}
		}

		// If this state can possibly be advanced, store it in the states list.
		if (!node->children.empty()) {
			lookups.emplace_back(*this);
		}
	}
};

/**
 * Transforms the given token into a text token containing the extracted
 * text.
 *
 * @param handler is the WhitespaceHandler containing the collected data.
 * @param token is the output token to which the text should be written.
 * @param sourceId is the source id of the underlying file.
 */
static void buildTextToken(const WhitespaceHandler &handler, TokenMatch &match,
                           SourceId sourceId)
{
	if (match.hasMatch()) {
		match.token.content =
		    std::string{handler.textBuf.data(), match.textLength};
		match.token.location =
		    SourceLocation{sourceId, handler.textStart, match.textEnd};
	} else {
		match.token.content = handler.toString();
		match.token.location =
		    SourceLocation{sourceId, handler.textStart, handler.textEnd};
	}
	match.token.type = TextToken;
}
}

/* Class Tokenizer */

Tokenizer::Tokenizer(WhitespaceMode whitespaceMode)
    : whitespaceMode(whitespaceMode), nextTokenTypeId(0)
{
}

template <typename TextHandler, bool read>
bool Tokenizer::next(CharReader &reader, Token &token)
{
	// If we're in the read mode, reset the char reader peek position to the
	// current read position
	if (read) {
		reader.resetPeek();
	}

	// Prepare the lookups in the token trie
	const TokenTrie::Node *root = trie.getRoot();
	TokenMatch match;
	std::vector<TokenLookup> lookups;
	std::vector<TokenLookup> nextLookups;

	// Instantiate the text handler
	TextHandler textHandler;

	// Peek characters from the reader and try to advance the current token tree
	// cursor
	char c;
	size_t charStart = reader.getPeekOffset();
	const SourceId sourceId = reader.getSourceId();
	while (reader.peek(c)) {
		const size_t charEnd = reader.getPeekOffset();
		const size_t textLength = textHandler.textBuf.size();
		const size_t textEnd = textHandler.textEnd;

		// If we do not have a match yet, start a new lookup from the root
		if (!match.hasMatch()) {
			TokenLookup{root, charStart, textLength, textEnd}.advance(
			    c, nextLookups, match, tokens, charEnd, sourceId);
		}

		// Try to advance all other lookups with the new character
		for (TokenLookup &lookup : lookups) {
			lookup.advance(c, nextLookups, match, tokens, charEnd, sourceId);
		}

		// We have found a token and there are no more states to advance or the
		// text handler has found something -- abort to return the new token
		if (match.hasMatch()) {
			if ((nextLookups.empty() || textHandler.hasText())) {
				break;
			}
		} else {
			// Record all incomming characters
			textHandler.append(c, charStart, charEnd);
		}

		// Swap the lookups and the nextLookups list
		lookups = std::move(nextLookups);
		nextLookups.clear();

		// Advance the offset
		charStart = charEnd;
	}

	// If we found text, emit that text
	if (textHandler.hasText() && (!match.hasMatch() || match.textLength > 0)) {
		buildTextToken(textHandler, match, sourceId);
	}

	// Move the read/peek cursor to the end of the token, abort if an error
	// happens while doing so
	if (match.hasMatch()) {
		// Make sure we have a valid location
		if (match.token.location.getEnd() == InvalidSourceOffset) {
			throw OusiaException{"Token end position offset out of range"};
		}

		// Seek to the end of the current token
		const size_t end = match.token.location.getEnd();
		if (read) {
			reader.seek(end);
		} else {
			reader.seekPeekCursor(end);
		}
		token = match.token;
	} else {
		token = Token{};
	}
	return match.hasMatch();
}

bool Tokenizer::read(CharReader &reader, Token &token)
{
	switch (whitespaceMode) {
		case WhitespaceMode::PRESERVE:
			return next<PreservingWhitespaceHandler, true>(reader, token);
		case WhitespaceMode::TRIM:
			return next<TrimmingWhitespaceHandler, true>(reader, token);
		case WhitespaceMode::COLLAPSE:
			return next<CollapsingWhitespaceHandler, true>(reader, token);
	}
	return false;
}

bool Tokenizer::peek(CharReader &reader, Token &token)
{
	switch (whitespaceMode) {
		case WhitespaceMode::PRESERVE:
			return next<PreservingWhitespaceHandler, false>(reader, token);
		case WhitespaceMode::TRIM:
			return next<TrimmingWhitespaceHandler, false>(reader, token);
		case WhitespaceMode::COLLAPSE:
			return next<CollapsingWhitespaceHandler, false>(reader, token);
	}
	return false;
}

TokenTypeId Tokenizer::registerToken(const std::string &token)
{
	// Abort if an empty token should be registered
	if (token.empty()) {
		return EmptyToken;
	}

	// Search for a new slot in the tokens list
	TokenTypeId type = EmptyToken;
	for (size_t i = nextTokenTypeId; i < tokens.size(); i++) {
		if (tokens[i].empty()) {
			tokens[i] = token;
			type = i;
			break;
		}
	}

	// No existing slot was found, add a new one -- make sure we do not
	// override the special token type handles
	if (type == EmptyToken) {
		type = tokens.size();
		if (type == TextToken || type == EmptyToken) {
			throw OusiaException{"Token type ids depleted!"};
		}
		tokens.emplace_back(token);
	}
	nextTokenTypeId = type + 1;

	// Try to register the token in the trie -- if this fails, remove it
	// from the tokens list
	if (!trie.registerToken(token, type)) {
		tokens[type] = std::string{};
		nextTokenTypeId = type;
		return EmptyToken;
	}
	return type;
}

bool Tokenizer::unregisterToken(TokenTypeId type)
{
	// Unregister the token from the trie, abort if an invalid type is given
	if (type < tokens.size() && trie.unregisterToken(tokens[type])) {
		tokens[type] = std::string{};
		nextTokenTypeId = type;
		return true;
	}
	return false;
}

std::string Tokenizer::getTokenString(TokenTypeId type)
{
	if (type < tokens.size()) {
		return tokens[type];
	}
	return std::string{};
}

void Tokenizer::setWhitespaceMode(WhitespaceMode mode)
{
	whitespaceMode = mode;
}

WhitespaceMode Tokenizer::getWhitespaceMode() { return whitespaceMode; }

/* Explicitly instantiate all possible instantiations of the "next" member
   function */
template bool Tokenizer::next<PreservingWhitespaceHandler, false>(
    CharReader &reader, Token &token);
template bool Tokenizer::next<TrimmingWhitespaceHandler, false>(
    CharReader &reader, Token &token);
template bool Tokenizer::next<CollapsingWhitespaceHandler, false>(
    CharReader &reader, Token &token);
template bool Tokenizer::next<PreservingWhitespaceHandler, true>(
    CharReader &reader, Token &token);
template bool Tokenizer::next<TrimmingWhitespaceHandler, true>(
    CharReader &reader, Token &token);
template bool Tokenizer::next<CollapsingWhitespaceHandler, true>(
    CharReader &reader, Token &token);
}

