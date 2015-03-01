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
#include <core/common/Variant.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/parser/utils/TokenizedData.hpp>

#include <formats/osml/OsmlStreamParser.hpp>

namespace ousia {

static TerminalLogger logger(std::cerr, true);
// static ConcreteLogger logger;

static void assertCommandStart(OsmlStreamParser &parser,
                               const std::string &name,
                               bool rangeCommand,
                               SourceOffset start = InvalidSourceOffset,
                               SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::COMMAND_START, parser.parse());
	EXPECT_EQ(name, parser.getCommandName().asString());
	EXPECT_EQ(rangeCommand, parser.inRangeCommand());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, parser.getCommandName().getLocation().getStart());
		EXPECT_EQ(start, parser.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, parser.getCommandName().getLocation().getEnd());
		EXPECT_EQ(end, parser.getLocation().getEnd());
	}
}

static void assertCommandStart(OsmlStreamParser &parser,
                               const std::string &name,
                               bool rangeCommand,
                               const Variant::mapType &args,
                               SourceOffset start = InvalidSourceOffset,
                               SourceOffset end = InvalidSourceOffset)
{
	assertCommandStart(parser, name, rangeCommand, start, end);
	EXPECT_EQ(args, parser.getCommandArguments());
}

static void assertCommand(OsmlStreamParser &parser,
                               const std::string &name,
                               SourceOffset start = InvalidSourceOffset,
                               SourceOffset end = InvalidSourceOffset)
{
	assertCommandStart(parser, name, false, Variant::mapType{}, start, end);
}

static void assertRangeEnd(OsmlStreamParser &parser,
                             SourceOffset start = InvalidSourceOffset,
                             SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::RANGE_END, parser.parse());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, parser.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, parser.getLocation().getEnd());
	}
}

static void assertTextData(OsmlStreamParser &parser, const std::string &text,
                           SourceOffset dataStart = InvalidSourceOffset,
                           SourceOffset dataEnd = InvalidSourceOffset,
                           SourceOffset textStart = InvalidSourceOffset,
                           SourceOffset textEnd = InvalidSourceOffset,
                           WhitespaceMode mode = WhitespaceMode::COLLAPSE)
{
	ASSERT_EQ(OsmlStreamParser::State::DATA, parser.parse());

	const TokenizedData &data = parser.getData();
	TokenizedDataReader dataReader = data.reader();

	Token token;
	ASSERT_TRUE(dataReader.read(token, TokenSet{}, mode));
	EXPECT_EQ(Tokens::Data, token.id);
	EXPECT_EQ(text, token.content);
	if (dataStart != InvalidSourceOffset) {
		EXPECT_EQ(dataStart, data.getLocation().getStart());
		EXPECT_EQ(dataStart, parser.getLocation().getStart());
	}
	if (dataEnd != InvalidSourceOffset) {
		EXPECT_EQ(dataEnd, data.getLocation().getEnd());
		EXPECT_EQ(dataEnd, parser.getLocation().getEnd());
	}
	if (textStart != InvalidSourceOffset) {
		EXPECT_EQ(textStart, token.getLocation().getStart());
	}
	if (textEnd != InvalidSourceOffset) {
		EXPECT_EQ(textEnd, token.getLocation().getEnd());
	}
}

static void assertData(OsmlStreamParser &parser, const std::string &text,
                           SourceOffset textStart = InvalidSourceOffset,
                           SourceOffset textEnd = InvalidSourceOffset,
                           WhitespaceMode mode = WhitespaceMode::COLLAPSE)
{
	assertTextData(parser, text, InvalidSourceOffset, InvalidSourceOffset, textStart, textEnd, mode);
}

static void assertEmptyData(OsmlStreamParser &parser)
{
	ASSERT_EQ(OsmlStreamParser::State::DATA, parser.parse());

	const TokenizedData &data = parser.getData();
	TokenizedDataReader dataReader = data.reader();

	Token token;
	EXPECT_FALSE(dataReader.read(token, TokenSet{}, WhitespaceMode::TRIM));
}


static void assertFieldStart(OsmlStreamParser &parser, bool defaultField,
                             SourceOffset start = InvalidSourceOffset,
                             SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::FIELD_START, parser.parse());
	EXPECT_EQ(defaultField, parser.inDefaultField());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, parser.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, parser.getLocation().getEnd());
	}
}

static void assertFieldEnd(OsmlStreamParser &parser,
                           SourceOffset start = InvalidSourceOffset,
                           SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::FIELD_END, parser.parse());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, parser.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, parser.getLocation().getEnd());
	}
}

static void assertAnnotationStart(OsmlStreamParser &parser,
                                  const std::string &name,
                                  SourceOffset start = InvalidSourceOffset,
                                  SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::ANNOTATION_START, parser.parse());
	EXPECT_EQ(name, parser.getCommandName().asString());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, parser.getCommandName().getLocation().getStart());
		EXPECT_EQ(start, parser.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, parser.getCommandName().getLocation().getEnd());
		EXPECT_EQ(end, parser.getLocation().getEnd());
	}
}

