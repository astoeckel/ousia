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

#include "TokenizedDataTestUtils.hpp"

namespace ousia {

static void assertPrimaryToken(CharReader &reader, Tokenizer &tokenizer,
                               TokenId id, const std::string &text,
                               SourceOffset start = InvalidSourceOffset,
                               SourceOffset end = InvalidSourceOffset,
                               SourceId sourceId = InvalidSourceId)
{
	Token token;
	TokenizedData data;
	ASSERT_TRUE(tokenizer.read(reader, token, data));
	EXPECT_EQ(id, token.id);
	EXPECT_EQ(text, token.content);
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, token.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, token.getLocation().getEnd());
	}
	EXPECT_EQ(sourceId, token.getLocation().getSourceId());
}

static void expectData(const std::string &expected, SourceOffset tokenStart,
                       SourceOffset tokenEnd, SourceOffset textStart,
                       SourceOffset textEnd, const Token &token,
                       TokenizedData &data,
                       WhitespaceMode mode = WhitespaceMode::PRESERVE)
{
	ASSERT_EQ(Tokens::Data, token.id);

	Token textToken;
	TokenizedDataReader reader = data.reader();
	ASSERT_TRUE(reader.read(textToken, TokenSet{}, mode));

	EXPECT_EQ(expected, textToken.content);
	EXPECT_EQ(tokenStart, token.location.getStart());
	EXPECT_EQ(tokenEnd, token.location.getEnd());
	EXPECT_EQ(textStart, textToken.getLocation().getStart());
	EXPECT_EQ(textEnd, textToken.getLocation().getEnd());
	EXPECT_TRUE(reader.atEnd());
}

static void assertDataToken(CharReader &reader, Tokenizer &tokenizer,
                            const std::string &expected,
                            SourceOffset tokenStart, SourceOffset tokenEnd,
                            SourceOffset textStart, SourceOffset textEnd,
                            WhitespaceMode mode = WhitespaceMode::PRESERVE)
{
	Token token;
	TokenizedData data;
	ASSERT_TRUE(tokenizer.read(reader, token, data));

	expectData(expected, tokenStart, tokenEnd, textStart, textEnd, token, data,
	           mode);
}

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

		expectData("this \t is only a  \n\n test   text", 0, 32, 0, 32, token,
		           data, WhitespaceMode::PRESERVE);

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

		expectData("this \t is only a  \n\n test   text", 0, 32, 0, 32, token,
		           data, WhitespaceMode::TRIM);

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
			TokenizedDataReader dataReader = data.reader();
			Token textToken;
			ASSERT_TRUE(dataReader.read(textToken, TokenSet{},
			                            WhitespaceMode::PRESERVE));
			EXPECT_TRUE(dataReader.atEnd());
			EXPECT_EQ(te.content, textToken.content);
		}
		EXPECT_EQ(te.location.getSourceId(), t.location.getSourceId());
		EXPECT_EQ(te.location.getStart(), t.location.getStart());
		EXPECT_EQ(te.location.getEnd(), t.location.getEnd());
	}

	TokenizedData data;
	ASSERT_FALSE(tokenizer.read(reader, t, data));
}

TEST(Tokenizer, nonPrimaryTokens)
{
	CharReader reader{
	    "<<switch to $inline \\math mode$ they said, see the world they "
	    "said>>"};
	//   012345678901234567890 12345678901234567890123456789012345678901234567
	//   0         1         2          3         4         5         6

	Tokenizer tokenizer;

	TokenId tBackslash = tokenizer.registerToken("\\");
	TokenId tDollar = tokenizer.registerToken("$", false);
	TokenId tSpeechStart = tokenizer.registerToken("<<", false);
	TokenId tSpeechEnd = tokenizer.registerToken(">>", false);

	TokenSet tokens = TokenSet{tDollar, tSpeechStart, tSpeechEnd};

	Token token, textToken;
	{
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		ASSERT_EQ(Tokens::Data, token.id);

		TokenizedDataReader dataReader = data.reader();
		assertToken(dataReader, tSpeechStart, "<<", tokens,
		            WhitespaceMode::TRIM, 0, 2);
		assertText(dataReader, "switch to", tokens, WhitespaceMode::TRIM, 2,
		           11);
		assertToken(dataReader, tDollar, "$", tokens, WhitespaceMode::TRIM, 12,
		            13);
		assertText(dataReader, "inline", tokens, WhitespaceMode::TRIM, 13, 19);
		assertEnd(dataReader);
	}

	{
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		ASSERT_EQ(tBackslash, token.id);
		ASSERT_EQ(20U, token.location.getStart());
		ASSERT_EQ(21U, token.location.getEnd());
	}

	{
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		ASSERT_EQ(Tokens::Data, token.id);

		TokenizedDataReader dataReader = data.reader();
		assertText(dataReader, "math mode", tokens, WhitespaceMode::TRIM, 21,
		           30);
		assertToken(dataReader, tDollar, "$", tokens, WhitespaceMode::TRIM, 30,
		            31);
		assertText(dataReader, "they said, see the world they said", tokens,
		           WhitespaceMode::TRIM, 32, 66);
		assertToken(dataReader, tSpeechEnd, ">>", tokens, WhitespaceMode::TRIM,
		            66, 68);
		assertEnd(dataReader);
	}

	TokenizedData data;
	ASSERT_FALSE(tokenizer.read(reader, token, data));
}

