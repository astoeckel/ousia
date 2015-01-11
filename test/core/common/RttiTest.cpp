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

#include <array>
#include <string>
#include <iostream>

#include <gtest/gtest.h>

#include <core/common/Rtti.hpp>

namespace ousia {
namespace {

class RttiTestClass1 {
};
class RttiTestClass2 {
};
class RttiTestClass3 {
};
class RttiTestClass4 {
};
class RttiTestClass5 {
};
class RttiTestClass6 {
};
class RttiTestClass7 {
};

extern const Rtti<RttiTestClass6> Type6;
extern const Rtti<RttiTestClass7> Type7;

const Rtti<RttiTestClass1> Type1("Type1");
const Rtti<RttiTestClass2> Type2("Type2");
const Rtti<RttiTestClass3> Type3("Type3", {&Type1});
const Rtti<RttiTestClass4> Type4("Type4", {&Type3, &Type2});
const Rtti<RttiTestClass5> Type5("Type5",
                                 std::unordered_set<const RttiType *>{},
                                 {&Type6, &Type7});
const Rtti<RttiTestClass6> Type6("Type6",
                                 std::unordered_set<const RttiType *>{},
                                 {&Type1});
const Rtti<RttiTestClass7> Type7("Type7", {&Type6},
                                 std::unordered_set<const RttiType *>{});

TEST(Rtti, isa)
{
	ASSERT_TRUE(Type1.isa(Type1));
	ASSERT_FALSE(Type1.isa(Type2));
	ASSERT_FALSE(Type1.isa(Type3));
	ASSERT_FALSE(Type1.isa(Type4));

	ASSERT_FALSE(Type2.isa(Type1));
	ASSERT_TRUE(Type2.isa(Type2));
	ASSERT_FALSE(Type2.isa(Type3));
	ASSERT_FALSE(Type2.isa(Type4));

	ASSERT_TRUE(Type3.isa(Type1));
	ASSERT_FALSE(Type3.isa(Type2));
	ASSERT_TRUE(Type3.isa(Type3));
	ASSERT_FALSE(Type3.isa(Type4));

	ASSERT_TRUE(Type4.isa(Type1));
	ASSERT_TRUE(Type4.isa(Type2));
	ASSERT_TRUE(Type4.isa(Type3));
	ASSERT_TRUE(Type4.isa(Type4));
}

TEST(Rtti, composedOf)
{
	std::vector<const RttiType *> types{&Type1, &Type2, &Type3, &Type4};
	for (auto t : types) {
		ASSERT_FALSE(t->composedOf(Type1));
		ASSERT_FALSE(t->composedOf(Type2));
		ASSERT_FALSE(t->composedOf(Type3));
		ASSERT_FALSE(t->composedOf(Type4));
		ASSERT_FALSE(t->composedOf(Type5));
		ASSERT_FALSE(t->composedOf(Type6));
		ASSERT_FALSE(t->composedOf(Type7));
	}

	ASSERT_TRUE(Type5.composedOf(Type1));
	ASSERT_FALSE(Type5.composedOf(Type2));
	ASSERT_FALSE(Type5.composedOf(Type3));
	ASSERT_FALSE(Type5.composedOf(Type4));
	ASSERT_FALSE(Type5.composedOf(Type5));
	ASSERT_TRUE(Type5.composedOf(Type6));
	ASSERT_TRUE(Type5.composedOf(Type7));

	ASSERT_TRUE(Type6.composedOf(Type1));
	ASSERT_FALSE(Type6.composedOf(Type2));
	ASSERT_FALSE(Type6.composedOf(Type3));
	ASSERT_FALSE(Type6.composedOf(Type4));
	ASSERT_FALSE(Type6.composedOf(Type5));
	ASSERT_FALSE(Type6.composedOf(Type6));
	ASSERT_FALSE(Type6.composedOf(Type7));

	ASSERT_TRUE(Type7.composedOf(Type1));
	ASSERT_FALSE(Type7.composedOf(Type2));
	ASSERT_FALSE(Type7.composedOf(Type3));
	ASSERT_FALSE(Type7.composedOf(Type4));
	ASSERT_FALSE(Type7.composedOf(Type5));
	ASSERT_FALSE(Type7.composedOf(Type6));
	ASSERT_FALSE(Type7.composedOf(Type7));
}
}
}

