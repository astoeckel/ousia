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
#include <core/parser/utils/TokenizedData.hpp>

namespace ousia {

TEST(Tokenizer, tokenRegistration)
{
	Tokenizer tokenizer;

	ASSERT_EQ(Tokens::Empty, tokenizer.registerToken(""));

	ASSERT_EQ(0U, tokenizer.registerToken("a"));
	ASSERT_EQ(Tokens::Empty, tokenizer.registerToken("a"));
	ASSERT_EQ("a", tokenizer.lookupToken(0U).string);

	ASSERT_EQ(1U, tokenizer.registerToken("b"));
	ASSERT_EQ(Tokens::Empty, tokenizer.registerToken("b"));
	ASSERT_EQ("b", tokenizer.lookupToken(1U).string);

	ASSERT_EQ(2U, tokenizer.registerToken("c"));
	ASSERT_EQ(Tokens::Empty, tokenizer.registerToken("c"));
	ASSERT_EQ("c", tokenizer.lookupToken(2U).string);

	ASSERT_TRUE(tokenizer.unregisterToken(1U));
	ASSERT_FALSE(tokenizer.unregisterToken(1U));
	ASSERT_EQ("", tokenizer.lookupToken(1U).string);

	ASSERT_EQ(1U, tokenizer.registerToken("d"));
	ASSERT_EQ(Tokens::Empty, tokenizer.registerToken("d"));
	ASSERT_EQ("d", tokenizer.lookupToken(1U).string);
}

void expectData(const std::string &expected, SourceOffset tokenStart,
                SourceOffset tokenEnd, SourceOffset textStart,
                SourceOffset textEnd, const Token &token, TokenizedData &data,
                WhitespaceMode mode = WhitespaceMode::PRESERVE)
{
	ASSERT_EQ(Tokens::Data, token.id);

	Variant text = data.text(mode);
	ASSERT_TRUE(text.isString());

	EXPECT_EQ(expected, text.asString());
	EXPECT_EQ(tokenStart, token.location.getStart());
	EXPECT_EQ(tokenEnd, token.location.getEnd());
	EXPECT_EQ(textStart, text.getLocation().getStart());
	EXPECT_EQ(textEnd, text.getLocation().getEnd());
}

TEST(Tokenizer, textTokenPreserveWhitespace)
{
	{
		CharReader reader{" this \t is only a  \n\n test   text   "};
		//                 012345 6789012345678 9 0123456789012345
		//                 0          1           2         3
		Tokenizer tokenizer;

		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		expectData(" this \t is only a  \n\n test   text   ", 0, 36, 0, 36,
		           token, data, WhitespaceMode::PRESERVE);

		data.clear();
		ASSERT_FALSE(tokenizer.read(reader, token, data));
	}

	{
		CharReader reader{"this \t is only a  \n\n test   text"};
		//                 01234 5678901234567 8 9012345678901
		//                 0          1           2         3
		Tokenizer tokenizer;

		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		expectData("this \t is only a  \n\n test   text", 0, 32, 0, 32,
		           token, data, WhitespaceMode::PRESERVE);

		data.clear();
		ASSERT_FALSE(tokenizer.read(reader, token, data));
	}
}

TEST(Tokenizer, textTokenTrimWhitespace)
{
	{
		CharReader reader{" this \t is only a  \n\n test   text   "};
		//                 012345 6789012345678 9 0123456789012345
		//                 0          1           2         3
		Tokenizer tokenizer;

		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		expectData("this \t is only a  \n\n test   text", 0, 36, 1, 33, token,
		           data, WhitespaceMode::TRIM);

		data.clear();
		ASSERT_FALSE(tokenizer.read(reader, token, data));
	}

	{
		CharReader reader{"this \t is only a  \n\n test   text"};
		//                 01234 5678901234567 8 9012345678901
		//                 0          1           2         3
		Tokenizer tokenizer;

		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		expectData("this \t is only a  \n\n test   text", 0, 32, 0, 32,
		           token, data, WhitespaceMode::TRIM);

		data.clear();
		ASSERT_FALSE(tokenizer.read(reader, token, data));
	}
}

TEST(Tokenizer, textTokenCollapseWhitespace)
{
	{
		CharReader reader{" this \t is only a  \n\n test   text   "};
		//                 012345 6789012345678 9 0123456789012345
		//                 0          1           2         3
		Tokenizer tokenizer;

		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		expectData("this is only a test text", 0, 36, 1, 33, token, data,
		           WhitespaceMode::COLLAPSE);

		data.clear();
		ASSERT_FALSE(tokenizer.read(reader, token, data));
	}

	{
		CharReader reader{"this \t is only a  \n\n test   text"};
		//                 01234 5678901234567 8 9012345678901
		//                 0          1           2         3
		Tokenizer tokenizer;

		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		expectData("this is only a test text", 0, 32, 0, 32, token, data,
		           WhitespaceMode::COLLAPSE);

		data.clear();
		ASSERT_FALSE(tokenizer.read(reader, token, data));
	}
}

TEST(Tokenizer, simpleReadToken)
{
	CharReader reader{"test1:test2"};
	Tokenizer tokenizer;

	const TokenId tid = tokenizer.registerToken(":");
	ASSERT_EQ(0U, tid);

	{
		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		ASSERT_EQ(Tokens::Data, token.id);

		expectData("test1", 0, 5, 0, 5, token, data);

		char c;
		ASSERT_TRUE(reader.peek(c));
		ASSERT_EQ(':', c);
	}

	{
		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		ASSERT_EQ(tid, token.id);
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
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		expectData("test2", 6, 11, 6, 11, token, data);

		char c;
		ASSERT_FALSE(reader.peek(c));
	}
}

TEST(Tokenizer, simplePeekToken)
{
	CharReader reader{"test1:test2"};
	Tokenizer tokenizer;

	const TokenId tid = tokenizer.registerToken(":");
	ASSERT_EQ(0U, tid);

	{
		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.peek(reader, token, data));
		expectData("test1", 0, 5, 0, 5, token, data);
		ASSERT_EQ(0U, reader.getOffset());
		ASSERT_EQ(5U, reader.getPeekOffset());
	}