static void assertAnnotationStart(OsmlStreamParser &parser,
                                  const std::string &name,
                                  const Variant::mapType &args,
                                  SourceOffset start = InvalidSourceOffset,
                                  SourceOffset end = InvalidSourceOffset)
{
	assertAnnotationStart(parser, name, start, end);
	EXPECT_EQ(args, parser.getCommandArguments());
}

static void assertAnnotationEnd(OsmlStreamParser &parser,
                                const std::string &name,
                                const std::string &elementName,
                                SourceOffset start = InvalidSourceOffset,
                                SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::ANNOTATION_END, parser.parse());
	ASSERT_EQ(name, parser.getCommandName().asString());
	if (!elementName.empty()) {
		ASSERT_EQ(1U, parser.getCommandArguments().asMap().size());
		ASSERT_EQ(1U, parser.getCommandArguments().asMap().count("name"));

		auto it = parser.getCommandArguments().asMap().find("name");
		ASSERT_EQ(elementName, it->second.asString());
	}
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, parser.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, parser.getLocation().getEnd());
	}
}

static void assertEnd(OsmlStreamParser &parser,
                      SourceOffset start = InvalidSourceOffset,
                      SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::END, parser.parse());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, parser.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, parser.getLocation().getEnd());
	}
}

TEST(OsmlStreamParser, empty)
{
	const char *testString = "";
	CharReader charReader(testString);

	OsmlStreamParser parser(charReader, logger);

	assertEnd(parser, 0, 0);
}

TEST(OsmlStreamParser, oneCharacter)
{
	const char *testString = "a";
	CharReader charReader(testString);

	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "a", 0, 1, 0, 1, WhitespaceMode::COLLAPSE);
	assertEnd(parser, 1, 1);
}

TEST(OsmlStreamParser, whitespacePreserve)
{
	const char *testString = " hello \t world ";
	//                        0123456 78901234
	//                        0          1
	CharReader charReader(testString);

	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, " hello \t world ", 0, 15, 0, 15,
	               WhitespaceMode::PRESERVE);
	assertEnd(parser, 15, 15);
}

TEST(OsmlStreamParser, whitespaceTrim)
{
	const char *testString = " hello \t world ";
	//                        0123456 78901234
	//                        0          1
	CharReader charReader(testString);

	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "hello \t world", 0, 15, 1, 14,
	               WhitespaceMode::TRIM);
	assertEnd(parser, 15, 15);
}

TEST(OsmlStreamParser, whitespaceCollapse)
{
	const char *testString = " hello \t world ";
	//                        0123456 78901234
	//                        0          1
	CharReader charReader(testString);

	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "hello world", 0, 15, 1, 14,
	               WhitespaceMode::COLLAPSE);
	assertEnd(parser, 15, 15);
}

TEST(OsmlStreamParser, whitespaceCollapseLinebreak)
{
	const char *testString = " hello \n world ";
	//                        0123456 78901234
	//                        0          1
	CharReader charReader(testString);

	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "hello world", 0, 15, 1, 14,
	               WhitespaceMode::COLLAPSE);
	assertEnd(parser, 15, 15);
}

TEST(OsmlStreamParser, whitespaceCollapseProtected)
{
	const char *testString = " hello\\ \\ world ";
	//                        012345 67 89012345
	//                        0           1
	CharReader charReader(testString);

	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "hello  world", 0, 16, 1, 15,
	               WhitespaceMode::COLLAPSE);
	assertEnd(parser, 16, 16);
}

TEST(OsmlStreamParser, whitespaceCollapseProtected2)
{
	const char *testString = " hello \\ \\ world ";
	//                        012345 67 89012345
	//                        0           1
	CharReader charReader(testString);

	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "hello   world", 0, 17, 1, 16,
	               WhitespaceMode::COLLAPSE);
	assertEnd(parser, 17, 17);
}

static void testEscapeSpecialCharacter(const std::string &c)
{
	CharReader charReader(std::string("\\") + c);
	OsmlStreamParser parser(charReader, logger);
	assertTextData(parser, c, 0, 2, 0, 2, WhitespaceMode::PRESERVE);
	assertEnd(parser, 2, 2);
}

TEST(OsmlStreamParser, escapeSpecialCharacters)
{
	testEscapeSpecialCharacter("\\");
	testEscapeSpecialCharacter("{");
	testEscapeSpecialCharacter("}");
}

TEST(OsmlStreamParser, simpleSingleLineComment)
{
	const char *testString = "% This is a single line comment";
	//                        0123456789012345678901234567890
	//                        0         1         2         3
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);
	assertEnd(parser, 31, 31);
}

TEST(OsmlStreamParser, singleLineComment)
{
	const char *testString = "a% This is a single line comment\nb";
	//                        01234567890123456789012345678901 23
	//                        0         1         2         3
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "ab", 0, 34, 0, 34, WhitespaceMode::PRESERVE);
	assertEnd(parser, 34, 34);
}