TEST(Tokenizer, primaryNonPrimaryTokenInteraction)
{
	CharReader reader{"<<test1>><test2><<test3\\><<<test4>>>"};
	//                 01234567890123456789012 3456789012345
	//                 0         1         2          3

	Tokenizer tokenizer;

	TokenId tP1 = tokenizer.registerToken("<", true);
	TokenId tP2 = tokenizer.registerToken(">", true);
	TokenId tP3 = tokenizer.registerToken("\\>", true);
	TokenId tN1 = tokenizer.registerToken("<<", false);
	TokenId tN2 = tokenizer.registerToken(">>", false);

	TokenSet tokens = TokenSet{tN1, tN2};

	Token token, textToken;
	{
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		ASSERT_EQ(Tokens::Data, token.id);

		TokenizedDataReader dataReader = data.reader();
		assertToken(dataReader, tN1, "<<", tokens, WhitespaceMode::TRIM, 0, 2);
		assertText(dataReader, "test1", tokens, WhitespaceMode::TRIM, 2, 7);
		assertToken(dataReader, tN2, ">>", tokens, WhitespaceMode::TRIM, 7, 9);
		assertEnd(dataReader);
	}

	assertPrimaryToken(reader, tokenizer, tP1, "<", 9, 10);
	assertDataToken(reader, tokenizer, "test2", 10, 15, 10, 15);
	assertPrimaryToken(reader, tokenizer, tP2, ">", 15, 16);

	{
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		ASSERT_EQ(Tokens::Data, token.id);

		TokenizedDataReader dataReader = data.reader();
		assertToken(dataReader, tN1, "<<", tokens, WhitespaceMode::TRIM, 16, 18);
		assertText(dataReader, "test3", tokens, WhitespaceMode::TRIM, 18, 23);
		assertEnd(dataReader);
	}

	assertPrimaryToken(reader, tokenizer, tP3, "\\>", 23, 25);

	{
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		ASSERT_EQ(Tokens::Data, token.id);

		TokenizedDataReader dataReader = data.reader();
		assertToken(dataReader, tN1, "<<", tokens, WhitespaceMode::TRIM, 25, 27);
		assertEnd(dataReader);
	}

	assertPrimaryToken(reader, tokenizer, tP1, "<", 27, 28);

	{
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		ASSERT_EQ(Tokens::Data, token.id);

		TokenizedDataReader dataReader = data.reader();
		assertText(dataReader, "test4", tokens, WhitespaceMode::TRIM, 28, 33);
		assertToken(dataReader, tN2, ">>", tokens, WhitespaceMode::TRIM, 33, 35);
		assertEnd(dataReader);
	}

	assertPrimaryToken(reader, tokenizer, tP2, ">", 35, 36);

	TokenizedData data;
	ASSERT_FALSE(tokenizer.read(reader, token, data));
}

TEST(Tokenizer, ambiguousTokens2)
{
	CharReader reader{"<\\"};

	Tokenizer tokenizer;

	TokenId tBackslash = tokenizer.registerToken("\\");
	TokenId tAnnotationStart = tokenizer.registerToken("<\\");

	TokenSet tokens = TokenSet{tBackslash, tAnnotationStart};
	Token token;
	{
		TokenizedData data;
		ASSERT_TRUE(tokenizer.read(reader, token, data));
		ASSERT_EQ("<\\", token.content);
		ASSERT_EQ(tAnnotationStart, token.id);
		ASSERT_TRUE(data.empty());
	}

	{
		TokenizedData data;
		ASSERT_FALSE(tokenizer.read(reader, token, data));
	}
}
}

