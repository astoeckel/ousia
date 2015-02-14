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

#include <core/common/Rtti.hpp>
#include <core/parser/stack/State.hpp>

namespace ousia {
namespace parser_stack {

static const Rtti t1;
static const Rtti t2;
static const Rtti t3;
static const Rtti t4;
static const Rtti t5;

static const State s1 = StateBuilder().createdNodeType(&t1);
static const State s2a =
    StateBuilder().parent(&s1).createdNodeType(&t2);
static const State s2b =
    StateBuilder().parent(&s1).createdNodeType(&t2);
static const State s3 =
    StateBuilder().parents({&s2a, &s1}).createdNodeType(&t3);
static const State s4 =
    StateBuilder().parent(&s3).createdNodeType(&t4);
static const State s5 =
    StateBuilder().parent(&s2b).createdNodeType(&t5);

TEST(StateDeductor, deduce)
{
	using Result = std::vector<const State *>;
	using Signature = std::vector<const Rtti *>;
	std::vector<const State *> states{&s1, &s2a, &s2b, &s3, &s4, &s5};

	// Should not crash on empty signature
	ASSERT_EQ(Result{}, StateDeductor(Signature{}, states).deduce());

	// Try repeating signature elements
	ASSERT_EQ(Result({&s1}),
	          StateDeductor(Signature({&t1}), states).deduce());
	ASSERT_EQ(Result({&s1}),
	          StateDeductor(Signature({&t1, &t1}), states).deduce());
	ASSERT_EQ(Result({&s1}),
	          StateDeductor(Signature({&t1, &t1, &t1}), states).deduce());

	// Go to another state
	ASSERT_EQ(Result({&s2a, &s2b}),
	          StateDeductor(Signature({&t1, &t1, &t2}), states).deduce());
	ASSERT_EQ(Result({&s4}),
	          StateDeductor(Signature({&t1, &t3, &t4}), states).deduce());

	// Skip one state
	ASSERT_EQ(Result({&s4}),
	          StateDeductor(Signature({&t2, &t4}), states).deduce());

	// Impossible signature
	ASSERT_EQ(Result({}),
	          StateDeductor(Signature({&t4, &t5}), states).deduce());

}
}
}