TEST(OsmlStreamParser, multilineComment)
{
	const char *testString = "a%{ This is a\n\n multiline line comment}%b";
	//                        0123456789012 3 456789012345678901234567890
	//                        0         1           2         3         4
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "ab", 0, 41, 0, 41, WhitespaceMode::PRESERVE);
	assertEnd(parser, 41, 41);
}

TEST(OsmlStreamParser, unfinishedMultilineComment)
{
	const char *testString = "a%{ This is a\n\n multiline line comment";
	//                        0123456789012 3 456789012345678901234567
	//                        0         1           2         3
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	logger.reset();

	ASSERT_FALSE(logger.hasError());
	assertTextData(parser, "a", 0, 1, 0, 1, WhitespaceMode::PRESERVE);
	ASSERT_TRUE(logger.hasError());
	assertEnd(parser, 38, 38);
}

TEST(OsmlStreamParser, nestedMultilineComment)
{
	const char *testString = "a%{%{Another\n\n}%multiline line comment}%b";
	//                        0123456789012 3 456789012345678901234567890
	//                        0         1           2         3         4
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertTextData(parser, "ab", 0, 41, 0, 41, WhitespaceMode::PRESERVE);
	assertEnd(parser, 41, 41);
}

TEST(OsmlStreamParser, simpleCommand)
{
	const char *testString = "\\test";
	//                        0 12345
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertCommand(parser, "test", 0, 5);
	assertEnd(parser);
}

TEST(OsmlStreamParser, simpleCommandWithName)
{
	const char *testString = "\\test#foo";
	//                         012345678
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertCommandStart(parser, "test", false, Variant::mapType{{"name", "foo"}},
	                   0, 5);

	Variant::mapType args = parser.getCommandArguments().asMap();
	ASSERT_EQ(5U, args["name"].getLocation().getStart());
	ASSERT_EQ(9U, args["name"].getLocation().getEnd());

	assertEnd(parser);
}

TEST(OsmlStreamParser, simpleCommandWithArguments)
{
	const char *testString = "\\test[a=1,b=2,c=\"test\"]";
	//                        0 123456789012345 678901 2
	//                        0          1          2
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertCommandStart(parser, "test", false,
	                   Variant::mapType{{"a", 1}, {"b", 2}, {"c", "test"}}, 0,
	                   5);

	Variant::mapType args = parser.getCommandArguments().asMap();
	ASSERT_EQ(8U, args["a"].getLocation().getStart());
	ASSERT_EQ(9U, args["a"].getLocation().getEnd());
	ASSERT_EQ(12U, args["b"].getLocation().getStart());
	ASSERT_EQ(13U, args["b"].getLocation().getEnd());
	ASSERT_EQ(16U, args["c"].getLocation().getStart());
	ASSERT_EQ(22U, args["c"].getLocation().getEnd());

	assertEnd(parser);
}

TEST(OsmlStreamParser, simpleCommandWithArgumentsAndName)
{
	const char *testString = "\\test#bla[a=1,b=2,c=\"test\"]";
	//                        0 1234567890123456789 01234 56
	//                        0          1          2
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertCommandStart(
	    parser, "test", false,
	    Variant::mapType{{"name", "bla"}, {"a", 1}, {"b", 2}, {"c", "test"}}, 0,
	    5);

	Variant::mapType args = parser.getCommandArguments().asMap();
	ASSERT_EQ(5U, args["name"].getLocation().getStart());
	ASSERT_EQ(9U, args["name"].getLocation().getEnd());
	ASSERT_EQ(12U, args["a"].getLocation().getStart());
	ASSERT_EQ(13U, args["a"].getLocation().getEnd());
	ASSERT_EQ(16U, args["b"].getLocation().getStart());
	ASSERT_EQ(17U, args["b"].getLocation().getEnd());
	ASSERT_EQ(20U, args["c"].getLocation().getStart());
	ASSERT_EQ(26U, args["c"].getLocation().getEnd());

	assertEnd(parser);
}

TEST(OsmlStreamParser, fields)
{
	const char *testString = "\\test{a}{b}{c}";
	//                         01234567890123
	//                         0         1
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertCommand(parser, "test", 0, 5);
	assertFieldStart(parser, false, 5, 6);
	assertTextData(parser, "a", 6, 7, 6, 7, WhitespaceMode::PRESERVE);
	assertFieldEnd(parser, 7, 8);

	assertFieldStart(parser, false, 8, 9);
	assertTextData(parser, "b", 9, 10, 9, 10, WhitespaceMode::PRESERVE);
	assertFieldEnd(parser, 10, 11);

	assertFieldStart(parser, false, 11, 12);
	assertTextData(parser, "c", 12, 13, 12, 13, WhitespaceMode::PRESERVE);
	assertFieldEnd(parser, 13, 14);
	assertEnd(parser, 14, 14);
}

