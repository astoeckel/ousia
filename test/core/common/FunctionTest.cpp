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

#include <core/common/Function.hpp>

namespace ousia {

class MethodTestClass {
public:
	bool visited = false;

	void visit() { visited = true; }
};

TEST(Method, simple)
{
	Method<MethodTestClass> m{
	    [](Variant::arrayType &args, MethodTestClass *thisRef) {
		    thisRef->visit();
		    return Variant{};
		}};

	MethodTestClass inst;
	m.call({}, &inst);
}

TEST(Method, validation)
{
	Method<void> m{{Argument::Int("a"), Argument::Int("b")},
	               [](Variant::arrayType &args, void *thisRef) {
		return Variant{args[0].asInt() + args[1].asInt()};
	}};

	MethodTestClass inst;
	ASSERT_EQ(3, m.call({1, 2}, &inst).asInt());
	ASSERT_THROW(m.call({1}, &inst), LoggableException);
	ASSERT_THROW(m.call({1, "bla"}, &inst), LoggableException);
}
}

