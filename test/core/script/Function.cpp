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
	// Local variable
	int v = 0;

	// Host function which sets the local variable
	auto f = createHostFunction(
		[&v](const std::vector<Variant> &args, void *data) {
			v = args[0].getIntegerValue();
			return VarNull;
		}, {ArgumentDescriptor{VariantType::integer}});

	// Call the host function
	ASSERT_EQ(VariantType::null, f.call({{(int64_t)42}}).getType());
	ASSERT_EQ(42, v);
}

TEST(HostFunction, callDefaults)
{
	// Local variable
	int v = 0;

	// Host function which sets the local variable
	auto f = createHostFunction(
		[&v](const std::vector<Variant> &args, void *data) {
			v = args[0].getIntegerValue();
			return Variant{"Hallo Welt"};
		}, {ArgumentDescriptor{VariantType::integer, {(int64_t)42}}});

	// Call the host function
	ASSERT_EQ("Hallo Welt", f.call().getStringValue());
	ASSERT_EQ(42, v);
}

}
}