TEST(OsmlStreamParser, fieldsWithoutCommand)
{
	const char *testString = "{a}{b}{c}";
	//                        012345678
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertFieldStart(parser, false, 0, 1);
	assertTextData(parser, "a", 1, 2, 1, 2, WhitespaceMode::PRESERVE);
	assertFieldEnd(parser, 2, 3);

	assertFieldStart(parser, false, 3, 4);
	assertTextData(parser, "b", 4, 5, 4, 5, WhitespaceMode::PRESERVE);
	assertFieldEnd(parser, 5, 6);

	assertFieldStart(parser, false, 6, 7);
	assertTextData(parser, "c", 7, 8, 7, 8, WhitespaceMode::PRESERVE);
	assertFieldEnd(parser, 8, 9);
	assertEnd(parser, 9, 9);
}

TEST(OsmlStreamParser, nestedField)
{
	const char *testString = "{{a{b}}}";
	//                        01234567
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertFieldStart(parser, false, 0, 1);
	assertFieldStart(parser, false, 1, 2);
	assertTextData(parser, "a", 2, 3, 2, 3, WhitespaceMode::PRESERVE);
	assertFieldStart(parser, false, 3, 4);
	assertTextData(parser, "b", 4, 5, 4, 5, WhitespaceMode::PRESERVE);
	assertFieldEnd(parser, 5, 6);
	assertFieldEnd(parser, 6, 7);
	assertFieldEnd(parser, 7, 8);
	assertEnd(parser, 8, 8);
}

TEST(OsmlStreamParser, errorUnbalancedField)
{
	const char *testString = "{a";
	//                        01
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	logger.reset();

	assertFieldStart(parser, false, 0, 1);
	assertTextData(parser, "a", 1, 2, 1, 2, WhitespaceMode::PRESERVE);
	ASSERT_FALSE(logger.hasError());
	assertEnd(parser, 2, 2);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, dataOutsideField)
{
	const char *testString = "\\test{a}{b} c";
	//                         0123456789012
	//                         0         1
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	assertCommand(parser, "test", 0, 5);
	assertFieldStart(parser, false, 5, 6);
	assertTextData(parser, "a", 6, 7, 6, 7, WhitespaceMode::COLLAPSE);
	assertFieldEnd(parser, 7, 8);

	assertFieldStart(parser, false, 8, 9);
	assertTextData(parser, "b", 9, 10, 9, 10, WhitespaceMode::COLLAPSE);
	assertFieldEnd(parser, 10, 11);

	assertTextData(parser, "c", 11, 13, 12, 13, WhitespaceMode::COLLAPSE);
	assertEnd(parser, 13, 13);
}

TEST(OsmlStreamParser, nestedCommand)
{
    const char *testString = "\\test{a}{\\test2{b} c} d";
    //                         012345678 90123456789012
    //                         0          1         2
    CharReader charReader(testString);
    OsmlStreamParser parser(charReader, logger);

    assertCommand(parser, "test", 0, 5);
    assertFieldStart(parser, false, 5, 6);
    assertData(parser, "a", 6, 7);
    assertFieldEnd(parser, 7, 8);

    assertFieldStart(parser, false, 8, 9);
    assertCommand(parser, "test2", 9, 15);
    assertFieldStart(parser, false, 15, 16);
    assertData(parser, "b", 16, 17);
    assertFieldEnd(parser, 17, 18);
    assertData(parser, "c", 19, 20);
    assertFieldEnd(parser, 20, 21);
    assertData(parser, "d", 22, 23);
    assertEnd(parser, 23, 23);
}


TEST(OsmlStreamParser, nestedCommandImmediateEnd)
{
    const char *testString = "\\test{\\test2{b}} d";
    //                         012345 678901234567
    //                         0          1
    CharReader charReader(testString);
    OsmlStreamParser parser(charReader, logger);

    assertCommand(parser, "test", 0, 5);
    assertFieldStart(parser, false, 5, 6);
    {
        assertCommand(parser, "test2", 6, 12);
        assertFieldStart(parser, false, 12, 13);
        assertData(parser, "b", 13, 14);
        assertFieldEnd(parser, 14, 15);
    }
    assertFieldEnd(parser, 15, 16);
    assertData(parser, "d", 17, 18);
    assertEnd(parser, 18, 18);
}

TEST(OsmlStreamParser, nestedCommandNoData)
{
    const char *testString = "\\test{\\test2}";
    //                         012345 6789012
    CharReader charReader(testString);
    OsmlStreamParser parser(charReader, logger);

    assertCommand(parser, "test", 0, 5);
    assertFieldStart(parser, false, 5, 6);
    assertCommand(parser, "test2", 6, 12);
    assertFieldEnd(parser, 12, 13);
    assertEnd(parser, 13, 13);
}

