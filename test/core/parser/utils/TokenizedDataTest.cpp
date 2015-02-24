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

#include <core/parser/utils/TokenizedData.hpp>

namespace ousia {

TEST(TokenizedData, dataWhitespacePreserve)
{
	TokenizedData data;
	ASSERT_EQ(16U, data.append(" test1   test2  "));
	//                          0123456789012345
	//                          0         1

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ(" test1   test2  ", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(16U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, dataWhitespaceTrim)
{
	TokenizedData data;
	ASSERT_EQ(16U, data.append(" test1   test2  "));
	//                          0123456789012345
	//                          0         1

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::TRIM));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ("test1   test2", token.content);
	EXPECT_EQ(1U, token.getLocation().getStart());
	EXPECT_EQ(14U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::TRIM));
}

TEST(TokenizedData, dataWhitespaceCollapse)
{
	TokenizedData data;
	ASSERT_EQ(16U, data.append(" test1   test2  "));
	//                          0123456789012345
	//                          0         1

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ("test1 test2", token.content);
	EXPECT_EQ(1U, token.getLocation().getStart());
	EXPECT_EQ(14U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::COLLAPSE));
}

TEST(TokenizedData, singleToken)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(5, 0, 2);

	data.enableToken(5);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, singleDisabledToken)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(5, 0, 2);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, dualToken)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(6, 0, 1);
	data.mark(5, 0, 2);
	data.mark(6, 1, 1);

	data.enableToken(5);
	data.enableToken(6);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, dualTokenShorterEnabled)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(6, 0, 1);
	data.mark(5, 0, 2);
	data.mark(6, 1, 1);

	data.enableToken(6);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(6U, token.id);
	EXPECT_EQ("$", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(1U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(6U, token.id);
	EXPECT_EQ("$", token.content);
	EXPECT_EQ(1U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, dualTokenLongerEnabled)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(5, 0, 2);

	data.enableToken(5);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, tokensAndDataPreserveWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(10U, data.append("$$ test $$"));
	//                          0123456789
	data.mark(5, 0, 2);
	data.mark(5, 2);

	data.enableToken(5);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ(" test ", token.content);
	EXPECT_EQ(2U, token.getLocation().getStart());
	EXPECT_EQ(8U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(8U, token.getLocation().getStart());
	EXPECT_EQ(10U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, tokensAndDataTrimWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(10U, data.append("$$ test $$"));
	//                          0123456789
	data.mark(5, 0, 2);
	data.mark(5, 2);

	data.enableToken(5);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::TRIM));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::TRIM));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ("test", token.content);
	EXPECT_EQ(3U, token.getLocation().getStart());
	EXPECT_EQ(7U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::TRIM));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(8U, token.getLocation().getStart());
	EXPECT_EQ(10U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::TRIM));
}

TEST(TokenizedData, tokensAndDataCollapseWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(10U, data.append("$$ test $$"));
	//                          0123456789
	data.mark(5, 0, 2);
	data.mark(5, 2);

	data.enableToken(5);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ("test", token.content);
	EXPECT_EQ(3U, token.getLocation().getStart());
	EXPECT_EQ(7U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(8U, token.getLocation().getStart());
	EXPECT_EQ(10U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::COLLAPSE));
}

TEST(TokenizedData, tokensAndWhitespacePreserveWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(10U, data.append("$$      $$"));
	//                          0123456789
	data.mark(5, 0, 2);
	data.mark(5, 2);

	data.enableToken(5);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ("      ", token.content);
	EXPECT_EQ(2U, token.getLocation().getStart());
	EXPECT_EQ(8U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(8U, token.getLocation().getStart());
	EXPECT_EQ(10U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, tokensAndWhitespaceTrimWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(10U, data.append("$$      $$"));
	//                          0123456789
	data.mark(5, 0, 2);
	data.mark(5, 2);

	data.enableToken(5);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::TRIM));
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::TRIM));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(8U, token.getLocation().getStart());
	EXPECT_EQ(10U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::TRIM));
}

