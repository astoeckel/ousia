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

#include <cassert>

#include "CodeTokenizer.hpp"

namespace ousia {
namespace utils {

Token CodeTokenizer::constructToken(const Token& t)
{
	std::string content = buf.str();
	buf.str(std::string());
	return Token{returnTokenId,          content,
	             startToken.startColumn, startToken.startLine,
	             t.endColumn,     t.endLine};
}

void CodeTokenizer::buffer(const Token &t) { buf << t.content; }

bool CodeTokenizer::doPrepare(const Token &t, std::deque<Token> &peeked)
{
	if (t.startLine != t.endLine) {
		throw TokenizerException(
		    "We did not expect a multiline token. Most likely you did not add "
		    "a linebreak token to your tokenizer!");
	}

	auto it = descriptors.find(t.tokenId);
	CodeTokenMode mode = CodeTokenMode::NONE;
	if (it != descriptors.end()) {
		mode = it->second.mode;
	}
	switch (state) {
		case CodeTokenizerState::NORMAL:
			switch (mode) {
				case CodeTokenMode::STRING_START_END:
					state = CodeTokenizerState::IN_STRING;
					break;
				case CodeTokenMode::BLOCK_COMMENT_START:
					state = CodeTokenizerState::IN_BLOCK_COMMENT;
					break;
				case CodeTokenMode::LINE_COMMENT:
					state = CodeTokenizerState::IN_LINE_COMMENT;
					break;
				default:
					if (t.tokenId == TOKEN_TEXT) {
						int begin = -1;
						for (size_t c = 0; c < t.content.length(); c++) {
							bool isWhitespace =
							    t.content[c] == ' ' || t.content[c] == '\t';
							if (begin >= 0 && isWhitespace) {
								peeked.push_back(Token{
								    TOKEN_TEXT,
								    t.content.substr(begin, (int)c - begin),
								    t.startColumn + begin, t.startLine,
								    t.startColumn + (int)c, t.endLine});
							}
							if (!isWhitespace && begin < 0) {
								begin = c;
							}
						}
					}
					peeked.push_back(t);
					return true;
			}
			startToken = t;
			returnTokenId = it->second.id;
			return false;
		case CodeTokenizerState::IN_LINE_COMMENT:
			switch (mode) {
				case CodeTokenMode::LINEBREAK:
					state = CodeTokenizerState::NORMAL;
					if (!ignoreComments) {
						peeked.push_back(constructToken(t));
					}
					return !ignoreComments;
				default:
					if (!ignoreComments) {
						buffer(t);
					}
					return false;
			}
		case CodeTokenizerState::IN_BLOCK_COMMENT:
			switch (mode) {
				case CodeTokenMode::BLOCK_COMMENT_END:
					state = CodeTokenizerState::NORMAL;
					if (!ignoreComments) {
						peeked.push_back(constructToken(t));
					}
					return !ignoreComments;
				default:
					if (!ignoreComments) {
						buffer(t);
					}
					return false;
			}
		case CodeTokenizerState::IN_STRING:
			switch (mode) {
				case CodeTokenMode::ESCAPE:
					if (escaped) {
						buffer(t);
					}
					escaped = !escaped;
					return false;
				case CodeTokenMode::STRING_START_END:
					if (escaped) {
						buffer(t);
						escaped = false;
						return false;
					} else {
						peeked.push_back(constructToken(t));
						state = CodeTokenizerState::NORMAL;
						return true;
					}
				default:
					if (escaped) {
						// TODO: handle escaped characters?
						escaped = false;
					}
					buffer(t);
					return false;
			}
	}
	assert(false);
}
}
}
