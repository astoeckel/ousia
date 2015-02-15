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

#include <gtest/gtest.h>

#include <core/common/CharReader.hpp>
#include <core/parser/utils/Tokenizer.hpp>

namespace ousia {

TEST(Tokenizer, tokenRegistration)
{
	Tokenizer tokenizer;

	ASSERT_EQ(EmptyToken, tokenizer.registerToken(""));

	ASSERT_EQ(0U, tokenizer.registerToken("a"));
	ASSERT_EQ(EmptyToken, tokenizer.registerToken("a"));
	ASSERT_EQ("a", tokenizer.getTokenString(0U));

	ASSERT_EQ(1U, tokenizer.registerToken("b"));
	ASSERT_EQ(EmptyToken, tokenizer.registerToken("b"));
	ASSERT_EQ("b", tokenizer.getTokenString(1U));

	ASSERT_EQ(2U, tokenizer.registerToken("c"));
	ASSERT_EQ(EmptyToken, tokenizer.registerToken("c"));
	ASSERT_EQ("c", tokenizer.getTokenString(2U));

	ASSERT_TRUE(tokenizer.unregisterToken(1U));
	ASSERT_FALSE(tokenizer.unregisterToken(1U));
	ASSERT_EQ("", tokenizer.getTokenString(1U));

	ASSERT_EQ(1U, tokenizer.registerToken("d"));
	ASSERT_EQ(EmptyToken, tokenizer.registerToken("d"));
	ASSERT_EQ("d", tokenizer.getTokenString(1U));
}

TEST(Tokenizer, textTokenPreserveWhitespace)
{
	{
		CharReader reader{" this \t is only a  \n\n test   text   "};
		//                 012345 6789012345678 9 0123456789012345
		//                 0          1           2         3
		Tokenizer tokenizer{WhitespaceMode::PRESERVE};

		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));
		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ(" this \t is only a  \n\n test   text   ", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(36U, loc.getEnd());

		ASSERT_FALSE(tokenizer.read(reader, token));
	}

	{
		CharReader reader{"this \t is only a  \n\n test   text"};
		//                 01234 5678901234567 8 9012345678901
		//                 0          1           2         3
		Tokenizer tokenizer{WhitespaceMode::PRESERVE};

		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));
		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("this \t is only a  \n\n test   text", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(32U, loc.getEnd());

		ASSERT_FALSE(tokenizer.read(reader, token));
	}
}

TEST(Tokenizer, textTokenTrimWhitespace)
{
	{
		CharReader reader{" this \t is only a  \n\n test   text   "};
		//                 012345 6789012345678 9 0123456789012345
		//                 0          1           2         3
		Tokenizer tokenizer{WhitespaceMode::TRIM};

		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));
		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("this \t is only a  \n\n test   text", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(1U, loc.getStart());
		ASSERT_EQ(33U, loc.getEnd());

		ASSERT_FALSE(tokenizer.read(reader, token));
	}

	{
		CharReader reader{"this \t is only a  \n\n test   text"};
		//                 01234 5678901234567 8 9012345678901
		//                 0          1           2         3
		Tokenizer tokenizer{WhitespaceMode::TRIM};

		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));
		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("this \t is only a  \n\n test   text", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(32U, loc.getEnd());

		ASSERT_FALSE(tokenizer.read(reader, token));
	}
}

TEST(Tokenizer, textTokenCollapseWhitespace)
{
	{
		CharReader reader{" this \t is only a  \n\n test   text   "};
		//                 012345 6789012345678 9 0123456789012345
		//                 0          1           2         3
		Tokenizer tokenizer{WhitespaceMode::COLLAPSE};

		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));
		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("this is only a test text", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(1U, loc.getStart());
		ASSERT_EQ(33U, loc.getEnd());

		ASSERT_FALSE(tokenizer.read(reader, token));
	}

	{
		CharReader reader{"this \t is only a  \n\n test   text"};
		//                 01234 5678901234567 8 9012345678901
		//                 0          1           2         3
		Tokenizer tokenizer{WhitespaceMode::COLLAPSE};

		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));
		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("this is only a test text", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(32U, loc.getEnd());

		ASSERT_FALSE(tokenizer.read(reader, token));
	}
}

TEST(Tokenizer, simpleReadToken)
{
	CharReader reader{"test1:test2"};
	Tokenizer tokenizer;

	const TokenTypeId tid = tokenizer.registerToken(":");
	ASSERT_EQ(0U, tid);

	{
		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));

		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("test1", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(5U, loc.getEnd());

		char c;
		ASSERT_TRUE(reader.peek(c));
		ASSERT_EQ(':', c);
	}

	{
		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));

		ASSERT_EQ(tid, token.type);
		ASSERT_EQ(":", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(5U, loc.getStart());
		ASSERT_EQ(6U, loc.getEnd());

		char c;
		ASSERT_TRUE(reader.peek(c));
		ASSERT_EQ('t', c);
	}

	{
		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));

		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("test2", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(6U, loc.getStart());
		ASSERT_EQ(11U, loc.getEnd());

		char c;
		ASSERT_FALSE(reader.peek(c));
	}
}

