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

#include <core/utils/Tokenizer.hpp>

namespace ousia {
namespace utils {
TEST(TokenTreeNode, testConstructor)
{
	TokenTreeNode root{{{"a", 1}, {"aab", 2}, {"aac", 3}, {"abd", 4}}};

	ASSERT_EQ(-1, root.tokenId);
	ASSERT_EQ(1, root.children.size());
	ASSERT_TRUE(root.children.find('a') != root.children.end());

	const TokenTreeNode &a = root.children.at('a');
	ASSERT_EQ(1, a.tokenId);
	ASSERT_EQ(2, a.children.size());
	ASSERT_TRUE(a.children.find('a') != a.children.end());
	ASSERT_TRUE(a.children.find('b') != a.children.end());

	const TokenTreeNode &aa = a.children.at('a');
	ASSERT_EQ(-1, aa.tokenId);
	ASSERT_EQ(2, aa.children.size());
	ASSERT_TRUE(aa.children.find('b') != aa.children.end());
	ASSERT_TRUE(aa.children.find('c') != aa.children.end());

	const TokenTreeNode &aab = aa.children.at('b');
	ASSERT_EQ(2, aab.tokenId);
	ASSERT_EQ(0, aab.children.size());

	const TokenTreeNode &aac = aa.children.at('c');
	ASSERT_EQ(3, aac.tokenId);
	ASSERT_EQ(0, aac.children.size());

	const TokenTreeNode &ab = a.children.at('b');
	ASSERT_EQ(-1, ab.tokenId);
	ASSERT_EQ(1, ab.children.size());
	ASSERT_TRUE(ab.children.find('d') != ab.children.end());

	const TokenTreeNode &abd = ab.children.at('d');
	ASSERT_EQ(4, abd.tokenId);
	ASSERT_EQ(0, abd.children.size());
}
}
}