	{
		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.peek(reader, token, data));

		ASSERT_EQ(tid, token.id);
		ASSERT_EQ(":", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(5U, loc.getStart());
		ASSERT_EQ(6U, loc.getEnd());
		ASSERT_EQ(0U, reader.getOffset());
		ASSERT_EQ(6U, reader.getPeekOffset());
	}

	{
		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.peek(reader, token, data));
		expectData("test2", 6, 11, 6, 11, token, data);
		ASSERT_EQ(0U, reader.getOffset());
		ASSERT_EQ(11U, reader.getPeekOffset());
	}

	{
		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		expectData("test1", 0, 5, 0, 5, token, data);
		ASSERT_EQ(5U, reader.getOffset());
		ASSERT_EQ(5U, reader.getPeekOffset());
	}

	{
		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));

		ASSERT_EQ(tid, token.id);
		ASSERT_EQ(":", token.content);

		SourceLocation loc = token.location;
		ASSERT_EQ(5U, loc.getStart());
		ASSERT_EQ(6U, loc.getEnd());
		ASSERT_EQ(6U, reader.getOffset());
		ASSERT_EQ(6U, reader.getPeekOffset());
	}

	{
		Token token;
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		expectData("test2", 6, 11, 6, 11, token, data);
		ASSERT_EQ(11U, reader.getOffset());
		ASSERT_EQ(11U, reader.getPeekOffset());
	}
}

TEST(Tokenizer, ambiguousTokens)
{
	CharReader reader{"abc"};
	Tokenizer tokenizer;
	TokenizedData data;

	TokenId t1 = tokenizer.registerToken("abd");
	TokenId t2 = tokenizer.registerToken("bc");

	ASSERT_EQ(0U, t1);
	ASSERT_EQ(1U, t2);

	Token token;
	data.clear();
	ASSERT_TRUE(tokenizer.read(reader, token, data));

	expectData("a", 0, 1, 0, 1, token, data);

	SourceLocation loc = token.location;
	ASSERT_EQ(0U, loc.getStart());
	ASSERT_EQ(1U, loc.getEnd());

	data.clear();
	ASSERT_TRUE(tokenizer.read(reader, token, data));

	ASSERT_EQ(t2, token.id);
	ASSERT_EQ("bc", token.content);

	loc = token.location;
	ASSERT_EQ(1U, loc.getStart());
	ASSERT_EQ(3U, loc.getEnd());

	data.clear();
	ASSERT_FALSE(tokenizer.read(reader, token, data));
}

TEST(Tokenizer, commentTestWhitespacePreserve)
{
	CharReader reader{"Test/Test /* Block Comment */", 0};
	//                 012345678901234567890123456789
	//                 0        1         2
	Tokenizer tokenizer;

	const TokenId t1 = tokenizer.registerToken("/");
	const TokenId t2 = tokenizer.registerToken("/*");
	const TokenId t3 = tokenizer.registerToken("*/");

	std::vector<Token> expected = {
	    {Tokens::Data, "Test", SourceLocation{0, 0, 4}},
	    {t1, "/", SourceLocation{0, 4, 5}},
	    {Tokens::Data, "Test ", SourceLocation{0, 5, 10}},
	    {t2, "/*", SourceLocation{0, 10, 12}},
	    {Tokens::Data, " Block Comment ", SourceLocation{0, 12, 27}},
	    {t3, "*/", SourceLocation{0, 27, 29}}};

	Token t;
	for (auto &te : expected) {
		TokenizedData data(0);
		EXPECT_TRUE(tokenizer.read(reader, t, data));
		EXPECT_EQ(te.id, t.id);
		if (te.id != Tokens::Data) {
			EXPECT_EQ(te.content, t.content);
		} else {
			Variant text = data.text(WhitespaceMode::PRESERVE);
			ASSERT_TRUE(text.isString());
			EXPECT_EQ(te.content, text.asString());
		}
		EXPECT_EQ(te.location.getSourceId(), t.location.getSourceId());
		EXPECT_EQ(te.location.getStart(), t.location.getStart());
		EXPECT_EQ(te.location.getEnd(), t.location.getEnd());
	}

	TokenizedData data;
	ASSERT_FALSE(tokenizer.read(reader, t, data));
}
}

