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

#include <core/variant/Variant.hpp>

namespace ousia {

TEST(Variant, nullValue)
{
	Variant v;
	ASSERT_TRUE(v.isNull());

	v = 1;
	ASSERT_FALSE(v.isNull());

	v = nullptr;
	ASSERT_TRUE(v.isNull());

	Variant v2{nullptr};
	ASSERT_TRUE(v.isNull());
}

TEST(Variant, booleanValue)
{
	Variant v{true};
	ASSERT_TRUE(v.isBool());
	ASSERT_TRUE(v.asBool());

	v = false;
	ASSERT_TRUE(v.isBool());
	ASSERT_FALSE(v.asBool());

	v.setBool(true);
	ASSERT_TRUE(v.isBool());
	ASSERT_TRUE(v.asBool());

	v = nullptr;
	ASSERT_FALSE(v.isBool());
}

TEST(Variant, intValue)
{
	Variant v{42};
	ASSERT_TRUE(v.isInt());
	ASSERT_EQ(42, v.asInt());

	v = 43;
	ASSERT_TRUE(v.isInt());
	ASSERT_EQ(43, v.asInt());

	v = false;
	ASSERT_FALSE(v.isInt());
}

TEST(Variant, doubleValue)
{
	Variant v{42.5};
	ASSERT_TRUE(v.isDouble());
	ASSERT_EQ(42.5, v.asDouble());

	v = 42;
	ASSERT_FALSE(v.isDouble());

	v = 43.5;
	ASSERT_TRUE(v.isDouble());
	ASSERT_EQ(43.5, v.asDouble());
}

TEST(Variant, stringValue)
{
	Variant v{"Hello World"};
	ASSERT_TRUE(v.isString());
	ASSERT_EQ("Hello World", v.asString());

	v = "Goodbye World";
	ASSERT_TRUE(v.isString());
	ASSERT_EQ("Goodbye World", v.asString());

	v = 42;
	ASSERT_FALSE(v.isString());
}

TEST(Variant, arrayValue)
{
	const Variant v{{"test1", 42}};
	ASSERT_EQ(2, v.asArray().size());
	ASSERT_EQ("test1", v.asArray()[0].asString());
	ASSERT_EQ(42, v.asArray()[1].asInt());
}

TEST(Variant, mapValue)
{
	const Variant v{{{"key1", "entry1"}, {"key2", "entry2"}}};

	auto map = v.asMap();
	ASSERT_EQ(2, map.size());

	ASSERT_EQ("entry1", map.find("key1")->second.asString());
	ASSERT_EQ("entry2", map.find("key2")->second.asString());

	const Variant v2{{{"key1", Variant::arrayType{1, 2}}, {"key2", "entry2"}}};
	ASSERT_EQ(2, v2.asMap().find("key1")->second.asArray()[1].asInt());
}


}

