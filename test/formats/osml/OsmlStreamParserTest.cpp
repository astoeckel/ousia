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

#include <formats/osml/OsmlStreamParser.hpp>

namespace ousia {

static TerminalLogger logger(std::cerr, true);
// static ConcreteLogger logger;

static void assertCommand(OsmlStreamParser &reader, const std::string &name,
                          SourceOffset start = InvalidSourceOffset,
                          SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::COMMAND, reader.parse());
	EXPECT_EQ(name, reader.getCommandName().asString());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, reader.getCommandName().getLocation().getStart());
		EXPECT_EQ(start, reader.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, reader.getCommandName().getLocation().getEnd());
		EXPECT_EQ(end, reader.getLocation().getEnd());
	}
}

static void assertCommand(OsmlStreamParser &reader, const std::string &name,
                          const Variant::mapType &args,
                          SourceOffset start = InvalidSourceOffset,
                          SourceOffset end = InvalidSourceOffset)
{
	assertCommand(reader, name, start, end);
	EXPECT_EQ(args, reader.getCommandArguments());
}

static void assertData(OsmlStreamParser &reader, const std::string &data,
                       SourceOffset start = InvalidSourceOffset,
                       SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::DATA, reader.parse());
	EXPECT_EQ(data, reader.getData().asString());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, reader.getData().getLocation().getStart());
		EXPECT_EQ(start, reader.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, reader.getData().getLocation().getEnd());
		EXPECT_EQ(end, reader.getLocation().getEnd());
	}
}

static void assertFieldStart(OsmlStreamParser &reader, bool defaultField,
                             SourceOffset start = InvalidSourceOffset,
                             SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::FIELD_START, reader.parse());
	EXPECT_EQ(defaultField, reader.inDefaultField());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, reader.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, reader.getLocation().getEnd());
	}
}

static void assertFieldEnd(OsmlStreamParser &reader,
                           SourceOffset start = InvalidSourceOffset,
                           SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::FIELD_END, reader.parse());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, reader.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, reader.getLocation().getEnd());
	}
}

static void assertAnnotationStart(OsmlStreamParser &reader,
                                  const std::string &name,
                                  SourceOffset start = InvalidSourceOffset,
                                  SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::ANNOTATION_START, reader.parse());
	EXPECT_EQ(name, reader.getCommandName().asString());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, reader.getCommandName().getLocation().getStart());
		EXPECT_EQ(start, reader.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, reader.getCommandName().getLocation().getEnd());
		EXPECT_EQ(end, reader.getLocation().getEnd());
	}
}

static void assertAnnotationStart(OsmlStreamParser &reader,
                                  const std::string &name,
                                  const Variant::mapType &args,
                                  SourceOffset start = InvalidSourceOffset,
                                  SourceOffset end = InvalidSourceOffset)
{
	assertAnnotationStart(reader, name, start, end);
	EXPECT_EQ(args, reader.getCommandArguments());
}

static void assertAnnotationEnd(OsmlStreamParser &reader,
                                const std::string &name,
                                const std::string &elementName,
                                SourceOffset start = InvalidSourceOffset,
                                SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::ANNOTATION_END, reader.parse());
	ASSERT_EQ(name, reader.getCommandName().asString());
	if (!elementName.empty()) {
		ASSERT_EQ(1U, reader.getCommandArguments().asMap().size());
		ASSERT_EQ(1U, reader.getCommandArguments().asMap().count("name"));

		auto it = reader.getCommandArguments().asMap().find("name");
		ASSERT_EQ(elementName, it->second.asString());
	}
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, reader.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, reader.getLocation().getEnd());
	}
}

static void assertEnd(OsmlStreamParser &reader,
                      SourceOffset start = InvalidSourceOffset,
                      SourceOffset end = InvalidSourceOffset)
{
	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, reader.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, reader.getLocation().getEnd());
	}
}

TEST(OsmlStreamParser, empty)
{
	const char *testString = "";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, oneCharacter)
{
	const char *testString = "a";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertData(reader, "a", 0, 1);
}

