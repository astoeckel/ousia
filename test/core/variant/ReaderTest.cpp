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

TEST(Reader, readString)
{
	TerminalLogger logger(std::cerr, true);

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
	TerminalLogger logger(std::cerr, true);

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

TEST(Reader, parseGeneric)
{
	TerminalLogger logger(std::cerr, true);

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