TEST(OsmlStreamParser, multipleCommands)
{
    const char *testString = "\\a \\b \\c \\d";
    //                         012 345 678 90
    //                         0            1
    CharReader charReader(testString);
    OsmlStreamParser parser(charReader, logger);

    assertCommand(parser, "a", 0, 2);
    assertEmptyData(parser);
    assertCommand(parser, "b", 3, 5);
    assertEmptyData(parser);
    assertCommand(parser, "c", 6, 8);
    assertEmptyData(parser);
    assertCommand(parser, "d", 9, 11);
    assertEnd(parser, 11, 11);
}

TEST(OsmlStreamParser, fieldsWithSpaces)
{
    const char *testString = "\\a {\\b \\c}   \n\n {\\d}";
    //                         0123 456 789012 3 456 789
    //                         0           1
    CharReader charReader(testString);
    OsmlStreamParser parser(charReader, logger);

    assertCommand(parser, "a", 0, 2);
    assertEmptyData(parser);
    assertFieldStart(parser, false, 3, 4);
    assertCommand(parser, "b", 4, 6);
    assertEmptyData(parser);
    assertCommand(parser, "c", 7, 9);
    assertFieldEnd(parser, 9, 10);
    assertEmptyData(parser);
    assertFieldStart(parser, false, 16, 17);
    assertCommand(parser, "d", 17, 19);
    assertFieldEnd(parser, 19, 20);
    assertEnd(parser, 20, 20);
}

TEST(OsmlStreamParser, errorEndButOpenField)
{
    const char *testString = "\\a b {";
    //                         012345
    //                         0
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    assertCommand(parser, "a", 0, 2);
    assertData(parser, "b", 3, 4);
    assertFieldStart(parser, false, 5, 6);
    ASSERT_FALSE(logger.hasError());
    assertEnd(parser, 6, 6);
    ASSERT_TRUE(logger.hasError());
}


TEST(OsmlStreamParser, errorNoFieldToEnd)
{
    const char *testString = "\\a b }";
    //                         012345
    //                         0
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    assertCommand(parser, "a", 0, 2);
    assertData(parser, "b", 3, 4);
    ASSERT_FALSE(logger.hasError());
    assertEnd(parser, 6, 6);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorNoFieldEndNested)
{
    const char *testString = "\\test{\\test2{}}}";
    //                         012345 6789012345
    //                         0          1
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    assertCommand(parser, "test", 0, 5);
    assertFieldStart(parser, false, 5, 6);
    assertCommand(parser, "test2", 6, 12);
    assertFieldStart(parser, false, 12, 13);
    assertFieldEnd(parser, 13, 14);
    assertFieldEnd(parser, 14, 15);
    ASSERT_FALSE(logger.hasError());
    assertEnd(parser, 16, 16);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorNoFieldEndNestedData)
{
    const char *testString = "\\test{\\test2{}}a}";
    //                         012345 67890123456
    //                         0          1
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    assertCommand(parser, "test", 0, 5);
    assertFieldStart(parser, false, 5, 6);
    assertCommand(parser, "test2", 6, 12);
    assertFieldStart(parser, false, 12, 13);
    assertFieldEnd(parser, 13, 14);
    assertFieldEnd(parser, 14, 15);
    assertData(parser, "a", 15, 16);
    ASSERT_FALSE(logger.hasError());
    assertEnd(parser, 17, 17);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, beginEnd)
{
    const char *testString = "\\begin{book}\\end{book}";
    //                         012345678901 2345678901
    //                         0         1          2
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommandStart(parser, "book", true, Variant::mapType{}, 7, 11);
    assertRangeEnd(parser, 17, 21);
    assertEnd(parser, 22, 22);
}

TEST(OsmlStreamParser, beginEndWithName)
{
    const char *testString = "\\begin{book#a}\\end{book}";
    //                         01234567890123 4567890123
    //                         0         1          2
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommandStart(parser, "book", true, {{"name", "a"}}, 7, 11);
    assertRangeEnd(parser, 19, 23);
    assertEnd(parser, 24, 24);
}

TEST(OsmlStreamParser, beginEndWithNameAndArgs)
{
    const char *testString = "\\begin{book#a}[a=1,b=2,c=\"test\"]\\end{book}";
    //                         0123456789012345678901234 56789 01 2345678901
    //                         0         1         2           3          4
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommandStart(parser, "book", true,
                  {{"name", "a"}, {"a", 1}, {"b", 2}, {"c", "test"}}, 7, 11);
    assertRangeEnd(parser, 37, 41);
    assertEnd(parser, 42, 42);
}

TEST(OsmlStreamParser, beginEndWithNameAndArgsMultipleFields)
{
    const char *testString =
        "\\begin{book#a}[a=1,b=2,c=\"test\"]{a \\test}{b \\test{}}\\end{book}";
    //    0123456789012345678901234 56789 01234 567890123 45678901 2345678901
    //    0         1         2           3          4          5          6
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommandStart(parser, "book", true,
                  {{"name", "a"}, {"a", 1}, {"b", 2}, {"c", "test"}}, 7, 11);
    assertFieldStart(parser, false, 32, 33);
    assertData(parser, "a", 33, 34);
    assertCommand(parser, "test", 35, 40);
    assertFieldEnd(parser, 40, 41);
    assertFieldStart(parser, false, 41, 42);
    assertData(parser, "b", 42, 43);
    assertCommand(parser, "test", 44, 49);
    assertFieldStart(parser, false, 49, 50);
    assertFieldEnd(parser, 50, 51);
    assertFieldEnd(parser, 51, 52);
    assertRangeEnd(parser, 57, 61);
    assertEnd(parser, 62, 62);
}

