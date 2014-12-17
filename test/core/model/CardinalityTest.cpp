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

#include <core/model/Cardinality.hpp>

namespace ousia {
namespace model {
TEST(Cardinality, testCardinalities)
{
	// Start with the elementary Cardinalities.
	{
		SingleCardinality c{1};
		for (size_t s = 0; s < 100; s++) {
			if (s != 1) {
				ASSERT_FALSE(c.permits(s));
			} else {
				ASSERT_TRUE(c.permits(s));
			}
		}
	}

	{
		OpenRangeCardinality c{4};
		for (size_t s = 0; s < 100; s++) {
			if (s < 4) {
				ASSERT_FALSE(c.permits(s));
			} else {
				ASSERT_TRUE(c.permits(s));
			}
		}
	}

	{
		RangeCardinality c{1, 10};
		for (size_t s = 0; s < 100; s++) {
			if (s < 1 || s > 10) {
				ASSERT_FALSE(c.permits(s));
			} else {
				ASSERT_TRUE(c.permits(s));
			}
		}
	}

	// Then construct more complex ones as unions.

	{
		UnionCardinality c =
		    unite(SingleCardinality(1),
		          unite(RangeCardinality(4, 6), OpenRangeCardinality(16)));
		for (size_t s = 0; s < 100; s++) {
			if (s < 1 || (s > 1 && s < 4) || (s > 6 && s < 16)) {
				ASSERT_FALSE(c.permits(s));
			} else {
				ASSERT_TRUE(c.permits(s));
			}
		}
	}
}

TEST(Cardinality, testEquals)
{
	{
		SingleCardinality a{1};
		SingleCardinality b{2};
		OpenRangeCardinality c{1};

		ASSERT_EQ(a, a);
		ASSERT_EQ(SingleCardinality(1), a);
		ASSERT_EQ(b, b);
		ASSERT_EQ(c, c);

		ASSERT_FALSE(a == b);
		ASSERT_FALSE(b == c);
		ASSERT_FALSE(a == c);
	}

	{
		RangeCardinality a{1, 1};
		RangeCardinality b{1, 2};
		RangeCardinality c{2, 2};

		ASSERT_EQ(a, a);
		ASSERT_EQ(RangeCardinality(1, 1), a);
		ASSERT_EQ(b, b);
		ASSERT_EQ(c, c);

		ASSERT_FALSE(a == b);
		// TODO: Here the semantics break down. It should be equal, in fact.
		ASSERT_FALSE(a == SingleCardinality(1));
		ASSERT_FALSE(b == c);
		ASSERT_FALSE(a == c);
	}
}
}
}
