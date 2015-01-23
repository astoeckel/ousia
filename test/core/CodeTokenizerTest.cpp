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
	CharReader reader{
	    "/**\n"                                 // 1
	    " * Some Block Comment\n"               // 2
	    " */\n"                                 // 3
	    "var my_string = 'My \\'String\\'';\n"  // 4
	    "// and a line comment\n"               // 5
	    "var my_obj = { a = 4;}", 0};              // 6
	//   123456789012345678901234567890123456789
	//   0        1         2         3
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
	    {BLOCK_COMMENT, "*\n * Some Block Comment\n ", SourceLocation{0, 0, 29}},
	    {LINEBREAK, "\n", SourceLocation{0, 29, 30}},
	    {TOKEN_TEXT, "var", SourceLocation{0, 30, 33}},
	    {TOKEN_TEXT, "my_string", SourceLocation{0, 34, 43}},
	    {TOKEN_TEXT, "=", SourceLocation{0, 44, 45}},
	    {STRING, "My 'String'", SourceLocation{0, 46, 61}},
	    {TOKEN_TEXT, ";", SourceLocation{0, 61, 62}},
	    {LINEBREAK, "\n", SourceLocation{0, 62, 63}},
	    // this is slightly counter-intuitive but makes sense if you think about
	    // it: As a line comment is ended by a line break the line break is
	    // technically still a part of the line comment and thus the ending
	    // is in the next line.
	    {LINE_COMMENT, " and a line comment", SourceLocation{0, 63, 85}},
	    {TOKEN_TEXT, "var", SourceLocation{0, 85, 88}},
	    {TOKEN_TEXT, "my_obj", SourceLocation{0, 89, 95}},
	    {TOKEN_TEXT, "=", SourceLocation{0, 96, 97}},
	    {CURLY_OPEN, "{", SourceLocation{0, 98, 99}},
	    {TOKEN_TEXT, "a", SourceLocation{0, 100, 101}},
	    {TOKEN_TEXT, "=", SourceLocation{0, 102, 103}},
	    {TOKEN_TEXT, "4;", SourceLocation{0, 104, 106}},
	    {CURLY_CLOSE, "}", SourceLocation{0, 106, 107}},
	};

	CodeTokenizer tokenizer{reader, root, descriptors};

	Token t;
	for (auto &te : expected) {
		EXPECT_TRUE(tokenizer.next(t));
		EXPECT_EQ(te.tokenId, t.tokenId);
		EXPECT_EQ(te.content, t.content);
		EXPECT_EQ(te.location.getSourceId(), t.location.getSourceId());
		EXPECT_EQ(te.location.getStart(), t.location.getStart());
		EXPECT_EQ(te.location.getEnd(), t.location.getEnd());
	}
	ASSERT_FALSE(tokenizer.next(t));
}
}

