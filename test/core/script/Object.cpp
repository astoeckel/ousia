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

#include <core/script/Object.hpp>

namespace ousia {
namespace script {

TEST(Object, addProperty)
{
	int64_t i = 0;

	Object o{&i};

	auto get = [](void *data) {
		return Variant{*((int64_t*)data)};
	};
	auto set = [](Variant v, void *data) {
		*((int64_t*)data) = v.getIntegerValue();
	};

	std::map<std::string, Property> ps;

	o.addProperty("p1", VariantType::integer, get, set);
	o.addReadonlyProperty("p2", get);

	ASSERT_TRUE(o.getProperty("p1") != nullptr);
	ASSERT_TRUE(o.getProperty("p2") != nullptr);
	ASSERT_FALSE(o.getMethod("p1") != nullptr);
	ASSERT_FALSE(o.getMethod("p2") != nullptr);

	o.getProperty("p1")->set({(int64_t)42});
	ASSERT_EQ(42, i);
	ASSERT_EQ(i, o.getProperty("p1")->get().getIntegerValue());

	ASSERT_FALSE(o.getProperty("p2")->set.exists());
	ASSERT_EQ(i, o.getProperty("p2")->get().getIntegerValue());
}
}
}

