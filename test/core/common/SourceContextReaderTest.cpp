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
#include <core/common/SourceContextReader.hpp>

namespace ousia {

static const std::string testStr{"first line\n\nthird line\nlast line"};
//                                0123456789 0 12345678901 23456789012
//                                0          1          2          3

static SourceContext readContext(
    SourceContextReader &sr, size_t pos,
    size_t width = SourceContextReader::MAX_MAX_CONTEXT_LENGTH)
{
	CharReader reader{testStr};
	return sr.readContext(reader, SourceRange{pos}, width);
}

TEST(SourceContextReader, firstLine)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 0);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("first line", ctx.text);
		EXPECT_EQ(0U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(1U, ctx.startLine);
		EXPECT_EQ(1U, ctx.startColumn);
		EXPECT_EQ(1U, ctx.endLine);
		EXPECT_EQ(1U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, firstLineCenter)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 5);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("first line", ctx.text);
		EXPECT_EQ(5U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(1U, ctx.startLine);
		EXPECT_EQ(6U, ctx.startColumn);
		EXPECT_EQ(1U, ctx.endLine);
		EXPECT_EQ(6U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, firstLineBeginTruncated)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 0, 3);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("fir", ctx.text);
		EXPECT_EQ(0U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(1U, ctx.startLine);
		EXPECT_EQ(1U, ctx.startColumn);
		EXPECT_EQ(1U, ctx.endLine);
		EXPECT_EQ(1U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_TRUE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, inWhitespaceSequence)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 10);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("first line", ctx.text);
		EXPECT_EQ(10U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(1U, ctx.startLine);
		EXPECT_EQ(11U, ctx.startColumn);
		EXPECT_EQ(1U, ctx.endLine);
		EXPECT_EQ(11U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, truncation)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 5, 3);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("t l", ctx.text);
		EXPECT_EQ(1U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(1U, ctx.startLine);
		EXPECT_EQ(6U, ctx.startColumn);
		EXPECT_EQ(1U, ctx.endLine);
		EXPECT_EQ(6U, ctx.endColumn);
		EXPECT_TRUE(ctx.truncatedStart);
		EXPECT_TRUE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, emptyLine)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 11);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("", ctx.text);
		EXPECT_EQ(0U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(2U, ctx.startLine);
		EXPECT_EQ(1U, ctx.startColumn);
		EXPECT_EQ(2U, ctx.endLine);
		EXPECT_EQ(1U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, thirdLine)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 12);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("third line", ctx.text);
		EXPECT_EQ(0U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(3U, ctx.startLine);
		EXPECT_EQ(1U, ctx.startColumn);
		EXPECT_EQ(3U, ctx.endLine);
		EXPECT_EQ(1U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, thirdLineBeginTruncated)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 12, 3);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("thi", ctx.text);
		EXPECT_EQ(0U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(3U, ctx.startLine);
		EXPECT_EQ(1U, ctx.startColumn);
		EXPECT_EQ(3U, ctx.endLine);
		EXPECT_EQ(1U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_TRUE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, thirdLineEnd)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 22);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("third line", ctx.text);
		EXPECT_EQ(10U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(3U, ctx.startLine);
		EXPECT_EQ(11U, ctx.startColumn);
		EXPECT_EQ(3U, ctx.endLine);
		EXPECT_EQ(11U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, lastLine)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 23);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("last line", ctx.text);
		EXPECT_EQ(0U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(4U, ctx.startLine);
		EXPECT_EQ(1U, ctx.startColumn);
		EXPECT_EQ(4U, ctx.endLine);
		EXPECT_EQ(1U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, lastLineMiddle)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 27);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("last line", ctx.text);
		EXPECT_EQ(4U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(4U, ctx.startLine);
		EXPECT_EQ(5U, ctx.startColumn);
		EXPECT_EQ(4U, ctx.endLine);
		EXPECT_EQ(5U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, lastLineMiddleTruncated)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 27, 3);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("t l", ctx.text);
		EXPECT_EQ(1U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(4U, ctx.startLine);
		EXPECT_EQ(5U, ctx.startColumn);
		EXPECT_EQ(4U, ctx.endLine);
		EXPECT_EQ(5U, ctx.endColumn);
		EXPECT_TRUE(ctx.truncatedStart);
		EXPECT_TRUE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, lastLineEnd)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 32);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("last line", ctx.text);
		EXPECT_EQ(9U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(4U, ctx.startLine);
		EXPECT_EQ(10U, ctx.startColumn);
		EXPECT_EQ(4U, ctx.endLine);
		EXPECT_EQ(10U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, lastLineEndTruncated)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 32, 3);

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("ine", ctx.text);
		EXPECT_EQ(3U, ctx.relPos);
		EXPECT_EQ(0U, ctx.relLen);
		EXPECT_EQ(4U, ctx.startLine);
		EXPECT_EQ(10U, ctx.startColumn);
		EXPECT_EQ(4U, ctx.endLine);
		EXPECT_EQ(10U, ctx.endColumn);
		EXPECT_TRUE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

TEST(SourceContextReader, lastLineBeyondEnd)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		SourceContext ctx = readContext(sr, 33);
		EXPECT_FALSE(ctx.isValid());
	}
}

TEST(SourceContextReader, multiline)
{
	SourceContextReader sr;
	for (int i = 0; i < 2; i++) {
		CharReader reader{testStr};
		SourceContext ctx = sr.readContext(reader, SourceRange{5, 17});

		EXPECT_TRUE(ctx.isValid());
		EXPECT_EQ("first line\n\nthird line", ctx.text);
		EXPECT_EQ(5U, ctx.relPos);
		EXPECT_EQ(12U, ctx.relLen);
		EXPECT_EQ(1U, ctx.startLine);
		EXPECT_EQ(6U, ctx.startColumn);
		EXPECT_EQ(3U, ctx.endLine);
		EXPECT_EQ(6U, ctx.endColumn);
		EXPECT_FALSE(ctx.truncatedStart);
		EXPECT_FALSE(ctx.truncatedEnd);
	}
}

}