TEST(TokenizedData, tokensAndWhitespaceCollapseWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(10U, data.append("$$      $$"));
	//                          0123456789
	data.mark(5, 0, 2);
	data.mark(5, 2);

	data.enableToken(5);

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(0U, token.getLocation().getStart());
	EXPECT_EQ(2U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(data.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(8U, token.getLocation().getStart());
	EXPECT_EQ(10U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_FALSE(data.next(token, WhitespaceMode::COLLAPSE));
}

TEST(TokenizedData, textPreserveWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(6U, data.append("  $$  "));
	//                         012345
	data.mark(5, 2, 2);

	data.enableToken(5);

	Variant text;
	text = data.text(WhitespaceMode::PRESERVE);
	EXPECT_EQ("  ", text.asString());
	EXPECT_EQ(0U, text.getLocation().getStart());
	EXPECT_EQ(2U, text.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, text.getLocation().getSourceId());

	Token token;
	ASSERT_TRUE(data.next(token, WhitespaceMode::PRESERVE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(2U, token.getLocation().getStart());
	EXPECT_EQ(4U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	text = data.text(WhitespaceMode::PRESERVE);
	EXPECT_EQ("  ", text.asString());
	EXPECT_EQ(4U, text.getLocation().getStart());
	EXPECT_EQ(6U, text.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, text.getLocation().getSourceId());

	ASSERT_EQ(nullptr, data.text(WhitespaceMode::PRESERVE));
	ASSERT_FALSE(data.next(token, WhitespaceMode::PRESERVE));
}

TEST(TokenizedData, textTrimWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(6U, data.append("  $$  "));
	//                         012345
	data.mark(5, 2, 2);

	data.enableToken(5);

	Token token;
	ASSERT_EQ(nullptr, data.text(WhitespaceMode::TRIM));

	ASSERT_TRUE(data.next(token, WhitespaceMode::TRIM));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(2U, token.getLocation().getStart());
	EXPECT_EQ(4U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_EQ(nullptr, data.text(WhitespaceMode::TRIM));
	ASSERT_FALSE(data.next(token, WhitespaceMode::TRIM));
}

TEST(TokenizedData, textCollapseWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(6U, data.append("  $$  "));
	//                         012345
	data.mark(5, 2, 2);

	data.enableToken(5);

	Token token;
	ASSERT_EQ(nullptr, data.text(WhitespaceMode::COLLAPSE));

	ASSERT_TRUE(data.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(5U, token.id);
	EXPECT_EQ("$$", token.content);
	EXPECT_EQ(2U, token.getLocation().getStart());
	EXPECT_EQ(4U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_EQ(nullptr, data.text(WhitespaceMode::COLLAPSE));
	ASSERT_FALSE(data.next(token, WhitespaceMode::COLLAPSE));
}

TEST(TokenizedData, appendChars)
{
	TokenizedData data;
	ASSERT_EQ(1U, data.append('t', 5, 7));
	ASSERT_EQ(2U, data.append('e', 7, 8));
	ASSERT_EQ(3U, data.append('s', 8, 10));
	ASSERT_EQ(4U, data.append('t', 10, 12));

	Variant text = data.text(WhitespaceMode::COLLAPSE);
	ASSERT_EQ("test", text.asString());
	EXPECT_EQ(5U, text.getLocation().getStart());
	EXPECT_EQ(12U, text.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, text.getLocation().getSourceId());

	ASSERT_EQ(nullptr, data.text(WhitespaceMode::PRESERVE));

	Token token;
	ASSERT_FALSE(data.next(token, WhitespaceMode::COLLAPSE));
}

TEST(TokenizedData, copy)
{
	TokenizedData data;
	ASSERT_EQ(7U, data.append(" a $ b "));
	//                         0123456
	data.mark(6, 3, 1);
	data.enableToken(6);

	Variant text;
	Token token;

	text = data.text(WhitespaceMode::COLLAPSE);
	ASSERT_EQ("a", text.asString());
	EXPECT_EQ(1U, text.getLocation().getStart());
	EXPECT_EQ(2U, text.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, text.getLocation().getSourceId());

	ASSERT_EQ(nullptr, data.text(WhitespaceMode::COLLAPSE));

	TokenizedData dataCopy = data;

	ASSERT_TRUE(data.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(6U, token.id);
	EXPECT_EQ("$", token.content);
	EXPECT_EQ(3U, token.getLocation().getStart());
	EXPECT_EQ(4U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	ASSERT_TRUE(dataCopy.next(token, WhitespaceMode::COLLAPSE));
	EXPECT_EQ(6U, token.id);
	EXPECT_EQ("$", token.content);
	EXPECT_EQ(3U, token.getLocation().getStart());
	EXPECT_EQ(4U, token.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, token.getLocation().getSourceId());

	text = data.text(WhitespaceMode::PRESERVE);
	ASSERT_EQ(" b ", text.asString());
	EXPECT_EQ(4U, text.getLocation().getStart());
	EXPECT_EQ(7U, text.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, text.getLocation().getSourceId());
	ASSERT_FALSE(data.next(token));

	text = dataCopy.text(WhitespaceMode::COLLAPSE);
	ASSERT_EQ("b", text.asString());
	EXPECT_EQ(5U, text.getLocation().getStart());
	EXPECT_EQ(6U, text.getLocation().getEnd());
	EXPECT_EQ(InvalidSourceId, text.getLocation().getSourceId());
	ASSERT_FALSE(data.next(token));
}
}