TEST(OsmlStreamParser, beginEndWithData)
{
    const char *testString = "\\begin{book}a\\end{book}";
    //                         0123456789012 3456789012
    //                         0         1          2
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommandStart(parser, "book", true, Variant::mapType{}, 7, 11);
    assertData(parser, "a", 12, 13);
    assertRangeEnd(parser, 18, 22);
    assertEnd(parser, 23, 23);
}

TEST(OsmlStreamParser, beginEndNested)
{
    const char *testString =
        "\\begin{a}{b} c \\begin{d}{e}{f} \\g{h} \\end{d}\\end{a}";
    //    012345678901234 5678901234567890 123456 7890123 4567890
    //    0         1          2         3           4          5
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommandStart(parser, "a", true, Variant::mapType{}, 7, 8);
    assertFieldStart(parser, false, 9, 10);
    assertData(parser, "b", 10, 11);
    assertFieldEnd(parser, 11, 12);

    assertData(parser, "c", 13, 14);

    assertCommandStart(parser, "d", true, Variant::mapType{}, 22, 23);
    assertFieldStart(parser, false, 24, 25);
    assertData(parser, "e", 25, 26);
    assertFieldEnd(parser, 26, 27);
    assertFieldStart(parser, false, 27, 28);
    assertData(parser, "f", 28, 29);
    assertFieldEnd(parser, 29, 30);

    assertEmptyData(parser);
    assertCommand(parser, "g", 31, 33);
    assertFieldStart(parser, false, 33, 34);
    assertData(parser, "h", 34, 35);
    assertFieldEnd(parser, 35, 36);
    assertEmptyData(parser);
    assertRangeEnd(parser, 42, 43);
    assertRangeEnd(parser, 49, 50);
    assertEnd(parser, 51, 51);
}

TEST(OsmlStreamParser, beginEndWithCommand)
{
    const char *testString = "\\begin{book}\\a{test}\\end{book}";
    //                         012345678901 23456789 0123456789
    //                         0         1           2
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommandStart(parser, "book", true, Variant::mapType{}, 7, 11);
    assertCommand(parser, "a", 12, 14);
    assertFieldStart(parser, false, 14, 15);
    assertData(parser, "test", 15, 19);
    assertFieldEnd(parser, 19, 20);
    assertRangeEnd(parser, 25, 29);
    assertEnd(parser, 30, 30);
}

TEST(OsmlStreamParser, beginEndNestedFields)
{
    const char *testString = "\\begin{book}a{{b{c}}}\\end{book}";
    //                         012345678901234567890 1234567890
    //                         0         1         2          3
    CharReader charReader(testString);
    OsmlStreamParser parser(charReader, logger);
    logger.reset();

    assertCommandStart(parser, "book", true, Variant::mapType{}, 7, 11);
    assertData(parser, "a", 12, 13);
    assertFieldStart(parser, false, 13, 14);
    assertFieldStart(parser, false, 14, 15);
    assertData(parser, "b", 15, 16);
    assertFieldStart(parser, false, 16, 17);
    assertData(parser, "c", 17, 18);
    assertFieldEnd(parser, 18, 19);
    assertFieldEnd(parser, 19, 20);
    assertFieldEnd(parser, 20, 21);
    assertRangeEnd(parser, 26, 30);
    assertEnd(parser, 31, 31);
}

TEST(OsmlStreamParser, errorBeginEndUnbalancedNestedFields)
{
    const char *testString = "\\begin{book}a{{b{c}}\\end{book}";
    //                         012345678901234567890 123456789
    //                         0         1         2
    CharReader charReader(testString);
    OsmlStreamParser parser(charReader, logger);
    logger.reset();

    assertCommandStart(parser, "book", true, Variant::mapType{}, 7, 11);
    assertData(parser, "a", 12, 13);
    assertFieldStart(parser, false, 13, 14);
    assertFieldStart(parser, false, 14, 15);
    assertData(parser, "b", 15, 16);
    assertFieldStart(parser, false, 16, 17);
    assertData(parser, "c", 17, 18);
    assertFieldEnd(parser, 18, 19);
    assertFieldEnd(parser, 19, 20);
    ASSERT_THROW(assertRangeEnd(parser, 25, 29), LoggableException);
}

