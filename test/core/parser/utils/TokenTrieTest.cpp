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

#include <core/parser/utils/TokenTrie.hpp>

namespace ousia {

static const TokenId t1 = 0;
static const TokenId t2 = 1;
static const TokenId t3 = 2;
static const TokenId t4 = 3;

TEST(TokenTrie, registerToken)
{
	TokenTrie tree;

	ASSERT_TRUE(tree.registerToken("a", t1));
	ASSERT_TRUE(tree.registerToken("ab", t2));
	ASSERT_TRUE(tree.registerToken("b", t3));
	ASSERT_TRUE(tree.registerToken("hello", t4));

	ASSERT_FALSE(tree.registerToken("", t1));
	ASSERT_FALSE(tree.registerToken("a", t4));
	ASSERT_FALSE(tree.registerToken("ab", t4));
	ASSERT_FALSE(tree.registerToken("b", t4));
	ASSERT_FALSE(tree.registerToken("hello", t4));

	ASSERT_EQ(t1, tree.hasToken("a"));
	ASSERT_EQ(t2, tree.hasToken("ab"));
	ASSERT_EQ(t3, tree.hasToken("b"));
	ASSERT_EQ(t4, tree.hasToken("hello"));
	ASSERT_EQ(Tokens::Empty, tree.hasToken(""));
	ASSERT_EQ(Tokens::Empty, tree.hasToken("abc"));
}

TEST(TokenTrie, unregisterToken)
{
	TokenTrie tree;

	ASSERT_TRUE(tree.registerToken("a", t1));
	ASSERT_FALSE(tree.registerToken("a", t4));

	ASSERT_TRUE(tree.registerToken("ab", t2));
	ASSERT_FALSE(tree.registerToken("ab", t4));

	ASSERT_TRUE(tree.registerToken("b", t3));
	ASSERT_FALSE(tree.registerToken("b", t4));

	ASSERT_EQ(t1, tree.hasToken("a"));
	ASSERT_EQ(t2, tree.hasToken("ab"));
	ASSERT_EQ(t3, tree.hasToken("b"));

	ASSERT_TRUE(tree.unregisterToken("a"));
	ASSERT_FALSE(tree.unregisterToken("a"));

	ASSERT_EQ(Tokens::Empty, tree.hasToken("a"));
	ASSERT_EQ(t2, tree.hasToken("ab"));
	ASSERT_EQ(t3, tree.hasToken("b"));

	ASSERT_TRUE(tree.unregisterToken("b"));
	ASSERT_FALSE(tree.unregisterToken("b"));

	ASSERT_EQ(Tokens::Empty, tree.hasToken("a"));
	ASSERT_EQ(t2, tree.hasToken("ab"));
	ASSERT_EQ(Tokens::Empty, tree.hasToken("b"));

	ASSERT_TRUE(tree.unregisterToken("ab"));
	ASSERT_FALSE(tree.unregisterToken("ab"));

	ASSERT_EQ(Tokens::Empty, tree.hasToken("a"));
	ASSERT_EQ(Tokens::Empty, tree.hasToken("ab"));
	ASSERT_EQ(Tokens::Empty, tree.hasToken("b"));
}
}