TEST(OsmlStreamParser, whitespaceElimination)
{
	const char *testString = " hello \t world ";
	//                        0123456 78901234
	//                        0          1
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertData(reader, "hello world", 1, 14);
}

TEST(OsmlStreamParser, whitespaceEliminationWithLinebreak)
{
	const char *testString = " hello \n world ";
	//                        0123456 78901234
	//                        0          1
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertData(reader, "hello world", 1, 14);
}

TEST(OsmlStreamParser, escapeWhitespace)
{
	const char *testString = " hello\\ \\ world ";
	//                        012345 67 89012345
	//                        0           1
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertData(reader, "hello  world", 1, 15);
}

static void testEscapeSpecialCharacter(const std::string &c)
{
	CharReader charReader(std::string("\\") + c);
	OsmlStreamParser reader(charReader, logger);
	EXPECT_EQ(OsmlStreamParser::State::DATA, reader.parse());
	EXPECT_EQ(c, reader.getData().asString());

	SourceLocation loc = reader.getData().getLocation();
	EXPECT_EQ(0U, loc.getStart());
	EXPECT_EQ(1U + c.size(), loc.getEnd());
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
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);
	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, singleLineComment)
{
	const char *testString = "a% This is a single line comment\nb";
	//                        01234567890123456789012345678901 23
	//                        0         1         2         3
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);
	{
		ASSERT_EQ(OsmlStreamParser::State::DATA, reader.parse());
		ASSERT_EQ("a", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(1U, loc.getEnd());
	}

	{
		ASSERT_EQ(OsmlStreamParser::State::DATA, reader.parse());
		ASSERT_EQ("b", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(33U, loc.getStart());
		ASSERT_EQ(34U, loc.getEnd());
	}

	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, multilineComment)
{
	const char *testString = "a%{ This is a\n\n multiline line comment}%b";
	//                        0123456789012 3 456789012345678901234567890
	//                        0         1           2         3         4
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);
	{
		ASSERT_EQ(OsmlStreamParser::State::DATA, reader.parse());
		ASSERT_EQ("a", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(1U, loc.getEnd());
	}

	{
		ASSERT_EQ(OsmlStreamParser::State::DATA, reader.parse());
		ASSERT_EQ("b", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(40U, loc.getStart());
		ASSERT_EQ(41U, loc.getEnd());
	}

	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, nestedMultilineComment)
{
	const char *testString = "a%{%{Another\n\n}%multiline line comment}%b";
	//                        0123456789012 3 456789012345678901234567890
	//                        0         1           2         3         4
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);
	{
		ASSERT_EQ(OsmlStreamParser::State::DATA, reader.parse());
		ASSERT_EQ("a", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(0U, loc.getStart());
		ASSERT_EQ(1U, loc.getEnd());
	}

	{
		ASSERT_EQ(OsmlStreamParser::State::DATA, reader.parse());
		ASSERT_EQ("b", reader.getData().asString());
		SourceLocation loc = reader.getData().getLocation();
		ASSERT_EQ(40U, loc.getStart());
		ASSERT_EQ(41U, loc.getEnd());
	}

	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, simpleCommand)
{
	const char *testString = "\\test";
	//                        0 12345
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);
	ASSERT_EQ(OsmlStreamParser::State::COMMAND, reader.parse());

	Variant commandName = reader.getCommandName();
	ASSERT_EQ("test", commandName.asString());

	SourceLocation loc = commandName.getLocation();
	ASSERT_EQ(0U, loc.getStart());
	ASSERT_EQ(5U, loc.getEnd());

	ASSERT_EQ(0U, reader.getCommandArguments().asMap().size());
	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, simpleCommandWithName)
{
	const char *testString = "\\test#bla";
	//                        0 12345678
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);
	ASSERT_EQ(OsmlStreamParser::State::COMMAND, reader.parse());

	Variant commandName = reader.getCommandName();
	ASSERT_EQ("test", commandName.asString());
	SourceLocation loc = commandName.getLocation();
	ASSERT_EQ(0U, loc.getStart());
	ASSERT_EQ(5U, loc.getEnd());

	Variant commandArguments = reader.getCommandArguments();
	ASSERT_TRUE(commandArguments.isMap());
	ASSERT_EQ(1U, commandArguments.asMap().size());
	ASSERT_EQ(1U, commandArguments.asMap().count("name"));
	ASSERT_EQ("bla", commandArguments.asMap()["name"].asString());

	loc = commandArguments.asMap()["name"].getLocation();
	ASSERT_EQ(5U, loc.getStart());
	ASSERT_EQ(9U, loc.getEnd());

	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, simpleCommandWithArguments)
{
	const char *testString = "\\test[a=1,b=2,c=\"test\"]";
	//                        0 123456789012345 678901 2
	//                        0          1          2
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);
	ASSERT_EQ(OsmlStreamParser::State::COMMAND, reader.parse());

	Variant commandName = reader.getCommandName();
	ASSERT_EQ("test", commandName.asString());
	SourceLocation loc = commandName.getLocation();
	ASSERT_EQ(0U, loc.getStart());
	ASSERT_EQ(5U, loc.getEnd());

	Variant commandArguments = reader.getCommandArguments();
	ASSERT_TRUE(commandArguments.isMap());
	ASSERT_EQ(3U, commandArguments.asMap().size());
	ASSERT_EQ(1U, commandArguments.asMap().count("a"));
	ASSERT_EQ(1U, commandArguments.asMap().count("b"));
	ASSERT_EQ(1U, commandArguments.asMap().count("c"));
	ASSERT_EQ(1, commandArguments.asMap()["a"].asInt());
	ASSERT_EQ(2, commandArguments.asMap()["b"].asInt());
	ASSERT_EQ("test", commandArguments.asMap()["c"].asString());

	loc = commandArguments.asMap()["a"].getLocation();
	ASSERT_EQ(8U, loc.getStart());
	ASSERT_EQ(9U, loc.getEnd());

	loc = commandArguments.asMap()["b"].getLocation();
	ASSERT_EQ(12U, loc.getStart());
	ASSERT_EQ(13U, loc.getEnd());

	loc = commandArguments.asMap()["c"].getLocation();
	ASSERT_EQ(16U, loc.getStart());
	ASSERT_EQ(22U, loc.getEnd());

	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, simpleCommandWithArgumentsAndName)
{
	const char *testString = "\\test#bla[a=1,b=2,c=\"test\"]";
	//                        0 1234567890123456789 01234 56
	//                        0          1          2
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);
	ASSERT_EQ(OsmlStreamParser::State::COMMAND, reader.parse());

	Variant commandName = reader.getCommandName();
	ASSERT_EQ("test", commandName.asString());
	SourceLocation loc = commandName.getLocation();
	ASSERT_EQ(0U, loc.getStart());
	ASSERT_EQ(5U, loc.getEnd());

	Variant commandArguments = reader.getCommandArguments();
	ASSERT_TRUE(commandArguments.isMap());
	ASSERT_EQ(4U, commandArguments.asMap().size());
	ASSERT_EQ(1U, commandArguments.asMap().count("a"));
	ASSERT_EQ(1U, commandArguments.asMap().count("b"));
	ASSERT_EQ(1U, commandArguments.asMap().count("c"));
	ASSERT_EQ(1U, commandArguments.asMap().count("name"));
	ASSERT_EQ(1, commandArguments.asMap()["a"].asInt());
	ASSERT_EQ(2, commandArguments.asMap()["b"].asInt());
	ASSERT_EQ("test", commandArguments.asMap()["c"].asString());
	ASSERT_EQ("bla", commandArguments.asMap()["name"].asString());

	loc = commandArguments.asMap()["a"].getLocation();
	ASSERT_EQ(12U, loc.getStart());
	ASSERT_EQ(13U, loc.getEnd());

	loc = commandArguments.asMap()["b"].getLocation();
	ASSERT_EQ(16U, loc.getStart());
	ASSERT_EQ(17U, loc.getEnd());

	loc = commandArguments.asMap()["c"].getLocation();
	ASSERT_EQ(20U, loc.getStart());
	ASSERT_EQ(26U, loc.getEnd());

	loc = commandArguments.asMap()["name"].getLocation();
	ASSERT_EQ(5U, loc.getStart());
	ASSERT_EQ(9U, loc.getEnd());

	ASSERT_EQ(OsmlStreamParser::State::END, reader.parse());
}

