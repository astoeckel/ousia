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

#include <gtest/gtest.h>

#include <core/CodeTokenizer.hpp>

namespace ousia {

static const int BLOCK_COMMENT = 30;
static const int LINE_COMMENT = 31;
static const int STRING = 20;
static const int ESCAPE = 21;
static const int LINEBREAK = 21;
static const int CURLY_OPEN = 40;
static const int CURLY_CLOSE = 41;

TEST(CodeTokenizer, testTokenizer)
{
	BufferedCharReader reader;
	reader.feed("/**\n");                                 // 1
	reader.feed(" * Some Block Comment\n");               // 2
	reader.feed(" */\n");                                 // 3
	reader.feed("var my_string = 'My \\'String\\'';\n");  // 4
	reader.feed("// and a line comment\n");               // 5
	reader.feed("var my_obj = { a = 4;}");                // 6
	//           123456789012345678901234567890123456789
	//           0        1         2         3
	TokenTreeNode root{{{"/*", 1},
	                    {"*/", 2},
	                    {"//", 3},
	                    {"'", 4},
	                    {"\\", 5},
	                    {"{", CURLY_OPEN},
	                    {"}", CURLY_CLOSE},
	                    {"\n", 6}}};
	std::map<int, CodeTokenDescriptor> descriptors{
	    // the block comment start Token has the id 1 and if the Tokenizer
	    // returns a Block Comment Token that should have the id 10.
	    {1, {CodeTokenMode::BLOCK_COMMENT_START, BLOCK_COMMENT}},
	    {2, {CodeTokenMode::BLOCK_COMMENT_END, BLOCK_COMMENT}},
	    {3, {CodeTokenMode::LINE_COMMENT, LINE_COMMENT}},
	    {4, {CodeTokenMode::STRING_START_END, STRING}},
	    {5, {CodeTokenMode::ESCAPE, ESCAPE}},
	    {6, {CodeTokenMode::LINEBREAK, LINEBREAK}}};

	std::vector<Token> expected = {
	    {BLOCK_COMMENT, "*\n * Some Block Comment\n ", 1, 1, 4, 3},
	    {LINEBREAK, "\n", 4, 3, 1, 4},
	    {TOKEN_TEXT, "var", 1, 4, 4, 4},
	    {TOKEN_TEXT, "my_string", 5, 4, 14, 4},
	    {TOKEN_TEXT, "=", 15, 4, 16, 4},
	    {STRING, "My 'String'", 17, 4, 32, 4},
	    {TOKEN_TEXT, ";", 32, 4, 33, 4},
	    {LINEBREAK, "\n", 33, 4, 1, 5},
		//this is slightly counter-intuitive but makes sense if you think about
		//it: As a line comment is ended by a line break the line break is
		//technically still a part of the line comment and thus the ending
		//is in the next line.
	    {LINE_COMMENT, " and a line comment", 1, 5, 1, 6},
	    {TOKEN_TEXT, "var", 1, 6, 4, 6},
	    {TOKEN_TEXT, "my_obj", 5, 6, 11, 6},
	    {TOKEN_TEXT, "=", 12, 6, 13, 6},
	    {CURLY_OPEN, "{", 14, 6, 15, 6},
	    {TOKEN_TEXT, "a", 16, 6, 17, 6},
	    {TOKEN_TEXT, "=", 18, 6, 19, 6},
	    {TOKEN_TEXT, "4;", 20, 6, 22, 6},
	    {CURLY_CLOSE, "}", 22, 6, 23, 6},
	};

	CodeTokenizer tokenizer{reader, root, descriptors};

	Token t;
	for (auto &te : expected) {
		ASSERT_TRUE(tokenizer.next(t));
		ASSERT_EQ(te.tokenId, t.tokenId);
		ASSERT_EQ(te.content, t.content);
		ASSERT_EQ(te.startColumn, t.startColumn);
		ASSERT_EQ(te.startLine, t.startLine);
		ASSERT_EQ(te.endColumn, t.endColumn);
		ASSERT_EQ(te.endLine, t.endLine);
	}
	ASSERT_FALSE(tokenizer.next(t));
}
}

