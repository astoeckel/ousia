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

#include <core/frontend/TerminalLogger.hpp>
#include <core/common/CharReader.hpp>
#include <core/common/Variant.hpp>

#include <formats/osxml/OsxmlEventParser.hpp>

namespace ousia {

static TerminalLogger logger(std::cerr, true);
// static ConcreteLogger logger;

namespace {
enum class OsxmlEvent {
	COMMAND_START,
	ANNOTATION_START,
	ANNOTATION_END,
	FIELD_END,
	DATA
};

class TestOsxmlEventListener : public OsxmlEvents {
public:
	std::vector<std::pair<OsxmlEvent, Variant>> events;

	void commandStart(Variant name, Variant args) override
	{
		events.emplace_back(OsxmlEvent::COMMAND_START,
		                    Variant::arrayType{name, args});
	}

	void annotationStart(Variant name, Variant args) override
	{
		events.emplace_back(OsxmlEvent::ANNOTATION_START,
		                    Variant::arrayType{name, args});
	}

	void annotationEnd(Variant name, Variant elementName) override
	{
		events.emplace_back(OsxmlEvent::ANNOTATION_END,
		                    Variant::arrayType{name, elementName});
	}

	void fieldEnd() override
	{
		events.emplace_back(OsxmlEvent::FIELD_END, Variant::arrayType{});
	}