TEST(OsmlStreamParser, fields)
{
	const char *testString = "\\test{a}{b}{c}";
	//                         01234567890123
	//                         0         1
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "test", 0, 5);
	assertFieldStart(reader, false, 5, 6);
	assertData(reader, "a", 6, 7);
	assertFieldEnd(reader, 7, 8);

	assertFieldStart(reader, false, 8, 9);
	assertData(reader, "b", 9, 10);
	assertFieldEnd(reader, 10, 11);

	assertFieldStart(reader, false, 11, 12);
	assertData(reader, "c", 12, 13);
	assertFieldEnd(reader, 13, 14);
	assertEnd(reader, 14, 14);
}

TEST(OsmlStreamParser, dataOutsideField)
{
	const char *testString = "\\test{a}{b} c";
	//                         0123456789012
	//                         0         1
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "test", 0, 5);
	assertFieldStart(reader, false, 5, 6);
	assertData(reader, "a", 6, 7);
	assertFieldEnd(reader, 7, 8);

	assertFieldStart(reader, false, 8, 9);
	assertData(reader, "b", 9, 10);
	assertFieldEnd(reader, 10, 11);

	assertData(reader, "c", 12, 13);
	assertEnd(reader, 13, 13);
}

TEST(OsmlStreamParser, nestedCommand)
{
	const char *testString = "\\test{a}{\\test2{b} c} d";
	//                         012345678 90123456789012
	//                         0          1         2
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "test", 0, 5);

	assertFieldStart(reader, false, 5, 6);
	assertData(reader, "a", 6, 7);
	assertFieldEnd(reader, 7, 8);

	assertFieldStart(reader, false, 8, 9);
	{
		assertCommand(reader, "test2", 9, 15);
		assertFieldStart(reader, false, 15, 16);
		assertData(reader, "b", 16, 17);
		assertFieldEnd(reader, 17, 18);
	}
	assertData(reader, "c", 19, 20);
	assertFieldEnd(reader, 20, 21);
	assertData(reader, "d", 22, 23);
	assertEnd(reader, 23, 23);
}

