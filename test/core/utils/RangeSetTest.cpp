/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#include <core/utils/RangeSet.hpp>

namespace ousia {
namespace model {

TEST(Range, IsValid)
{
	ASSERT_FALSE(Range<int>().isValid());
	ASSERT_TRUE(Range<int>(10).isValid());
	ASSERT_TRUE(Range<int>(10, 20).isValid());
	ASSERT_FALSE(Range<int>(20, 10).isValid());
}

TEST(Range, InRange)
{
	Range<int> r(10, 20);
	ASSERT_FALSE(r.inRange(0));
	ASSERT_FALSE(r.inRange(21));
	ASSERT_TRUE(r.inRange(10));
	ASSERT_TRUE(r.inRange(20));
	ASSERT_TRUE(r.inRange(15));
}

TEST(Range, overlaps)
{
	ASSERT_FALSE(Range<int>(10, 20).overlaps(Range<int>(0, 9)));
	ASSERT_FALSE(Range<int>(10, 20).overlaps(Range<int>(21, 30)));
	ASSERT_TRUE(Range<int>(10, 20).overlaps(Range<int>(0, 10)));
	ASSERT_TRUE(Range<int>(10, 20).overlaps(Range<int>(20, 30)));
	ASSERT_TRUE(Range<int>(10, 20).overlaps(Range<int>(5, 15)));
	ASSERT_TRUE(Range<int>(10, 20).overlaps(Range<int>(15, 25)));
	ASSERT_TRUE(Range<int>(10, 20).overlaps(Range<int>(15, 19)));
	ASSERT_TRUE(Range<int>(10, 20).overlaps(Range<int>(15, 15)));
	ASSERT_TRUE(Range<int>(10, 20).overlaps(Range<int>(10, 20)));
	ASSERT_TRUE(Range<int>(10, 20).overlaps(Range<int>(0, 30)));
}

TEST(Range, CoveredBy)
{
	ASSERT_FALSE(Range<int>(10, 20).coveredBy(Range<int>(0, 9)));
	ASSERT_FALSE(Range<int>(10, 20).coveredBy(Range<int>(21, 30)));
	ASSERT_FALSE(Range<int>(10, 20).coveredBy(Range<int>(0, 10)));
	ASSERT_FALSE(Range<int>(10, 20).coveredBy(Range<int>(20, 30)));
	ASSERT_FALSE(Range<int>(10, 20).coveredBy(Range<int>(5, 15)));
	ASSERT_FALSE(Range<int>(10, 20).coveredBy(Range<int>(15, 25)));
	ASSERT_FALSE(Range<int>(10, 20).coveredBy(Range<int>(15, 19)));
	ASSERT_FALSE(Range<int>(10, 20).coveredBy(Range<int>(15, 15)));
	ASSERT_TRUE(Range<int>(10, 20).coveredBy(Range<int>(10, 20)));
	ASSERT_TRUE(Range<int>(10, 20).coveredBy(Range<int>(0, 30)));
}

TEST(Range, Covers)
{
	ASSERT_FALSE(Range<int>(10, 20).covers(Range<int>(0, 9)));
	ASSERT_FALSE(Range<int>(10, 20).covers(Range<int>(21, 30)));
	ASSERT_FALSE(Range<int>(10, 20).covers(Range<int>(0, 10)));
	ASSERT_FALSE(Range<int>(10, 20).covers(Range<int>(20, 30)));
	ASSERT_FALSE(Range<int>(10, 20).covers(Range<int>(5, 15)));
	ASSERT_FALSE(Range<int>(10, 20).covers(Range<int>(15, 25)));
	ASSERT_TRUE(Range<int>(10, 20).covers(Range<int>(15, 19)));
	ASSERT_TRUE(Range<int>(10, 20).covers(Range<int>(15, 15)));
	ASSERT_TRUE(Range<int>(10, 20).covers(Range<int>(10, 20)));
	ASSERT_FALSE(Range<int>(10, 20).covers(Range<int>(0, 30)));
}

TEST(RangeSet, Neighbours)
{
	ASSERT_TRUE(Range<int>(10, 19).neighbours(Range<int>(20, 30)));
	ASSERT_TRUE(Range<int>(20, 29).neighbours(Range<int>(10, 19)));
}

TEST(Range, Merge)
{
	Range<int> r1(10, 20);
	Range<int> r2(15, 25);
	Range<int> r3(5, 15);
	Range<int> rM = r1.merge(r2).merge(r3);
	ASSERT_EQ(rM.start, 5);
	ASSERT_EQ(rM.end, 25);
}

TEST(RangeSet, Merge)
{
	RangeSet<int> s;
	auto &ranges = s.getRanges();

	// Insert some non-overlapping elements into the range. We expect these to
	// be just inserted into the ranges.
	s.merge(Range<int>( 0, 10));
	s.merge(Range<int>(20, 30));
	s.merge(Range<int>(40, 50));
	s.merge(Range<int>(60, 70));
	{
		ASSERT_EQ(ranges.size(), 4);

		auto it = ranges.begin();
		ASSERT_EQ((*it).start, 0);
		ASSERT_EQ((*it).end, 10);

		it++;
		ASSERT_EQ((*it).start, 20);
		ASSERT_EQ((*it).end, 30);

		it++;
		ASSERT_EQ((*it).start, 40);
		ASSERT_EQ((*it).end, 50);

		it++;
		ASSERT_EQ((*it).start, 60);
		ASSERT_EQ((*it).end, 70);
	}

	// Now insert an element which spans the second and third element
	s.merge(Range<int>(15, 55));
	{
		ASSERT_EQ(ranges.size(), 3);

		auto it = ranges.begin();
		ASSERT_EQ((*it).start, 0);
		ASSERT_EQ((*it).end, 10);

		it++;
		ASSERT_EQ((*it).start, 15);
		ASSERT_EQ((*it).end, 55);

		it++;
		ASSERT_EQ((*it).start, 60);
		ASSERT_EQ((*it).end, 70);
	}

	// Now insert an element which expands the first element
	s.merge(Range<int>(-10, 11));
	{
		ASSERT_EQ(ranges.size(), 3);

		auto it = ranges.begin();
		ASSERT_EQ((*it).start, -10);
		ASSERT_EQ((*it).end, 11);

		it++;
		ASSERT_EQ((*it).start, 15);
		ASSERT_EQ((*it).end, 55);

		it++;
		ASSERT_EQ((*it).start, 60);
		ASSERT_EQ((*it).end, 70);
	}

	// Now insert an element which merges the last two elements
	s.merge(Range<int>(13, 70));
	{
		ASSERT_EQ(ranges.size(), 2);

		auto it = ranges.begin();
		ASSERT_EQ((*it).start, -10);
		ASSERT_EQ((*it).end, 11);

		it++;
		ASSERT_EQ((*it).start, 13);
		ASSERT_EQ((*it).end, 70);
	}

	// Now insert an element which merges the remaining elements
	s.merge(Range<int>(-9, 12));
	{
		ASSERT_EQ(ranges.size(), 1);

		auto it = ranges.begin();
		ASSERT_EQ((*it).start, -10);
		ASSERT_EQ((*it).end, 70);
	}

}

TEST(RangeSet, Contains)
{
	RangeSet<int> s;

	// Insert some non-overlapping elements into the range. We expect these to
	// be just inserted into the ranges.
	s.merge(Range<int>( 0, 10));
	s.merge(Range<int>(20, 30));
	s.merge(Range<int>(40, 50));
	s.merge(Range<int>(60, 70));
	s.merge(Range<int>(71));
	s.merge(Range<int>(72));
	s.merge(Range<int>(73));
	s.merge(Range<int>(74));

	ASSERT_TRUE(s.contains(60));
	ASSERT_TRUE(s.contains(0));
	ASSERT_TRUE(s.contains(25));
	ASSERT_TRUE(s.contains(73));
	ASSERT_TRUE(s.contains(Range<int>(25, 30)));
	ASSERT_FALSE(s.contains(Range<int>(25, 35)));
	ASSERT_TRUE(s.contains(Range<int>(0, 10)));
	ASSERT_TRUE(s.contains(Range<int>(70, 74)));
}

}
}

