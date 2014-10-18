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

#include <core/script/Variant.hpp>

namespace ousia {
namespace script {

TEST(Variant, getBooleanValue)
{
	ASSERT_TRUE((Variant{true}).getBooleanValue());
	ASSERT_FALSE((Variant{false}).getBooleanValue());
	ASSERT_FALSE((Variant{(int64_t)0}).getBooleanValue());
	ASSERT_TRUE((Variant{(int64_t)1}).getBooleanValue());
	ASSERT_FALSE((Variant{0.0}).getBooleanValue());
	ASSERT_TRUE((Variant{1.2}).getBooleanValue());
	ASSERT_FALSE((Variant{""}).getBooleanValue());
}

TEST(Variant, getIntegerValue)
{
	Variant vi{(int64_t)42};
	Variant vf{42.0};

	ASSERT_EQ(42, vi.getIntegerValue());
	ASSERT_EQ(42, vf.getIntegerValue());
	ASSERT_EQ(1, (Variant{true}).getIntegerValue());
	ASSERT_EQ(0, (Variant{false}).getIntegerValue());
}

TEST(Variant, getNumberValue)
{
	Variant vi{(int64_t)42};
	Variant vf{42.5};

	ASSERT_EQ(42, vi.getNumberValue());
	ASSERT_EQ(42.5, vf.getNumberValue());
	ASSERT_EQ(1.0, (Variant{true}).getIntegerValue());
	ASSERT_EQ(0.0, (Variant{false}).getIntegerValue());
}

TEST(Variant, getStringValue)
{
	Variant v{"hello world"};
	ASSERT_EQ("hello world", v.getStringValue());
}

TEST(Variant, getArrayValue)
{
	Variant v{{"test1", (int64_t)42}};
	ASSERT_EQ(2, v.getArrayValue().size());
	ASSERT_EQ("test1", v.getArrayValue()[0].getStringValue());
	ASSERT_EQ(42, v.getArrayValue()[1].getIntegerValue());
}

TEST(Variant, getMapValue)
{
	Variant v{{{"key1", "entry1"}, {"key2", "entry2"}}};

	auto map = v.getMapValue();
	ASSERT_EQ(2, map.size());

	ASSERT_EQ("entry1", (*map.find("key1")).second.getStringValue());
	ASSERT_EQ("entry2", (*map.find("key2")).second.getStringValue());
}

}
}

