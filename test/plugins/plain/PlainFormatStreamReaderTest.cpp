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

#include <iostream>

#include <core/common/CharReader.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <plugins/plain/PlainFormatStreamReader.hpp>

namespace ousia {

static TerminalLogger logger(std::cerr, true);

TEST(PlainFormatStreamReader, empty)
{
	const char *testString = "";
	CharReader charReader(testString);

	PlainFormatStreamReader reader(charReader, logger);

	ASSERT_EQ(PlainFormatStreamReader::State::END, reader.parse());
}

TEST(PlainFormatStreamReader, oneCharacter)
{
	const char *testString = "a";
	CharReader charReader(testString);

	PlainFormatStreamReader reader(charReader, logger);

	ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
	ASSERT_EQ("a", reader.getData().asString());

	SourceLocation loc = reader.getData().getLocation();
	ASSERT_EQ(0U, loc.getStart());
	ASSERT_EQ(1U, loc.getEnd());
}

TEST(PlainFormatStreamReader, whitespaceElimination)
{
	const char *testString = " hello \t world ";
	//                        0123456 78901234
	//                        0          1
	CharReader charReader(testString);

	PlainFormatStreamReader reader(charReader, logger);

	ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
	ASSERT_EQ("hello world", reader.getData().asString());

	SourceLocation loc = reader.getData().getLocation();
	ASSERT_EQ(1U, loc.getStart());
	ASSERT_EQ(14U, loc.getEnd());
}

TEST(PlainFormatStreamReader, whitespaceEliminationWithLinebreak)
{
	const char *testString = " hello \n world ";
	//                        0123456 78901234
	//                        0          1
	CharReader charReader(testString);

	PlainFormatStreamReader reader(charReader, logger);

	ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
	{
		ASSERT_EQ("hello", reader.getData().asString());

		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(1U, loc.getStart());
		ASSERT_EQ(6U, loc.getEnd());
	}
	ASSERT_EQ(PlainFormatStreamReader::State::LINEBREAK, reader.parse());
	ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
	{
		ASSERT_EQ("world", reader.getData().asString());

		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(9U, loc.getStart());
		ASSERT_EQ(14U, loc.getEnd());
	}
}

TEST(PlainFormatStreamReader, escapeWhitespace)
{
	const char *testString = " hello \n\\ world ";
	//                        0123456 7 89012345
	//                        0           1
	CharReader charReader(testString);

	PlainFormatStreamReader reader(charReader, logger);

	ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
	{
		ASSERT_EQ("hello", reader.getData().asString());

		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(1U, loc.getStart());
		ASSERT_EQ(6U, loc.getEnd());
	}
	ASSERT_EQ(PlainFormatStreamReader::State::LINEBREAK, reader.parse());
	ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
	{
		ASSERT_EQ(" world", reader.getData().asString());

		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(8U, loc.getStart());
		ASSERT_EQ(15U, loc.getEnd());
	}
}

static void testEscapeSpecialCharacter(const std::string &c)
{
	CharReader charReader(std::string("\\") + c);
	PlainFormatStreamReader reader(charReader, logger);
	EXPECT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
	EXPECT_EQ(c, reader.getData().asString());

	SourceLocation loc = reader.getData().getLocation();
	EXPECT_EQ(0U, loc.getStart());
	EXPECT_EQ(1U + c.size(), loc.getEnd());
}

TEST(PlainFormatStreamReader, escapeSpecialCharacters)
{
	testEscapeSpecialCharacter("\\");
	testEscapeSpecialCharacter("{");
	testEscapeSpecialCharacter("}");
	testEscapeSpecialCharacter("<");
	testEscapeSpecialCharacter(">");
}

TEST(PlainFormatStreamReader, simpleSingleLineComment)
{
	const char *testString = "% This is a single line comment";
	CharReader charReader(testString);
	PlainFormatStreamReader reader(charReader, logger);
	ASSERT_EQ(PlainFormatStreamReader::State::END, reader.parse());
}

TEST(PlainFormatStreamReader, singleLineComment)
{
	const char *testString = "a% This is a single line comment\nb";
	//                        01234567890123456789012345678901 23
	//                        0         1         2         3
	CharReader charReader(testString);
	PlainFormatStreamReader reader(charReader, logger);
	{
		ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
		ASSERT_EQ("a", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(1U, loc.getEnd());
	}

	{
		ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
		ASSERT_EQ("b", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(33U, loc.getStart());
		ASSERT_EQ(34U, loc.getEnd());
	}

	ASSERT_EQ(PlainFormatStreamReader::State::END, reader.parse());
}

TEST(PlainFormatStreamReader, multilineComment)
{
	const char *testString = "a%{ This is a\n\n multiline line comment}%b";
	//                        0123456789012 3 456789012345678901234567890
	//                        0         1           2         3         4
	CharReader charReader(testString);
	PlainFormatStreamReader reader(charReader, logger);
	{
		ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
		ASSERT_EQ("a", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(1U, loc.getEnd());
	}

	{
		ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
		ASSERT_EQ("b", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(40U, loc.getStart());
		ASSERT_EQ(41U, loc.getEnd());
	}

	ASSERT_EQ(PlainFormatStreamReader::State::END, reader.parse());
}

TEST(PlainFormatStreamReader, nestedMultilineComment)
{
	const char *testString = "a%{%{Another\n\n}%multiline line comment}%b";
	//                        0123456789012 3 456789012345678901234567890
	//                        0         1           2         3         4
	CharReader charReader(testString);
	PlainFormatStreamReader reader(charReader, logger);
	{
		ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
		ASSERT_EQ("a", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(1U, loc.getEnd());
	}

	{
		ASSERT_EQ(PlainFormatStreamReader::State::DATA, reader.parse());
		ASSERT_EQ("b", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(40U, loc.getStart());
		ASSERT_EQ(41U, loc.getEnd());
	}

	ASSERT_EQ(PlainFormatStreamReader::State::END, reader.parse());
}



}