TEST(OsmlStreamParser, nestedCommandImmediateEnd)
{
	const char *testString = "\\test{\\test2{b}} d";
	//                         012345 678901234567
	//                         0          1
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "test", 0, 5);
	assertFieldStart(reader, false, 5, 6);
	{
		assertCommand(reader, "test2", 6, 12);
		assertFieldStart(reader, false, 12, 13);
		assertData(reader, "b", 13, 14);
		assertFieldEnd(reader, 14, 15);
	}
	assertFieldEnd(reader, 15, 16);
	assertData(reader, "d", 17, 18);
	assertEnd(reader, 18, 18);
}

TEST(OsmlStreamParser, nestedCommandNoData)
{
	const char *testString = "\\test{\\test2}";
	//                         012345 6789012
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "test", 0, 5);
	assertFieldStart(reader, false, 5, 6);
	assertCommand(reader, "test2", 6, 12);
	assertFieldEnd(reader, 12, 13);
	assertEnd(reader, 13, 13);
}

TEST(OsmlStreamParser, multipleCommands)
{
	const char *testString = "\\a \\b \\c \\d";
	//                         012 345 678 90
	//                         0            1
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "a", 0, 2);
	assertCommand(reader, "b", 3, 5);
	assertCommand(reader, "c", 6, 8);
	assertCommand(reader, "d", 9, 11);
	assertEnd(reader, 11, 11);
}

TEST(OsmlStreamParser, fieldsWithSpaces)
{
	const char *testString = "\\a {\\b \\c}   \n\n {\\d}";
	//                         0123 456 789012 3 456 789
	//                         0           1
	CharReader charReader(testString);
	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "a", 0, 2);
	assertFieldStart(reader, false, 3, 4);
	assertCommand(reader, "b", 4, 6);
	assertCommand(reader, "c", 7, 9);
	assertFieldEnd(reader, 9, 10);
	assertFieldStart(reader, false, 16, 17);
	assertCommand(reader, "d", 17, 19);
	assertFieldEnd(reader, 19, 20);
	assertEnd(reader, 20, 20);
}