TEST(OsmlStreamParser, errorBeginEndUnbalancedFields)
{
	const char *testString = "{a";
	//                        01
	CharReader charReader(testString);
	OsmlStreamParser parser(charReader, logger);

	logger.reset();

	assertFieldStart(parser, false, 0, 1);
	assertTextData(parser, "a", 1, 2, 1, 2, WhitespaceMode::PRESERVE);
	ASSERT_FALSE(logger.hasError());
	assertEnd(parser, 2, 2);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorBeginNoBraceOpen)
{
    const char *testString = "\\begin a";
    //                         01234567
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    assertData(parser, "a", 7, 8);
    ASSERT_TRUE(logger.hasError());
    assertEnd(parser, 8, 8);
}

TEST(OsmlStreamParser, errorBeginNoIdentifier)
{
    const char *testString = "\\begin{!";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    ASSERT_THROW(parser.parse(), LoggableException);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorBeginNoBraceClose)
{
    const char *testString = "\\begin{a";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    ASSERT_THROW(parser.parse(), LoggableException);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorBeginNoName)
{
    const char *testString = "\\begin{a#}";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    assertCommandStart(parser, "a", true);
    ASSERT_TRUE(logger.hasError());
    logger.reset();
    ASSERT_FALSE(logger.hasError());
    assertEnd(parser);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorEndNoBraceOpen)
{
    const char *testString = "\\end a";
    //                         012345
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    assertData(parser, "a", 5, 6);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorEndNoIdentifier)
{
    const char *testString = "\\end{!";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    ASSERT_THROW(parser.parse(), LoggableException);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorEndNoBraceClose)
{
    const char *testString = "\\end{a";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    ASSERT_THROW(parser.parse(), LoggableException);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorEndNoBegin)
{
    const char *testString = "\\end{a}";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    ASSERT_THROW(parser.parse(), LoggableException);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorBeginEndMismatch)
{
    const char *testString = "\\begin{a} \\begin{b} test \\end{a}";
    //                         0123456789 012345678901234 5678901
    //                         0          1         2          3
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    assertCommandStart(parser, "a", true, Variant::mapType{}, 7, 8);
    assertEmptyData(parser);
    assertCommandStart(parser, "b", true, Variant::mapType{}, 17, 18);
    assertData(parser, "test", 20, 24);
    ASSERT_FALSE(logger.hasError());
    ASSERT_THROW(parser.parse(), LoggableException);
    ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, commandWithNSSep)
{
    const char *testString = "\\test1:test2";
    //                         012345678901
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommand(parser, "test1:test2", 0, 12);
    assertEnd(parser, 12, 12);
}

TEST(OsmlStreamParser, beginEndWithNSSep)
{
    const char *testString = "\\begin{test1:test2}\\end{test1:test2}";
    //                         0123456789012345678 90123456789012345
    //                         0         1          2         3
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommandStart(parser, "test1:test2", true, Variant::mapType{}, 7, 18);
    assertRangeEnd(parser, 24, 35);
    assertEnd(parser, 36, 36);
}

TEST(OsmlStreamParser, errorBeginNSSep)
{
    const char *testString = "\\begin:test{blub}\\end{blub}";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    assertCommandStart(parser, "blub", true, Variant::mapType{});
    ASSERT_TRUE(logger.hasError());
    assertRangeEnd(parser);
    assertEnd(parser);
}

TEST(OsmlStreamParser, errorEndNSSep)
{
    const char *testString = "\\begin{blub}\\end:test{blub}";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    assertCommandStart(parser, "blub", true, Variant::mapType{});
    ASSERT_FALSE(logger.hasError());
    assertRangeEnd(parser);
    ASSERT_TRUE(logger.hasError());
    assertEnd(parser);
}

TEST(OsmlStreamParser, errorEmptyNs)
{
    const char *testString = "\\test:";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    assertCommand(parser, "test");
    ASSERT_TRUE(logger.hasError());
    assertData(parser, ":");
    assertEnd(parser);
}

TEST(OsmlStreamParser, errorRepeatedNs)
{
    const char *testString = "\\test::";
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    assertCommand(parser, "test");
    ASSERT_TRUE(logger.hasError());
    assertData(parser, "::");
    assertEnd(parser);
}

TEST(OsmlStreamParser, explicitDefaultField)
{
    const char *testString = "\\a{!b}c";
    //                         01234567
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommand(parser, "a", 0, 2);
    assertFieldStart(parser, true, 2, 4);
    assertData(parser, "b", 4, 5);
    assertFieldEnd(parser, 5, 6);
    assertData(parser, "c", 6, 7);
    assertEnd(parser, 7, 7);
}

TEST(OsmlStreamParser, explicitDefaultFieldWithCommand)
{
    const char *testString = "\\a{!\\b}c";
    //                         0123 4567
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertCommand(parser, "a", 0, 2);
    assertFieldStart(parser, true, 2, 4);
    assertCommand(parser, "b", 4, 6);
    assertFieldEnd(parser, 6, 7);
    assertData(parser, "c", 7, 8);
    assertEnd(parser, 8, 8);
}

