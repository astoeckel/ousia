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

#include <core/utils/CSSParser.hpp>

namespace ousia {
namespace utils {
TEST(Specificity, testOperators)
{
	Specificity s1{0,0,1};
	Specificity s2{0,1,1};
	Specificity s3{1,1,1};
	Specificity s4{0,0,2};
	Specificity s5{1,0,2};
	
	//This should be s1 < s4 < s2 < s5 < s3

	ASSERT_TRUE(s1 == s1);
	ASSERT_FALSE(s1 < s1);
	ASSERT_FALSE(s1 > s1);
	ASSERT_FALSE(s1 == s2);
	ASSERT_TRUE(s1 < s2);
	ASSERT_FALSE(s1 > s2);
	ASSERT_FALSE(s1 == s3);
	ASSERT_TRUE(s1 < s3);
	ASSERT_FALSE(s1 > s3);
	ASSERT_FALSE(s1 == s4);
	ASSERT_TRUE(s1 < s4);
	ASSERT_FALSE(s1 > s4);
	ASSERT_FALSE(s1 == s5);
	ASSERT_TRUE(s1 < s5);
	ASSERT_FALSE(s1 > s5);

	ASSERT_FALSE(s2 == s1);
	ASSERT_FALSE(s2 < s1);
	ASSERT_TRUE(s2 > s1);
	ASSERT_TRUE(s2 == s2);
	ASSERT_FALSE(s2 < s2);
	ASSERT_FALSE(s2 > s2);
	ASSERT_FALSE(s2 == s3);
	ASSERT_TRUE(s2 < s3);
	ASSERT_FALSE(s2 > s3);
	ASSERT_FALSE(s2 == s4);
	ASSERT_FALSE(s2 < s4);
	ASSERT_TRUE(s2 > s4);
	ASSERT_FALSE(s2 == s5);
	ASSERT_TRUE(s2 < s5);
	ASSERT_FALSE(s2 > s5);

	ASSERT_FALSE(s3 == s1);
	ASSERT_FALSE(s3 < s1);
	ASSERT_TRUE(s3 > s1);
	ASSERT_FALSE(s3 == s2);
	ASSERT_FALSE(s3 < s2);
	ASSERT_TRUE(s3 > s2);
	ASSERT_TRUE(s3 == s3);
	ASSERT_FALSE(s3 < s3);
	ASSERT_FALSE(s3 > s3);
	ASSERT_FALSE(s3 == s4);
	ASSERT_FALSE(s3 < s4);
	ASSERT_TRUE(s3 > s4);
	ASSERT_FALSE(s3 == s5);
	ASSERT_FALSE(s3 < s5);
	ASSERT_TRUE(s3 > s5);

	ASSERT_FALSE(s4 == s1);
	ASSERT_FALSE(s4 < s1);
	ASSERT_TRUE(s4 > s1);
	ASSERT_FALSE(s4 == s2);
	ASSERT_TRUE(s4 < s2);
	ASSERT_FALSE(s4 > s2);
	ASSERT_FALSE(s4 == s3);
	ASSERT_TRUE(s4 < s3);
	ASSERT_FALSE(s4 > s3);
	ASSERT_TRUE(s4 == s4);
	ASSERT_FALSE(s4 < s4);
	ASSERT_FALSE(s4 > s4);
	ASSERT_FALSE(s4 == s5);
	ASSERT_TRUE(s4 < s5);
	ASSERT_FALSE(s4 > s5);

	ASSERT_FALSE(s5 == s1);
	ASSERT_FALSE(s5 < s1);
	ASSERT_TRUE(s5 > s1);
	ASSERT_FALSE(s5 == s2);
	ASSERT_FALSE(s5 < s2);
	ASSERT_TRUE(s5 > s2);
	ASSERT_FALSE(s5 == s3);
	ASSERT_TRUE(s5 < s3);
	ASSERT_FALSE(s5 > s3);
	ASSERT_FALSE(s5 == s4);
	ASSERT_FALSE(s5 < s4);
	ASSERT_TRUE(s5 > s4);
	ASSERT_TRUE(s5 == s5);
	ASSERT_FALSE(s5 < s5);
	ASSERT_FALSE(s5 > s5);
}
}
}