TEST(OsmlStreamParser, errorNoFieldToStart)
{
	const char *testString = "\\a b {";
	//                         012345
	//                         0
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	assertCommand(reader, "a", 0, 2);
	assertData(reader, "b", 3, 4);
	ASSERT_FALSE(logger.hasError());
	assertEnd(reader, 6, 6);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorNoFieldToEnd)
{
	const char *testString = "\\a b }";
	//                         012345
	//                         0
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	assertCommand(reader, "a", 0, 2);
	assertData(reader, "b", 3, 4);
	ASSERT_FALSE(logger.hasError());
	assertEnd(reader, 6, 6);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorNoFieldEndNested)
{
	const char *testString = "\\test{\\test2{}}}";
	//                         012345 6789012345
	//                         0          1
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	assertCommand(reader, "test", 0, 5);
	assertFieldStart(reader, false, 5, 6);
	assertCommand(reader, "test2", 6, 12);
	assertFieldStart(reader, false, 12, 13);
	assertFieldEnd(reader, 13, 14);
	assertFieldEnd(reader, 14, 15);
	ASSERT_FALSE(logger.hasError());
	assertEnd(reader, 16, 16);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorNoFieldEndNestedData)
{
	const char *testString = "\\test{\\test2{}}a}";
	//                         012345 67890123456
	//                         0          1
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	assertCommand(reader, "test", 0, 5);
	assertFieldStart(reader, false, 5, 6);
	assertCommand(reader, "test2", 6, 12);
	assertFieldStart(reader, false, 12, 13);
	assertFieldEnd(reader, 13, 14);
	assertFieldEnd(reader, 14, 15);
	assertData(reader, "a", 15, 16);
	ASSERT_FALSE(logger.hasError());
	assertEnd(reader, 17, 17);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, beginEnd)
{
	const char *testString = "\\begin{book}\\end{book}";
	//                         012345678901 2345678901
	//                         0         1          2
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "book", 7, 11);
	assertFieldStart(reader, true, 12, 13);
	assertFieldEnd(reader, 17, 21);
	assertEnd(reader, 22, 22);
}

TEST(OsmlStreamParser, beginEndWithName)
{
	const char *testString = "\\begin{book#a}\\end{book}";
	//                         01234567890123 4567890123
	//                         0         1          2
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "book", {{"name", "a"}}, 7, 11);
	assertFieldStart(reader, true, 14, 15);
	assertFieldEnd(reader, 19, 23);
	assertEnd(reader, 24, 24);
}

TEST(OsmlStreamParser, beginEndWithNameAndArgs)
{
	const char *testString = "\\begin{book#a}[a=1,b=2,c=\"test\"]\\end{book}";
	//                         0123456789012345678901234 56789 01 2345678901
	//                         0         1         2           3          4
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "book",
	              {{"name", "a"}, {"a", 1}, {"b", 2}, {"c", "test"}}, 7, 11);
	assertFieldStart(reader, true, 32, 33);
	assertFieldEnd(reader, 37, 41);
	assertEnd(reader, 42, 42);
}

TEST(OsmlStreamParser, beginEndWithNameAndArgsMultipleFields)
{
	const char *testString =
	    "\\begin{book#a}[a=1,b=2,c=\"test\"]{a \\test}{b \\test{}}\\end{book}";
	//    0123456789012345678901234 56789 01234 567890123 45678901 2345678901
	//    0         1         2           3          4          5          6
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "book",
	              {{"name", "a"}, {"a", 1}, {"b", 2}, {"c", "test"}}, 7, 11);
	assertFieldStart(reader, false, 32, 33);
	assertData(reader, "a", 33, 34);
	assertCommand(reader, "test", Variant::mapType{}, 35, 40);
	assertFieldEnd(reader, 40, 41);
	assertFieldStart(reader, false, 41, 42);
	assertData(reader, "b", 42, 43);
	assertCommand(reader, "test", Variant::mapType{}, 44, 49);
	assertFieldStart(reader, false, 49, 50);
	assertFieldEnd(reader, 50, 51);
	assertFieldEnd(reader, 51, 52);
	assertFieldStart(reader, true, 52, 53);
	assertFieldEnd(reader, 57, 61);
	assertEnd(reader, 62, 62);
}

