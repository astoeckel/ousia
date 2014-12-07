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

#include <core/variant/Reader.hpp>

namespace ousia {
namespace variant {

static TerminalLogger logger{std::cerr, true};

TEST(Reader, readString)
{
	// Simple, double quoted string
	{
		BufferedCharReader reader("\"hello world\"");
		auto res = Reader::parseString(reader, logger);
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Simple, double quoted string with whitespace
	{
		BufferedCharReader reader("    \"hello world\"   ");
		auto res = Reader::parseString(reader, logger);
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Simple, single quoted string
	{
		BufferedCharReader reader("'hello world'");
		auto res = Reader::parseString(reader, logger);
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Escape characters
	{
		BufferedCharReader reader("'\\'\\\"\\b\\f\\n\\r\\t\\v'");
		auto res = Reader::parseString(reader, logger);
		ASSERT_TRUE(res.first);
		ASSERT_EQ("'\"\b\f\n\r\t\v", res.second);
	}
}

TEST(Reader, parseUnescapedString)
{
	// Simple case
	{
		BufferedCharReader reader("hello world;");
		auto res = Reader::parseUnescapedString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Simple case with whitespace
	{
		BufferedCharReader reader("    hello world   ;    ");
		auto res = Reader::parseUnescapedString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}

	// Linebreaks
	{
		BufferedCharReader reader("    hello\nworld   ;    ");
		auto res = Reader::parseUnescapedString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello\nworld", res.second);
	}

	// End of stream
	{
		BufferedCharReader reader("    hello world ");
		auto res = Reader::parseUnescapedString(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ("hello world", res.second);
	}
}

TEST(Reader, parseInteger)
{
	// Valid integers
	{
		BufferedCharReader reader("0  ");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(0, res.second);
	}

	{
		BufferedCharReader reader("42 ");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(42, res.second);
	}

	{
		BufferedCharReader reader("-42");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-42, res.second);
	}

	{
		BufferedCharReader reader("  -0x4A2  ");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-0x4A2, res.second);
	}

	{
		BufferedCharReader reader(" 0Xaffe");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(0xAFFE, res.second);
	}

	{
		BufferedCharReader reader("0x7FFFFFFFFFFFFFFF");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(0x7FFFFFFFFFFFFFFFL, res.second);
	}

	{
		BufferedCharReader reader("-0x7FFFFFFFFFFFFFFF");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-0x7FFFFFFFFFFFFFFFL, res.second);
	}

	// Invalid integers
	{
		BufferedCharReader reader("-");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_FALSE(res.first);
	}

	{
		BufferedCharReader reader("0a");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_FALSE(res.first);
	}

	{
		BufferedCharReader reader("-0xag");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_FALSE(res.first);
	}

	{
		BufferedCharReader reader("0x8000000000000000");
		auto res = Reader::parseInteger(reader, logger, {});
		ASSERT_FALSE(res.first);
	}
}

TEST(Reader, parseDouble)
{
	// Valid doubles
	{
		BufferedCharReader reader("1.25");
		auto res = Reader::parseDouble(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(1.25, res.second);
	}

	{
		BufferedCharReader reader(".25");
		auto res = Reader::parseDouble(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(.25, res.second);
	}

	{
		BufferedCharReader reader(".25e1");
		auto res = Reader::parseDouble(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(2.5, res.second);
	}

	{
		BufferedCharReader reader("-2.5e-1");
		auto res = Reader::parseDouble(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-0.25, res.second);
	}

	{
		BufferedCharReader reader("-50e-2");
		auto res = Reader::parseDouble(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-0.5, res.second);
	}

	{
		BufferedCharReader reader("-1.");
		auto res = Reader::parseDouble(reader, logger, {});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-1., res.second);
	}

	{
		BufferedCharReader reader("-50.e-2");
		auto res = Reader::parseDouble(reader, logger, {'.'});
		ASSERT_TRUE(res.first);
		ASSERT_EQ(-50, res.second);
	}

	// Invalid doubles
	{
		BufferedCharReader reader(".e1");
		auto res = Reader::parseDouble(reader, logger, {});
		ASSERT_FALSE(res.first);
	}

	{
		BufferedCharReader reader("0e100000");
		auto res = Reader::parseDouble(reader, logger, {});
		ASSERT_FALSE(res.first);
	}
}

TEST(Reader, parseArray)
{
	// Simple case (only primitive data types)
	{
		BufferedCharReader reader("[\"Hello, World\", unescaped\n string ,\n"
			"1234, 0.56, true, false, null]");
		auto res = Reader::parseArray(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure array has the correct size
		ASSERT_EQ(7, res.second.size());

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
		BufferedCharReader reader("[  'test' ,]");
		auto res = Reader::parseArray(reader, logger);
		ASSERT_TRUE(res.first);

		// Make sure the array has the correct size
		ASSERT_EQ(1, res.second.size());

		// Check the types
		ASSERT_TRUE(res.second[0].isString());

		// Check the values
		ASSERT_EQ("test", res.second[0].asString());
	}

	// Recovery from invalid values
	// TODO: Actually parseGeneric should fall back to returning a simple string
	// if parsing of a special (non-string) type failed
	{
		BufferedCharReader reader("[ 0invalidNumber, str, 1invalid]");
		auto res = Reader::parseArray(reader, logger);
		ASSERT_FALSE(res.first);

		// Make sure the array has the correct size
		ASSERT_EQ(3, res.second.size());

		// Check the types (only for the valid entries, the other types are
		// undefined)
		ASSERT_TRUE(res.second[1].isString());

		// Check the values
		ASSERT_EQ("str", res.second[1].asString());
	}
}

TEST(Reader, parseGeneric)
{
	// Simple case, unescaped string
	{
		BufferedCharReader reader("hello world");
		auto res = Reader::parseGeneric(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_TRUE(res.second.isString());
		ASSERT_EQ("hello world", res.second.asString());
	}

	// Simple case, double quoted string
	{
		BufferedCharReader reader(" \"hello world\"    ");
		auto res = Reader::parseGeneric(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_TRUE(res.second.isString());
		ASSERT_EQ("hello world", res.second.asString());
	}

	// Simple case, single quoted string
	{
		BufferedCharReader reader(" 'hello world'    ");
		auto res = Reader::parseGeneric(reader, logger, {';'});
		ASSERT_TRUE(res.first);
		ASSERT_TRUE(res.second.isString());
		ASSERT_EQ("hello world", res.second.asString());
	}
}

}
}