TEST(Tokenizer, simplePeekToken)
{
	CharReader reader{"test1:test2"};
	Tokenizer tokenizer;

	const TokenTypeId tid = tokenizer.registerToken(":");
	ASSERT_EQ(0U, tid);

	{
		Token token;
		ASSERT_TRUE(tokenizer.peek(reader, token));

		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("test1", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(5U, loc.getEnd());
		ASSERT_EQ(0U, reader.getOffset());
		ASSERT_EQ(5U, reader.getPeekOffset());
	}

	{
		Token token;
		ASSERT_TRUE(tokenizer.peek(reader, token));

		ASSERT_EQ(tid, token.type);
		ASSERT_EQ(":", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(5U, loc.getStart());
		ASSERT_EQ(6U, loc.getEnd());
		ASSERT_EQ(0U, reader.getOffset());
		ASSERT_EQ(6U, reader.getPeekOffset());
	}

	{
		Token token;
		ASSERT_TRUE(tokenizer.peek(reader, token));

		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("test2", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(6U, loc.getStart());
		ASSERT_EQ(11U, loc.getEnd());
		ASSERT_EQ(0U, reader.getOffset());
		ASSERT_EQ(11U, reader.getPeekOffset());
	}

	{
		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));

		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("test1", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(5U, loc.getEnd());
		ASSERT_EQ(5U, reader.getOffset());
		ASSERT_EQ(5U, reader.getPeekOffset());
	}

	{
		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));

		ASSERT_EQ(tid, token.type);
		ASSERT_EQ(":", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(5U, loc.getStart());
		ASSERT_EQ(6U, loc.getEnd());
		ASSERT_EQ(6U, reader.getOffset());
		ASSERT_EQ(6U, reader.getPeekOffset());
	}

	{
		Token token;
		ASSERT_TRUE(tokenizer.read(reader, token));

		ASSERT_EQ(TextToken, token.type);
		ASSERT_EQ("test2", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(6U, loc.getStart());
		ASSERT_EQ(11U, loc.getEnd());
		ASSERT_EQ(11U, reader.getOffset());
		ASSERT_EQ(11U, reader.getPeekOffset());
	}
}

TEST(Tokenizer, ambiguousTokens)
{
	CharReader reader{"abc"};
	Tokenizer tokenizer;

	TokenTypeId t1 = tokenizer.registerToken("abd");
	TokenTypeId t2 = tokenizer.registerToken("bc");

	ASSERT_EQ(0U, t1);
	ASSERT_EQ(1U, t2);

	Token token;
	ASSERT_TRUE(tokenizer.read(reader, token));

	ASSERT_EQ(TextToken, token.type);
	ASSERT_EQ("a", token.content);

	SourceLocation loc = token.location;
	ASSERT_EQ(0U, loc.getStart());
	ASSERT_EQ(1U, loc.getEnd());

	ASSERT_TRUE(tokenizer.read(reader, token));

	ASSERT_EQ(t2, token.type);
	ASSERT_EQ("bc", token.content);

	loc = token.location;
	ASSERT_EQ(1U, loc.getStart());
	ASSERT_EQ(3U, loc.getEnd());

	ASSERT_FALSE(tokenizer.read(reader, token));
}

TEST(Tokenizer, commentTestWhitespacePreserve)
{
	CharReader reader{"Test/Test /* Block Comment */", 0};
	//                 012345678901234567890123456789
	//                 0        1         2
	Tokenizer tokenizer(WhitespaceMode::PRESERVE);

	const TokenTypeId t1 = tokenizer.registerToken("/");
	const TokenTypeId t2 = tokenizer.registerToken("/*");
	const TokenTypeId t3 = tokenizer.registerToken("*/");

	std::vector<Token> expected = {
	    {TextToken, "Test", SourceLocation{0, 0, 4}},
	    {t1, "/", SourceLocation{0, 4, 5}},
	    {TextToken, "Test ", SourceLocation{0, 5, 10}},
	    {t2, "/*", SourceLocation{0, 10, 12}},
	    {TextToken, " Block Comment ", SourceLocation{0, 12, 27}},
	    {t3, "*/", SourceLocation{0, 27, 29}}};

	Token t;
	for (auto &te : expected) {
		EXPECT_TRUE(tokenizer.read(reader, t));
		EXPECT_EQ(te.type, t.type);
		EXPECT_EQ(te.content, t.content);
		EXPECT_EQ(te.location.getSourceId(), t.location.getSourceId());
		EXPECT_EQ(te.location.getStart(), t.location.getStart());
		EXPECT_EQ(te.location.getEnd(), t.location.getEnd());
	}
	ASSERT_FALSE(tokenizer.read(reader, t));
}

TEST(Tokenizer, commentTestWhitespaceCollapse)
{
	CharReader reader{"Test/Test /* Block Comment */", 0};
	//                 012345678901234567890123456789
	//                 0        1         2
	Tokenizer tokenizer(WhitespaceMode::COLLAPSE);

	const TokenTypeId t1 = tokenizer.registerToken("/");
	const TokenTypeId t2 = tokenizer.registerToken("/*");
	const TokenTypeId t3 = tokenizer.registerToken("*/");

	std::vector<Token> expected = {
	    {TextToken, "Test", SourceLocation{0, 0, 4}},
	    {t1, "/", SourceLocation{0, 4, 5}},
	    {TextToken, "Test", SourceLocation{0, 5, 9}},
	    {t2, "/*", SourceLocation{0, 10, 12}},
	    {TextToken, "Block Comment", SourceLocation{0, 13, 26}},
	    {t3, "*/", SourceLocation{0, 27, 29}}};

	Token t;
	for (auto &te : expected) {
		EXPECT_TRUE(tokenizer.read(reader, t));
		EXPECT_EQ(te.type, t.type);
		EXPECT_EQ(te.content, t.content);
		EXPECT_EQ(te.location.getSourceId(), t.location.getSourceId());
		EXPECT_EQ(te.location.getStart(), t.location.getStart());
		EXPECT_EQ(te.location.getEnd(), t.location.getEnd());
	}
	ASSERT_FALSE(tokenizer.read(reader, t));
}
}