TEST(OsmlStreamParser, beginEndWithData)
{
	const char *testString = "\\begin{book}a\\end{book}";
	//                         0123456789012 3456789012
	//                         0         1          2
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "book", 7, 11);
	assertFieldStart(reader, true, 12, 13);
	assertData(reader, "a", 12, 13);
	assertFieldEnd(reader, 18, 22);
	assertEnd(reader, 23, 23);
}

TEST(OsmlStreamParser, beginEndNested)
{
	const char *testString =
	    "\\begin{a}{b} c \\begin{d}{e}{f} \\g{h} \\end{d}\\end{a}";
	//    012345678901234 5678901234567890 123456 7890123 4567890
	//    0         1          2         3           4          5
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "a", 7, 8);
	assertFieldStart(reader, false, 9, 10);
	assertData(reader, "b", 10, 11);
	assertFieldEnd(reader, 11, 12);
	assertFieldStart(reader, true, 13, 14);
	assertData(reader, "c", 13, 14);
	assertCommand(reader, "d", 22, 23);
	assertFieldStart(reader, false, 24, 25);
	assertData(reader, "e", 25, 26);
	assertFieldEnd(reader, 26, 27);
	assertFieldStart(reader, false, 27, 28);
	assertData(reader, "f", 28, 29);
	assertFieldEnd(reader, 29, 30);
	assertFieldStart(reader, true, 31, 32);
	assertCommand(reader, "g", 31, 33);
	assertFieldStart(reader, false, 33, 34);
	assertData(reader, "h", 34, 35);
	assertFieldEnd(reader, 35, 36);
	assertFieldEnd(reader, 42, 43);
	assertFieldEnd(reader, 49, 50);
	assertEnd(reader, 51, 51);
}

TEST(OsmlStreamParser, beginEndWithCommand)
{
	const char *testString = "\\begin{book}\\a{test}\\end{book}";
	//                         012345678901 23456789 0123456789
	//                         0         1           2
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "book", 7, 11);
	assertFieldStart(reader, true, 12, 13);
	assertCommand(reader, "a", 12, 14);
	assertFieldStart(reader, false, 14, 15);
	assertData(reader, "test", 15, 19);
	assertFieldEnd(reader, 19, 20);
	assertFieldEnd(reader, 25, 29);
	assertEnd(reader, 30, 30);
}

TEST(OsmlStreamParser, errorBeginNoBraceOpen)
{
	const char *testString = "\\begin a";
	//                         01234567
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	assertData(reader, "a", 7, 8);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorBeginNoIdentifier)
{
	const char *testString = "\\begin{!";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	ASSERT_THROW(reader.parse(), LoggableException);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorBeginNoBraceClose)
{
	const char *testString = "\\begin{a";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	ASSERT_THROW(reader.parse(), LoggableException);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorBeginNoName)
{
	const char *testString = "\\begin{a#}";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	assertCommand(reader, "a");
	ASSERT_TRUE(logger.hasError());
	logger.reset();
	ASSERT_FALSE(logger.hasError());
	assertEnd(reader);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorEndNoBraceOpen)
{
	const char *testString = "\\end a";
	//                         012345
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	assertData(reader, "a", 5, 6);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorEndNoIdentifier)
{
	const char *testString = "\\end{!";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	ASSERT_THROW(reader.parse(), LoggableException);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorEndNoBraceClose)
{
	const char *testString = "\\end{a";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	ASSERT_THROW(reader.parse(), LoggableException);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorEndNoBegin)
{
	const char *testString = "\\end{a}";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	ASSERT_THROW(reader.parse(), LoggableException);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, errorBeginEndMismatch)
{
	const char *testString = "\\begin{a} \\begin{b} test \\end{a}";
	//                         0123456789 012345678901234 5678901
	//                         0          1         2          3
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	assertCommand(reader, "a", 7, 8);
	assertFieldStart(reader, true, 10, 11);
	assertCommand(reader, "b", 17, 18);
	assertFieldStart(reader, true, 20, 24);
	assertData(reader, "test", 20, 24);
	ASSERT_FALSE(logger.hasError());
	ASSERT_THROW(reader.parse(), LoggableException);
	ASSERT_TRUE(logger.hasError());
}

