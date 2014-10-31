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
#include <queue>

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

struct Token {
	const int tokenId;
	const std::string content;
	const int column;
	const int line;

	Token(int tokenId, std::string content, int column, int line)
	    : tokenId(tokenId), content(content), column(column), line(line)
	{
	}
};

class Tokenizer {
private:
	const std::istream &input;
	const TokenTreeNode root;
	const std::queue<Token> peekQueue;

public:
	Tokenizer(const TokenTreeNode &root, std::istream &input);

	bool hasNext();

	const Token &next();

	const Token &peek();

	void reset();
};
}
}

#endif
