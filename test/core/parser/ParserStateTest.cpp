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
#include <core/parser/ParserState.hpp>

namespace ousia {

static const Rtti t1;
static const Rtti t2;
static const Rtti t3;
static const Rtti t4;
static const Rtti t5;

static const ParserState s1 = ParserStateBuilder().createdNodeType(&t1);
static const ParserState s2a =
    ParserStateBuilder().parent(&s1).createdNodeType(&t2);
static const ParserState s2b =
    ParserStateBuilder().parent(&s1).createdNodeType(&t2);
static const ParserState s3 =
    ParserStateBuilder().parents({&s2a, &s1}).createdNodeType(&t3);
static const ParserState s4 =
    ParserStateBuilder().parent(&s3).createdNodeType(&t4);
static const ParserState s5 =
    ParserStateBuilder().parent(&s2b).createdNodeType(&t5);

TEST(ParserStateDeductor, deduce)
{
	using Result = std::vector<const ParserState *>;
	using Signature = std::vector<const Rtti *>;
	std::vector<const ParserState *> states{&s1, &s2a, &s2b, &s3, &s4, &s5};

	// Should not crash on empty signature
	ASSERT_EQ(Result{}, ParserStateDeductor(Signature{}, states).deduce());

	// Try repeating signature elements
	ASSERT_EQ(Result({&s1}),
	          ParserStateDeductor(Signature({&t1}), states).deduce());
	ASSERT_EQ(Result({&s1}),
	          ParserStateDeductor(Signature({&t1, &t1}), states).deduce());
	ASSERT_EQ(Result({&s1}),
	          ParserStateDeductor(Signature({&t1, &t1, &t1}), states).deduce());

	// Go to another state
	ASSERT_EQ(Result({&s2a, &s2b}),
	          ParserStateDeductor(Signature({&t1, &t1, &t2}), states).deduce());
	ASSERT_EQ(Result({&s4}),
	          ParserStateDeductor(Signature({&t1, &t3, &t4}), states).deduce());

	// Skip one state
	ASSERT_EQ(Result({&s4}),
	          ParserStateDeductor(Signature({&t2, &t4}), states).deduce());

	// Impossible signature
	ASSERT_EQ(Result({}),
	          ParserStateDeductor(Signature({&t4, &t5}), states).deduce());

}
}