TEST(OsmlStreamParser, commandWithNSSep)
{
	const char *testString = "\\test1:test2";
	//                         012345678901
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "test1:test2", 0, 12);
	assertEnd(reader, 12, 12);
}

TEST(OsmlStreamParser, beginEndWithNSSep)
{
	const char *testString = "\\begin{test1:test2}\\end{test1:test2}";
	//                         0123456789012345678 90123456789012345
	//                         0         1          2         3
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "test1:test2", 7, 18);
	assertFieldStart(reader, true, 19, 20);
	assertFieldEnd(reader, 24, 35);
	assertEnd(reader, 36, 36);
}

TEST(OsmlStreamParser, errorBeginNSSep)
{
	const char *testString = "\\begin:test{blub}\\end{blub}";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	assertCommand(reader, "blub");
	ASSERT_TRUE(logger.hasError());
	assertFieldStart(reader, true);
	assertFieldEnd(reader);
	assertEnd(reader);
}

TEST(OsmlStreamParser, errorEndNSSep)
{
	const char *testString = "\\begin{blub}\\end:test{blub}";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	assertCommand(reader, "blub");
	assertFieldStart(reader, true);
	ASSERT_FALSE(logger.hasError());
	assertFieldEnd(reader);
	ASSERT_TRUE(logger.hasError());
	assertEnd(reader);
}

TEST(OsmlStreamParser, errorEmptyNs)
{
	const char *testString = "\\test:";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	assertCommand(reader, "test");
	ASSERT_TRUE(logger.hasError());
	assertData(reader, ":");
	assertEnd(reader);
}

TEST(OsmlStreamParser, errorRepeatedNs)
{
	const char *testString = "\\test::";
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	assertCommand(reader, "test");
	ASSERT_TRUE(logger.hasError());
	assertData(reader, "::");
	assertEnd(reader);
}

TEST(OsmlStreamParser, explicitDefaultField)
{
	const char *testString = "\\a{!b}c";
	//                         01234567
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "a", 0, 2);
	assertFieldStart(reader, true, 2, 4);
	assertData(reader, "b", 4, 5);
	assertFieldEnd(reader, 5, 6);
	assertData(reader, "c", 6, 7);
	assertEnd(reader, 7, 7);
}

TEST(OsmlStreamParser, explicitDefaultFieldWithCommand)
{
	const char *testString = "\\a{!\\b}c";
	//                         0123 4567
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertCommand(reader, "a", 0, 2);
	assertFieldStart(reader, true, 2, 4);
	assertCommand(reader, "b", 4, 6);
	assertFieldEnd(reader, 6, 7);
	assertData(reader, "c", 7, 8);
	assertEnd(reader, 8, 8);
}

TEST(OsmlStreamParser, errorFieldAfterExplicitDefaultField)
{
	const char *testString = "\\a{!\\b}{c}";
	//                         0123 456789
	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	assertCommand(reader, "a", 0, 2);
	assertFieldStart(reader, true, 2, 4);
	assertCommand(reader, "b", 4, 6);
	assertFieldEnd(reader, 6, 7);
	ASSERT_FALSE(logger.hasError());
	assertData(reader, "c", 8, 9);
	ASSERT_TRUE(logger.hasError());
	assertEnd(reader, 10, 10);
}

TEST(OsmlStreamParser, annotationStart)
{
	const char *testString = "<\\a";
	//                        0 12

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertAnnotationStart(reader, "a", Variant::mapType{}, 0, 3);
	assertEnd(reader, 3, 3);
}

TEST(OsmlStreamParser, annotationStartWithName)
{
	const char *testString = "<\\annotationWithName#aName";
	//                        0 1234567890123456789012345
	//                        0          1         2

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertAnnotationStart(reader, "annotationWithName",
	                      Variant::mapType{{"name", "aName"}}, 0, 20);
	assertEnd(reader, 26, 26);
}

