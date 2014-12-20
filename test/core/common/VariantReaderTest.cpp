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

#include <iostream>
#include <gtest/gtest.h>

#include <core/common/VariantReader.hpp>

namespace ousia {
namespace variant {

static TerminalLogger logger{std::cerr, true};
//static Logger logger;

TEST(VariantReader, readString)
{
	// Simple, double quoted string
	{
		CharReader reader("\"hello world\"");
		auto res = VariantReader::parseString(reader, logger);
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Simple, double quoted string with whitespace
	{
		CharReader reader("    \"hello world\"   ");
		auto res = VariantReader::parseString(reader, logger);
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Simple, single quoted string
	{
		CharReader reader("'hello world'");
		auto res = VariantReader::parseString(reader, logger);
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Escape characters
	{
		CharReader reader("'\\'\\\"\\b\\f\\n\\r\\t\\v'");
		auto res = VariantReader::parseString(reader, logger);
		ASSERT_TRUE(res.first);
		ASSERT_EQ("'\"\b\f\n\r\t\v", res.second);
	}


	// Hex Unicode character
	{
		CharReader reader("'linebreak\\u000A in unicode'");
		auto res = VariantReader::parseString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("linebreak\n in unicode", res.second);
	}
}

TEST(VariantReader, readStringUnicode)
{
	// Hex Unicode character
	{
		CharReader reader("'linebreak \\u000A in unicode'");
		auto res = VariantReader::parseString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("linebreak \n in unicode", res.second);
	}

	// Hex Unicode character
	{
		CharReader reader("'hammer and sickle \\u262D in unicode'");
		auto res = VariantReader::parseString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hammer and sickle \342\230\255 in unicode", res.second);
	}

	// Octal Latin-1 character
	{
		CharReader reader("'copyright symbol \\251 in Unicode'");
		auto res = VariantReader::parseString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("copyright symbol \302\251 in Unicode", res.second);
	}

	// Hexadecimal Latin-1 character
	{
		CharReader reader("'copyright symbol \\xA9 in Unicode'");
		auto res = VariantReader::parseString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("copyright symbol \302\251 in Unicode", res.second);
	}

	// Errornous unicode escape sequence
	{
		CharReader reader("'\\uBLUB'");
		auto res = VariantReader::parseString(reader, logger, {';'});
		ASSERT_FALSE(res.first);
	}

	// Errornous octal escape sequence
	{
		CharReader reader("'\\400'");
		auto res = VariantReader::parseString(reader, logger, {';'});
		ASSERT_FALSE(res.first);
	}

	// Errornous hexadecimal latin1 escape sequence
	{
		CharReader reader("'\\xa'");
		auto res = VariantReader::parseString(reader, logger, {';'});
		ASSERT_FALSE(res.first);
	}
}

TEST(VariantReader, parseUnescapedString)
{
	// Simple case
	{
		CharReader reader("hello world;");
		auto res = VariantReader::parseUnescapedString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Simple case with whitespace
	{
		CharReader reader("    hello world   ;    ");
		auto res = VariantReader::parseUnescapedString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Linebreaks
	{
		CharReader reader("    hello\nworld   ;    ");
		auto res = VariantReader::parseUnescapedString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello\nworld", res.second);
	}

	// End of stream
	{
		CharReader reader("    hello world ");
		auto res = VariantReader::parseUnescapedString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}
}

static const std::unordered_set<char> noDelim;

TEST(VariantReader, parseInteger)
{
	// Valid integers
	{
		CharReader reader("0  ");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(0, res.second);
	}

	{
		CharReader reader("42 ");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(42, res.second);
	}

	{
		CharReader reader("-42");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-42, res.second);
	}

	{
		CharReader reader("  -0x4A2  ");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-0x4A2, res.second);
	}

	{
		CharReader reader(" 0Xaffe");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(0xAFFE, res.second);
	}

	{
		CharReader reader("0x7FFFFFFFFFFFFFFF");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(0x7FFFFFFFFFFFFFFFL, res.second);
	}

	{
		CharReader reader("-0x7FFFFFFFFFFFFFFF");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-0x7FFFFFFFFFFFFFFFL, res.second);
	}

	// Invalid integers
	{
		CharReader reader("-");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_FALSE(res.first);
	}

	{
		CharReader reader("0a");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_FALSE(res.first);
	}

	{
		CharReader reader("-0xag");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_FALSE(res.first);
	}

	{
		CharReader reader("0x8000000000000000");
		auto res = VariantReader::parseInteger(reader, logger, noDelim);
		ASSERT_FALSE(res.first);
	}
}

TEST(VariantReader, parseDouble)
{
	// Valid doubles
	{
		CharReader reader("1.25");
		auto res = VariantReader::parseDouble(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(1.25, res.second);
	}

	{
		CharReader reader(".25");
		auto res = VariantReader::parseDouble(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(.25, res.second);
	}

	{
		CharReader reader(".25e1");
		auto res = VariantReader::parseDouble(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(2.5, res.second);
	}

	{
		CharReader reader("-2.5e-1");
		auto res = VariantReader::parseDouble(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-0.25, res.second);
	}

	{
		CharReader reader("-50e-2");
		auto res = VariantReader::parseDouble(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-0.5, res.second);
	}

	{
		CharReader reader("-1.");
		auto res = VariantReader::parseDouble(reader, logger, noDelim);
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-1., res.second);
	}

	{
		CharReader reader("-50.e-2");
		auto res = VariantReader::parseDouble(reader, logger, {'.'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-50, res.second);
	}

	// Invalid doubles
	{
		CharReader reader(".e1");
		auto res = VariantReader::parseDouble(reader, logger, noDelim);
		ASSERT_FALSE(res.first);
	}

	{
		CharReader reader("0e100000");
		auto res = VariantReader::parseDouble(reader, logger, noDelim);
		ASSERT_FALSE(res.first);
	}
}

TEST(VariantReader, parseArray)
{
	// Simple case (only primitive data types)
	{
		CharReader reader("[\"Hello, World\", unescaped\n string ,\n"
			"1234, 0.56, true, false, null]");
		auto res = VariantReader::parseArray(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure array has the correct size
		ASSERT_EQ(7U, res.second.size());

		// Check the types
		ASSERT_TRUE(res.second[0].isString());
		ASSERT_TRUE(res.second[1].isString());
		ASSERT_TRUE(res.second[2].isInt());
		ASSERT_TRUE(res.second[3].isDouble());
		ASSERT_TRUE(res.second[4].isBool());
		ASSERT_TRUE(res.second[5].isBool());
		ASSERT_TRUE(res.second[6].isNull());

		// Check the values
		ASSERT_EQ("Hello, World", res.second[0].asString());
		ASSERT_EQ("unescaped\n string", res.second[1].asString());
		ASSERT_EQ(1234, res.second[2].asInt());
		ASSERT_EQ(0.56, res.second[3].asDouble());
		ASSERT_TRUE(res.second[4].asBool());
		ASSERT_FALSE(res.second[5].asBool());
	}

	// Ending with comma
	{
		CharReader reader("[  'test' ,]");
		auto res = VariantReader::parseArray(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure the array has the correct size
		ASSERT_EQ(1U, res.second.size());

		// Check the types
		ASSERT_TRUE(res.second[0].isString());

		// Check the values
		ASSERT_EQ("test", res.second[0].asString());
	}

	// Recovery from invalid values
	{
		CharReader reader("[ 0invalidNumber, str, 1invalid]");
		auto res = VariantReader::parseArray(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure the array has the correct size
		ASSERT_EQ(3U, res.second.size());

		// Check the types (all must be strings since the numbers are invalid)
		ASSERT_TRUE(res.second[0].isString());
		ASSERT_TRUE(res.second[1].isString());
		ASSERT_TRUE(res.second[2].isString());

		// Check the values
		ASSERT_EQ("0invalidNumber", res.second[0].asString());
		ASSERT_EQ("str", res.second[1].asString());
		ASSERT_EQ("1invalid", res.second[2].asString());
	}
}

TEST(VariantReader, parseObject)
{
	// Array as object
	{
		CharReader reader("[\"Hello, World\", unescaped\n string ,\n"
			"1234, 0.56, true, false, null]");
		auto res = VariantReader::parseObject(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure the object has the correct size
		ASSERT_EQ(7U, res.second.size());

		// Check the types
		ASSERT_TRUE(res.second["#0"].isString());
		ASSERT_TRUE(res.second["#1"].isString());
		ASSERT_TRUE(res.second["#2"].isInt());
		ASSERT_TRUE(res.second["#3"].isDouble());
		ASSERT_TRUE(res.second["#4"].isBool());
		ASSERT_TRUE(res.second["#5"].isBool());
		ASSERT_TRUE(res.second["#6"].isNull());

		// Check the values
		ASSERT_EQ("Hello, World", res.second["#0"].asString());
		ASSERT_EQ("unescaped\n string", res.second["#1"].asString());
		ASSERT_EQ(1234, res.second["#2"].asInt());
		ASSERT_EQ(0.56, res.second["#3"].asDouble());
		ASSERT_TRUE(res.second["#4"].asBool());
		ASSERT_FALSE(res.second["#5"].asBool());
	}

	// Simple object
	{
		CharReader reader("[key1=foo, key2=bar]");
		auto res = VariantReader::parseObject(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure the object has the correct size
		ASSERT_EQ(2U, res.second.size());

		// Check the types
		ASSERT_TRUE(res.second["key1"].isString());
		ASSERT_TRUE(res.second["key2"].isString());

		// Check the values
		ASSERT_EQ("foo", res.second["key1"].asString());
		ASSERT_EQ("bar", res.second["key2"].asString());
	}

	// Simple array/object interleaved
	{
		CharReader reader("[foo1, key1=foo, foo2, key2=bar, foo3]");
		auto res = VariantReader::parseObject(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure the object has the correct size
		ASSERT_EQ(5U, res.second.size());

		// Check the types
		ASSERT_TRUE(res.second["key1"].isString());
		ASSERT_TRUE(res.second["key2"].isString());
		ASSERT_TRUE(res.second["#0"].isString());
		ASSERT_TRUE(res.second["#2"].isString());
		ASSERT_TRUE(res.second["#4"].isString());

		// Check the values
		ASSERT_EQ("foo", res.second["key1"].asString());
		ASSERT_EQ("bar", res.second["key2"].asString());
		ASSERT_EQ("foo1", res.second["#0"].asString());
		ASSERT_EQ("foo2", res.second["#2"].asString());
		ASSERT_EQ("foo3", res.second["#4"].asString());
	}

	// More complex array/object
	{
		CharReader reader("[key1=true, foo, key2=3.5]");
		auto res = VariantReader::parseObject(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure the object has the correct size
		ASSERT_EQ(3U, res.second.size());

		// Check the types
		ASSERT_TRUE(res.second["key1"].isBool());
		ASSERT_TRUE(res.second["key2"].isDouble());
		ASSERT_TRUE(res.second["#1"].isString());

		// Check the values
		ASSERT_TRUE(res.second["key1"].asBool());
		ASSERT_EQ(3.5, res.second["key2"].asDouble());
		ASSERT_EQ("foo", res.second["#1"].asString());
	}

	// Even More complex array/object
	{
		CharReader reader("[\"key1\" = [4, 5, true, e=[1, 2, 3]], \"key2\"=[]]");
		auto res = VariantReader::parseObject(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure the object has the correct size
		ASSERT_EQ(2U, res.second.size());

		// Check the types
		ASSERT_TRUE(res.second["key1"].isMap());
		ASSERT_TRUE(res.second["key2"].isArray());

		// Check the values
		std::cout << res.second << std::endl;
	}

	// Invalid array/object
	{
		CharReader reader("[invalid key = bla]");
		auto res = VariantReader::parseObject(reader, logger);
		ASSERT_FALSE(res.first);
	}
}

TEST(VariantReader, parseGeneric)
{
	// Simple case, unescaped string
	{
		CharReader reader("hello world");
		auto res = VariantReader::parseGeneric(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_TRUE(res.second.isString());
		ASSERT_EQ("hello world", res.second.asString());
	}

	// Simple case, double quoted string
	{
		CharReader reader(" \"hello world\"    ");
		auto res = VariantReader::parseGeneric(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_TRUE(res.second.isString());
		ASSERT_EQ("hello world", res.second.asString());
	}

	// Simple case, single quoted string
	{
		CharReader reader(" 'hello world'    ");
		auto res = VariantReader::parseGeneric(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_TRUE(res.second.isString());
		ASSERT_EQ("hello world", res.second.asString());
	}
}

}
}

