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

#include <plugins/plain/DynamicTokenTree.hpp>

namespace ousia {

static const TokenDescriptor *d1 = reinterpret_cast<const TokenDescriptor*>(1);
static const TokenDescriptor *d2 = reinterpret_cast<const TokenDescriptor*>(2);
static const TokenDescriptor *d3 = reinterpret_cast<const TokenDescriptor*>(3);
static const TokenDescriptor *d4 = reinterpret_cast<const TokenDescriptor*>(4);

TEST(DynamicTokenTree, registerToken)
{
	DynamicTokenTree tree;

	ASSERT_TRUE(tree.registerToken("a", d1));
	ASSERT_TRUE(tree.registerToken("ab", d2));
	ASSERT_TRUE(tree.registerToken("b", d3));
	ASSERT_TRUE(tree.registerToken("hello", d4));

	ASSERT_FALSE(tree.registerToken("", d1));
	ASSERT_FALSE(tree.registerToken("a", d4));
	ASSERT_FALSE(tree.registerToken("ab", d4));
	ASSERT_FALSE(tree.registerToken("b", d4));
	ASSERT_FALSE(tree.registerToken("hello", d4));

	ASSERT_EQ(d1, tree.hasToken("a"));
	ASSERT_EQ(d2, tree.hasToken("ab"));
	ASSERT_EQ(d3, tree.hasToken("b"));
	ASSERT_EQ(d4, tree.hasToken("hello"));
	ASSERT_EQ(nullptr, tree.hasToken(""));
	ASSERT_EQ(nullptr, tree.hasToken("abc"));
}

TEST(DynamicTokenTree, unregisterToken)
{
	DynamicTokenTree tree;

	ASSERT_TRUE(tree.registerToken("a", d1));
	ASSERT_FALSE(tree.registerToken("a", d4));

	ASSERT_TRUE(tree.registerToken("ab", d2));
	ASSERT_FALSE(tree.registerToken("ab", d4));

	ASSERT_TRUE(tree.registerToken("b", d3));
	ASSERT_FALSE(tree.registerToken("b", d4));

	ASSERT_EQ(d1, tree.hasToken("a"));
	ASSERT_EQ(d2, tree.hasToken("ab"));
	ASSERT_EQ(d3, tree.hasToken("b"));

	ASSERT_TRUE(tree.unregisterToken("a"));
	ASSERT_FALSE(tree.unregisterToken("a"));

	ASSERT_EQ(nullptr, tree.hasToken("a"));
	ASSERT_EQ(d2, tree.hasToken("ab"));
	ASSERT_EQ(d3, tree.hasToken("b"));

	ASSERT_TRUE(tree.unregisterToken("b"));
	ASSERT_FALSE(tree.unregisterToken("b"));

	ASSERT_EQ(nullptr, tree.hasToken("a"));
	ASSERT_EQ(d2, tree.hasToken("ab"));
	ASSERT_EQ(nullptr, tree.hasToken("b"));

	ASSERT_TRUE(tree.unregisterToken("ab"));
	ASSERT_FALSE(tree.unregisterToken("ab"));

	ASSERT_EQ(nullptr, tree.hasToken("a"));
	ASSERT_EQ(nullptr, tree.hasToken("ab"));
	ASSERT_EQ(nullptr, tree.hasToken("b"));
}


}