TEST(OsmlStreamParser, fieldAfterExplicitDefaultField)
{
    const char *testString = "\\a{!\\b}{c}";
    //                         0123 456789
    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    assertCommand(parser, "a", 0, 2);
    assertFieldStart(parser, true, 2, 4);
    assertCommand(parser, "b", 4, 6);
    assertFieldEnd(parser, 6, 7);
    assertFieldStart(parser, false, 7, 8);
    assertData(parser, "c", 8, 9);
    assertFieldEnd(parser, 9, 10);
    assertEnd(parser, 10, 10);
}

TEST(OsmlStreamParser, annotationStart)
{
    const char *testString = "<\\a";
    //                        0 12

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertAnnotationStart(parser, "a", Variant::mapType{}, 0, 3);
    assertEnd(parser, 3, 3);
}

TEST(OsmlStreamParser, annotationStartWithName)
{
    const char *testString = "<\\annotationWithName#aName";
    //                        0 1234567890123456789012345
    //                        0          1         2

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertAnnotationStart(parser, "annotationWithName",
                          Variant::mapType{{"name", "aName"}}, 0, 20);
    assertEnd(parser, 26, 26);
}

TEST(OsmlStreamParser, annotationStartWithArguments)
{
    const char *testString = "<\\annotationWithName#aName[a=1,b=2]";
    //                        0 1234567890123456789012345678901234
    //                        0          1         2         3

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertAnnotationStart(
        parser, "annotationWithName",
        Variant::mapType{{"name", "aName"}, {"a", 1}, {"b", 2}}, 0, 20);
    assertEnd(parser, 35, 35);
}

TEST(OsmlStreamParser, simpleAnnotationStartBeginEnd)
{
    const char *testString = "<\\begin{ab#name}[a=1,b=2] a \\end{ab}\\>";
    //                        0 123456789012345678901234567 89012345 67
    //                        0          1         2          3

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertAnnotationStart(
        parser, "ab", Variant::mapType{{"name", "name"}, {"a", 1}, {"b", 2}}, 8,
        10);
    ASSERT_TRUE(parser.inRangeCommand());
    assertData(parser, "a", 26, 27);
    assertRangeEnd(parser, 33, 35);
    assertAnnotationEnd(parser, "", "", 36, 38);
    assertEnd(parser, 38, 38);
}

TEST(OsmlStreamParser, annotationEnd)
{
    const char *testString = "\\a>";
    //                         012

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertAnnotationEnd(parser, "a", "", 0, 2);
    assertEnd(parser, 3, 3);
}

TEST(OsmlStreamParser, annotationEndWithName)
{
    const char *testString = "\\a#name>";
    //                         01234567

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertAnnotationEnd(parser, "a", "name", 0, 2);
    assertEnd(parser, 8, 8);
}

TEST(OsmlStreamParser, annotationEndWithNameAsArgs)
{
    const char *testString = "\\a[name=name]>";
    //                         01234567890123

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertAnnotationEnd(parser, "a", "name", 0, 2);
    assertEnd(parser, 14, 14);
}

TEST(OsmlStreamParser, errorAnnotationEndWithArguments)
{
    const char *testString = "\\a[foo=bar]>";
    //                         012345678901
    //                         0         1

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    logger.reset();
    ASSERT_FALSE(logger.hasError());
    assertCommandStart(parser, "a", false, Variant::mapType{{"foo", "bar"}}, 0, 2);
    ASSERT_TRUE(logger.hasError());
    assertData(parser, ">", 11, 12);
    assertEnd(parser, 12, 12);
}

TEST(OsmlStreamParser, closingAnnotation)
{
    const char *testString = "<\\a>";
    //                        0 123

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertAnnotationStart(parser, "a", Variant::mapType{}, 0, 3);
    assertData(parser, ">", 3, 4);
    assertEnd(parser, 4, 4);
}

TEST(OsmlStreamParser, annotationWithFields)
{
    const char *testString = "a <\\b{c}{d}{!e} f \\> g";
    //                        012 345678901234567 8901
    //                        0          1          2

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertData(parser, "a", 0, 1);
    assertAnnotationStart(parser, "b", Variant::mapType{}, 2, 5);
    assertFieldStart(parser, false, 5, 6);
    assertData(parser, "c", 6, 7);
    assertFieldEnd(parser, 7, 8);
    assertFieldStart(parser, false, 8, 9);
    assertData(parser, "d", 9, 10);
    assertFieldEnd(parser, 10, 11);
    assertFieldStart(parser, true, 11, 13);
    assertData(parser, "e", 13, 14);
    assertFieldEnd(parser, 14, 15);
    assertData(parser, "f", 16, 17);
    assertAnnotationEnd(parser, "", "", 18, 20);
    assertData(parser, "g", 21, 22);
    assertEnd(parser, 22, 22);
}

TEST(OsmlStreamParser, annotationStartEscape)
{
    const char *testString = "<\\%test";
    //                        0 123456
    //                        0

    CharReader charReader(testString);

    OsmlStreamParser parser(charReader, logger);

    assertData(parser, "<%test", 0, 7);
    assertEnd(parser, 7, 7);
}

}

