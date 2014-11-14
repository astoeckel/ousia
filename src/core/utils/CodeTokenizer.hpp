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

#ifndef _OUSIA_UTILS_CODE_TOKENIZER_HPP_
#define _OUSIA_UTILS_CODE_TOKENIZER_HPP_

#include <map>
#include <sstream>

#include "BufferedCharReader.hpp"
#include "Tokenizer.hpp"

namespace ousia {
namespace utils {

enum class CodeTokenMode {
	STRING_START_END,
	LINE_COMMENT,
	BLOCK_COMMENT_START,
	BLOCK_COMMENT_END,
	LINEBREAK,
	ESCAPE,
	NONE
};

struct CodeTokenDescriptor {
	CodeTokenMode mode;
	int id;

	CodeTokenDescriptor(CodeTokenMode mode, int id) : mode(mode), id(id) {}
};


enum class CodeTokenizerState {
	NORMAL,
	IN_BLOCK_COMMENT,
	IN_LINE_COMMENT,
	IN_STRING
};

class CodeTokenizer : public Tokenizer {
private:
	std::map<int, CodeTokenDescriptor> descriptors;
	CodeTokenizerState state;
	std::stringstream buf;
	Token startToken;
	int returnTokenId;
	bool escaped = false;

	Token constructToken(const Token& t);
	void buffer(const Token& t);

protected:
	bool doPrepare(const Token &t, std::deque<Token> &peeked) override;

public:
	bool ignoreComments = false;

	CodeTokenizer(BufferedCharReader &input, const TokenTreeNode &root,
	          std::map<int, CodeTokenDescriptor> descriptors)
	    : Tokenizer(input, root), descriptors(descriptors)
	{
	}
};
}
}

#endif
