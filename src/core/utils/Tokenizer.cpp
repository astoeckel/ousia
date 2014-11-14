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

#include <sstream>

#include "Tokenizer.hpp"

namespace ousia {
namespace utils {

static std::map<char, TokenTreeNode> buildChildren(
    const std::map<std::string, int> &inputs)
{
	std::map<char, TokenTreeNode> children;
	std::map<char, std::map<std::string, int>> nexts;

	for (auto &e : inputs) {
		const std::string &s = e.first;
		const int id = e.second;
		if (s.empty()) {
			continue;
		}
		char start = s[0];
		const std::string suffix = s.substr(1);
		if (nexts.find(start) != nexts.end()) {
			nexts[start].insert(std::make_pair(suffix, id));
		} else {
			nexts.insert(std::make_pair(
			    start, std::map<std::string, int>{{suffix, id}}));
		}
	}

	for (auto &n : nexts) {
		children.insert(std::make_pair(n.first, TokenTreeNode{n.second}));
	}

	return children;
}

static int buildId(const std::map<std::string, int> &inputs)
{
	int tokenId = TOKEN_NONE;
	for (auto &e : inputs) {
		if (e.first.empty()) {
			if (tokenId != TOKEN_NONE) {
				throw TokenizerException{std::string{"Ambigous token found: "} +
				                         std::to_string(e.second)};
			} else {
				tokenId = e.second;
			}
		}
	}
	return tokenId;
}

TokenTreeNode::TokenTreeNode(const std::map<std::string, int> &inputs)
    : children(buildChildren(inputs)), tokenId(buildId(inputs))
{
}

Tokenizer::Tokenizer(BufferedCharReader &input, const TokenTreeNode &root)
    : input(input), root(root)
{
}

bool Tokenizer::prepare()
{
	std::stringstream buffer;
	char c;
	const int startColumn = input.getColumn();
	const int startLine = input.getLine();
	bool bufEmpty = true;
	while (input.peek(&c)) {
		if (root.children.find(c) != root.children.end()) {
			// if there might be a special token, keep peeking forward
			// until we find the token (or we don't).
			TokenTreeNode const *n = &root;
			std::stringstream tBuf;
			int match = TOKEN_NONE;
			while (true) {
				tBuf << c;
				n = &(n->children.at(c));
				if (n->tokenId != TOKEN_NONE) {
					match = n->tokenId;
					// from here on we found a token. If we have something
					// in our buffer already, we end the search now.
					if (!bufEmpty) {
						break;
					} else {
						// if we want to return this token ( = we have nothing
						// in our buffer yet) we look greedily for the longest
						// possible token we can construct.
						input.consumePeek();
					}
				}
				if (!input.peek(&c)) {
					// if we are at the end we break off the search.
					break;
				}
				if (n->children.find(c) == n->children.end()) {
					// if we do not find a possible continuation anymore,
					// break off the search.
					break;
				}
			}
			// check if we did indeed find a special token.
			if (match != TOKEN_NONE) {
				input.resetPeek();
				if (bufEmpty) {
					// if we did not have text before, construct that token.
					if (doPrepare(
					        Token{match, tBuf.str(), startColumn, startLine,
					              input.getColumn(), input.getLine()},
					        peeked)) {
						return true;
					}

				} else {
					// otherwise we return the text before the token.
					if (doPrepare(Token{TOKEN_TEXT, buffer.str(), startColumn,
					                    startLine, input.getColumn(),
					                    input.getLine()},
					              peeked)) {
						return true;
					}
				}
			}
		}
		buffer << c;
		bufEmpty = false;
		input.consumePeek();
	}
	if (!bufEmpty) {
		return doPrepare(Token{TOKEN_TEXT, buffer.str(), startColumn, startLine,
		                       input.getColumn(), input.getLine()},
		                 peeked);
	}
	return false;
}

bool Tokenizer::doPrepare(const Token &t, std::deque<Token> &peeked)
{
	peeked.push_back(t);
	return true;
}

bool Tokenizer::next(Token &t)
{
	if (peeked.empty()) {
		if (!prepare()) {
			return false;
		}
	}
	t = peeked.front();
	peeked.pop_front();
	resetPeek();
	return true;
}

bool Tokenizer::peek(Token &t)
{
	if (peekCursor >= peeked.size()) {
		if (!prepare()) {
			return false;
		}
	}
	t = peeked[peekCursor];
	return true;
}

void Tokenizer::resetPeek() { peekCursor = 0; }

void Tokenizer::consumePeek()
{
	while (peekCursor > 0) {
		peeked.pop_front();
		peekCursor--;
	}
}
}
}
