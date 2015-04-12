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

#include <core/parser/stack/TokenStack.hpp>

namespace ousia {
namespace parser_stack {

static Manager mgr;

static Rooted<Node> nd1{new Node(mgr)};
static Rooted<Node> nd2{new Node(mgr)};
static Rooted<Node> nd3{new Node(mgr)};

static const std::vector<SyntaxDescriptor> ListA{
    SyntaxDescriptor(Tokens::Empty, 1, Tokens::Empty, nd1, 0, true),
    SyntaxDescriptor(2, Tokens::Empty, Tokens::Empty, nd2, 2, true),
    SyntaxDescriptor(3, Tokens::Empty, Tokens::Empty, nd3, 1, true)};

static const std::vector<SyntaxDescriptor> ListB{
    SyntaxDescriptor(Tokens::Empty, 1, Tokens::Empty, nd1, -1, true),
    SyntaxDescriptor(2, Tokens::Empty, 3, nd3, 3, true),
};

static const std::vector<SyntaxDescriptor> ListC{
    SyntaxDescriptor(Tokens::Empty, Tokens::Empty, 4, nd2, 5, true),
    SyntaxDescriptor(Tokens::Empty, Tokens::Empty, 3, nd3, 6, true),
};

TEST(TokenStack, tokens)
{
	TokenStack ts;
	ASSERT_EQ((TokenSet{}), ts.tokens());
	ts.pushTokens(ListA);
	ASSERT_EQ((TokenSet{1, 2, 3}), ts.tokens());
	ts.pushTokens(ListB);
	ASSERT_EQ((TokenSet{1, 2, 3}), ts.tokens());
	ts.pushTokens(ListC);
	ASSERT_EQ((TokenSet{3, 4}), ts.tokens());
	ts.popTokens();
	ASSERT_EQ((TokenSet{1, 2, 3}), ts.tokens());
	ts.popTokens();
	ASSERT_EQ((TokenSet{1, 2, 3}), ts.tokens());
	ts.popTokens();
	ASSERT_EQ((TokenSet{}), ts.tokens());
}

TEST(TokenStack, lookup)
{
	TokenStack ts;
	ts.pushTokens(ListA);
	ts.pushTokens(ListB);
	ts.pushTokens(ListC);

	TokenDescriptor descr = ts.lookup(3);
	ASSERT_EQ(0U, descr.open.size());
	ASSERT_EQ(0U, descr.close.size());
	ASSERT_EQ(1U, descr.shortForm.size());
	ASSERT_EQ(ListC[1], descr.shortForm[0]);
}

TEST(TokenStack, sorting)
{
	TokenStack ts;
	std::vector<SyntaxDescriptor> descrs;
	descrs.insert(descrs.end(), ListC.begin(), ListC.end());
	descrs.insert(descrs.end(), ListA.begin(), ListA.end());
	descrs.insert(descrs.end(), ListB.begin(), ListB.end());
	ts.pushTokens(descrs);

	TokenDescriptor descr = ts.lookup(3);
	ASSERT_EQ(1U, descr.open.size());
	ASSERT_EQ(0U, descr.close.size());
	ASSERT_EQ(2U, descr.shortForm.size());
	ASSERT_EQ(ListA[2], descr.open[0]);
	ASSERT_EQ(ListB[1], descr.shortForm[0]);
	ASSERT_EQ(ListC[1], descr.shortForm[1]);
}

}
}
