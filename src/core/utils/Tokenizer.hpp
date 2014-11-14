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

#ifndef _OUSIA_UTILS_TOKENIZER_HPP_
#define _OUSIA_UTILS_TOKENIZER_HPP_

#include <istream>
#include <map>
#include <deque>

#include "BufferedCharReader.hpp"

namespace ousia {
namespace utils {

class TokenizerException : public std::exception {
public:
	const std::string msg;

	TokenizerException(const std::string &msg) : msg(msg){};

	virtual const char *what() const noexcept override { return msg.c_str(); }
};

class TokenTreeNode {
public:
	const std::map<char, TokenTreeNode> children;
	const int tokenId;

	TokenTreeNode(const std::map<std::string, int> &inputs);
};

static const int TOKEN_NONE = -1;
static const int TOKEN_TEXT = -2;

struct Token {
	int tokenId;
	std::string content;
	int startColumn;
	int startLine;
	int endColumn;
	int endLine;

	Token(int tokenId, std::string content, int startColumn, int startLine,
	      int endColumn, int endLine)
	    : tokenId(tokenId),
	      content(content),
	      startColumn(startColumn),
	      startLine(startLine),
	      endColumn(endColumn),
	      endLine(endLine)
	{
	}

	Token() : tokenId(TOKEN_NONE) {}
};

class Tokenizer {
private:
	BufferedCharReader &input;
	const TokenTreeNode &root;
	std::deque<Token> peeked;
	unsigned int peekCursor = 0;

	bool prepare();

protected:
	/**
	* This method is an interface to build multiple tokens from a single one in
	* derived classes. This might be interesting if you want to implement
	* further logic on text tokens or similar applications.
	*
	* @param t a Token the "basic" tokenizer found.
	* @param peeked a reference to the deque containing all temporary Tokens.
	* You are supposed to append your tokens there. In the trivial case you just
	* put the given Token on top of the deque.
	* @return false if no token was appended to the deque (meaning that you want
	* to ignore the given token explicitly) and true in all other cases.
	*/
	virtual bool doPrepare(const Token &t, std::deque<Token> &peeked);

public:
	Tokenizer(BufferedCharReader &input, const TokenTreeNode &root);

	bool next(Token &t);

	bool peek(Token &t);

	void resetPeek();

	void consumePeek();
};
}
}

#endif