TEST(OsmlStreamParser, annotationStartWithArguments)
{
	const char *testString = "<\\annotationWithName#aName[a=1,b=2]";
	//                        0 1234567890123456789012345678901234
	//                        0          1         2         3

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertAnnotationStart(
	    reader, "annotationWithName",
	    Variant::mapType{{"name", "aName"}, {"a", 1}, {"b", 2}}, 0, 20);
	assertEnd(reader, 35, 35);
}

TEST(OsmlStreamParser, simpleAnnotationStartBeginEnd)
{
	const char *testString = "<\\begin{ab#name}[a=1,b=2] a \\end{ab}\\>";
	//                        0 123456789012345678901234567 89012345 67
	//                        0          1         2          3

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertAnnotationStart(
	    reader, "ab", Variant::mapType{{"name", "name"}, {"a", 1}, {"b", 2}}, 8,
	    10);
	assertFieldStart(reader, true, 26, 27);
	assertData(reader, "a", 26, 27);
	assertFieldEnd(reader, 33, 35);
	assertAnnotationEnd(reader, "", "", 36, 38);
	assertEnd(reader, 38, 38);
}

TEST(OsmlStreamParser, annotationEnd)
{
	const char *testString = "\\a>";
	//                         012

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertAnnotationEnd(reader, "a", "", 0, 2);
	assertEnd(reader, 3, 3);
}

TEST(OsmlStreamParser, annotationEndWithName)
{
	const char *testString = "\\a#name>";
	//                         01234567

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertAnnotationEnd(reader, "a", "name", 0, 2);
	assertEnd(reader, 8, 8);
}

TEST(OsmlStreamParser, annotationEndWithNameAsArgs)
{
	const char *testString = "\\a[name=name]>";
	//                         01234567890123

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertAnnotationEnd(reader, "a", "name", 0, 2);
	assertEnd(reader, 14, 14);
}

TEST(OsmlStreamParser, errorAnnotationEndWithArguments)
{
	const char *testString = "\\a[foo=bar]>";
	//                         012345678901
	//                         0         1

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	logger.reset();
	ASSERT_FALSE(logger.hasError());
	assertCommand(reader, "a", Variant::mapType{{"foo", "bar"}}, 0, 2);
	ASSERT_TRUE(logger.hasError());
	assertData(reader, ">", 11, 12);
	assertEnd(reader, 12, 12);
}

TEST(OsmlStreamParser, closingAnnotation)
{
	const char *testString = "<\\a>";
	//                        0 123

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertAnnotationStart(reader, "a", Variant::mapType{}, 0, 3);
	assertData(reader, ">", 3, 4);
	assertEnd(reader, 4, 4);
}

TEST(OsmlStreamParser, annotationWithFields)
{
	const char *testString = "a <\\b{c}{d}{!e} f \\> g";
	//                        012 345678901234567 8901
	//                        0          1          2

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertData(reader, "a", 0, 1);
	assertAnnotationStart(reader, "b", Variant::mapType{}, 2, 5);
	assertFieldStart(reader, false, 5, 6);
	assertData(reader, "c", 6, 7);
	assertFieldEnd(reader, 7, 8);
	assertFieldStart(reader, false, 8, 9);
	assertData(reader, "d", 9, 10);
	assertFieldEnd(reader, 10, 11);
	assertFieldStart(reader, true, 11, 13);
	assertData(reader, "e", 13, 14);
	assertFieldEnd(reader, 14, 15);
	assertData(reader, "f", 16, 17);
	assertAnnotationEnd(reader, "", "", 18, 20);
	assertData(reader, "g", 21, 22);
	assertEnd(reader, 22, 22);
}

TEST(OsmlStreamParser, annotationStartEscape)
{
	const char *testString = "<\\%test";
	//                        0 123456
	//                        0

	CharReader charReader(testString);

	OsmlStreamParser reader(charReader, logger);

	assertData(reader, "<%test", 0, 7);
	assertEnd(reader, 7, 7);
}
}

