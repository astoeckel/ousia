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

#include <core/script/Function.hpp>

namespace ousia {
namespace script {

TEST(HostFunction, callDirect)
{
	int v = 0;
	HostFunction f{[](const std::vector<Variant> &args, void *data) {
		*(static_cast<int*>(data)) = args[0].getIntegerValue();
		return Variant::Null;
	}, {Argument{VariantType::integer}}, &v};
	ASSERT_EQ(VariantType::null, f.call({{(int64_t)42}}).getType());
	ASSERT_EQ(42, v);
}

TEST(HostFunction, callDefaults)
{
	int v = 0;
	HostFunction f{[](const std::vector<Variant> &args, void *data) {
		*(static_cast<int*>(data)) = args[0].getIntegerValue();
		return Variant{"Hallo Welt"};
	}, {Argument{VariantType::integer, {(int64_t)42}}}, &v};
	ASSERT_EQ("Hallo Welt", f.call().getStringValue());
	ASSERT_EQ(42, v);
}

TEST(Setter, call)
{
	int v = 0;
	Setter f{VariantType::integer, [](Variant arg, void *data) {
		*(static_cast<int*>(data)) = arg.getIntegerValue();
	}, &v};
	f.call({(int64_t)42});
	ASSERT_EQ(42, v);
}

TEST(Getter, call)
{
	int v = 42;
	Getter f{[](void *data) {
		return Variant{int64_t(*(static_cast<int*>(data)))};
	}, &v};
	ASSERT_EQ(v, f.call().getIntegerValue());
}

}
}