	void data(Variant data) override
	{
		events.emplace_back(OsxmlEvent::DATA, Variant::arrayType{data});
	}
};

static std::vector<std::pair<OsxmlEvent, Variant>> parseXml(
    const char *testString,
    WhitespaceMode whitespaceMode = WhitespaceMode::TRIM)
{
	TestOsxmlEventListener listener;
	CharReader reader(testString);
	OsxmlEventParser parser(reader, listener, logger);
	parser.setWhitespaceMode(whitespaceMode);
	parser.parse();
	return listener.events;
}
}

TEST(OsxmlEventParser, simpleCommandWithArgs)
{
	const char *testString = "<a name=\"test\" a=\"1\" b=\"2\" c=\"blub\"/>";
	//                        01234567 89012 3456 78 9012 34 5678 90123 456
	//                        0          1            2            3

	std::vector<std::pair<OsxmlEvent, Variant>> expectedEvents{
	    {OsxmlEvent::COMMAND_START,
	     Variant::arrayType{
	         "a", Variant::mapType{
	                  {"name", "test"}, {"a", 1}, {"b", 2}, {"c", "blub"}}}},
	    {OsxmlEvent::FIELD_END, Variant::arrayType{}}};

	auto events = parseXml(testString);
	ASSERT_EQ(expectedEvents, events);

	// Check the locations (I'll do this one time and then just assume it works)
	ASSERT_EQ(1U, events[0].second.asArray()[0].getLocation().getStart());
	ASSERT_EQ(2U, events[0].second.asArray()[0].getLocation().getEnd());
	ASSERT_EQ(
	    9U,
	    events[0].second.asArray()[1].asMap()["name"].getLocation().getStart());
	ASSERT_EQ(
	    13U,
	    events[0].second.asArray()[1].asMap()["name"].getLocation().getEnd());
	ASSERT_EQ(
	    18U,
	    events[0].second.asArray()[1].asMap()["a"].getLocation().getStart());
	ASSERT_EQ(
	    19U, events[0].second.asArray()[1].asMap()["a"].getLocation().getEnd());
	ASSERT_EQ(
	    24U,
	    events[0].second.asArray()[1].asMap()["b"].getLocation().getStart());
	ASSERT_EQ(
	    25U, events[0].second.asArray()[1].asMap()["b"].getLocation().getEnd());
	ASSERT_EQ(
	    30U,
	    events[0].second.asArray()[1].asMap()["c"].getLocation().getStart());
	ASSERT_EQ(
	    34U, events[0].second.asArray()[1].asMap()["c"].getLocation().getEnd());
}

TEST(OsxmlEventParser, magicTopLevelTag)
{
	const char *testString = "<ousia><a/><b/></ousia>";

	std::vector<std::pair<OsxmlEvent, Variant>> expectedEvents{
	    {OsxmlEvent::COMMAND_START,
	     Variant::arrayType{{"a", Variant::mapType{}}}},
	    {OsxmlEvent::FIELD_END, Variant::arrayType{}},
	    {OsxmlEvent::COMMAND_START,
	     Variant::arrayType{{"b", Variant::mapType{}}}},
	    {OsxmlEvent::FIELD_END, Variant::arrayType{}}};

	auto events = parseXml(testString);
	ASSERT_EQ(expectedEvents, events);
}

TEST(OsxmlEventParser, magicTopLevelTagInside)
{
	const char *testString = "<a><ousia/></a>";

	std::vector<std::pair<OsxmlEvent, Variant>> expectedEvents{
	    {OsxmlEvent::COMMAND_START,
	     Variant::arrayType{{"a", Variant::mapType{}}}},
	    {OsxmlEvent::COMMAND_START,
	     Variant::arrayType{{"ousia", Variant::mapType{}}}},
	    {OsxmlEvent::FIELD_END, Variant::arrayType{}},
	    {OsxmlEvent::FIELD_END, Variant::arrayType{}}};

	auto events = parseXml(testString);
	ASSERT_EQ(expectedEvents, events);
}

TEST(OsxmlEventParser, commandWithDataPreserveWhitespace)
{
	const char *testString = "<a>  hello  \n world </a>";
	//                        012345678901 234567890123
	//                        0         1          2

	std::vector<std::pair<OsxmlEvent, Variant>> expectedEvents{
	    {OsxmlEvent::COMMAND_START,
	     Variant::arrayType{"a", Variant::mapType{}}},
	    {OsxmlEvent::DATA, Variant::arrayType{"  hello  \n world "}},
	    {OsxmlEvent::FIELD_END, Variant::arrayType{}}};

	auto events = parseXml(testString, WhitespaceMode::PRESERVE);
	ASSERT_EQ(expectedEvents, events);

	// Check the location of the text
	ASSERT_EQ(3U, events[1].second.asArray()[0].getLocation().getStart());
	ASSERT_EQ(20U, events[1].second.asArray()[0].getLocation().getEnd());
}

TEST(OsxmlEventParser, commandWithDataTrimWhitespace)
{
	const char *testString = "<a>  hello  \n world </a>";
	//                        012345678901 234567890123
	//                        0         1          2

	std::vector<std::pair<OsxmlEvent, Variant>> expectedEvents{
	    {OsxmlEvent::COMMAND_START,
	     Variant::arrayType{"a", Variant::mapType{}}},
	    {OsxmlEvent::DATA, Variant::arrayType{"hello  \n world"}},
	    {OsxmlEvent::FIELD_END, Variant::arrayType{}}};

	auto events = parseXml(testString, WhitespaceMode::TRIM);
	ASSERT_EQ(expectedEvents, events);

	// Check the location of the text
	ASSERT_EQ(5U, events[1].second.asArray()[0].getLocation().getStart());
	ASSERT_EQ(19U, events[1].second.asArray()[0].getLocation().getEnd());
}

TEST(OsxmlEventParser, commandWithDataCollapseWhitespace)
{
	const char *testString = "<a>  hello  \n world </a>";
	//                        012345678901 234567890123
	//                        0         1          2

	std::vector<std::pair<OsxmlEvent, Variant>> expectedEvents{
	    {OsxmlEvent::COMMAND_START,
	     Variant::arrayType{"a", Variant::mapType{}}},
	    {OsxmlEvent::DATA, Variant::arrayType{"hello world"}},
	    {OsxmlEvent::FIELD_END, Variant::arrayType{}}};

	auto events = parseXml(testString, WhitespaceMode::COLLAPSE);
	ASSERT_EQ(expectedEvents, events);

	// Check the location of the text
	ASSERT_EQ(5U, events[1].second.asArray()[0].getLocation().getStart());
	ASSERT_EQ(19U, events[1].second.asArray()[0].getLocation().getEnd());
}

}

