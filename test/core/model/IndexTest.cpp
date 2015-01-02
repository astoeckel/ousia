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

#include <iostream>

#include <core/model/Node.hpp>

namespace ousia {

TEST(Index, simple)
{
	Manager mgr{1};
	Rooted<Managed> owner{new Managed(mgr)};
	{
		NodeVector<Node> nodes(owner);
		Index &idx = nodes.getIndex();

		Node* n1 = new Node(mgr, "node1");
		Node* n2 = new Node(mgr, "node2");
		Node* n3 = new Node(mgr, "node3");

		nodes.push_back(n1);
		nodes.push_back(n2);
		nodes.push_back(n3);

		// Index
		ASSERT_EQ(n1, idx.resolve("node1"));
		ASSERT_EQ(n2, idx.resolve("node2"));
		ASSERT_EQ(n3, idx.resolve("node3"));
		ASSERT_EQ(nullptr, idx.resolve("node4"));

		// Rename
		n2->setName("node2b");
		ASSERT_EQ(nullptr, idx.resolve("node2"));
		ASSERT_EQ(n2, idx.resolve("node2b"));

		// Delete
		nodes.erase(nodes.begin() + 1);
		ASSERT_EQ(nullptr, idx.resolve("node2b"));

		nodes.erase(nodes.begin() + 0);
		ASSERT_EQ(nullptr, idx.resolve("node1"));

		// Another rename
		n3->setName("node3b");
		ASSERT_EQ(nullptr, idx.resolve("node3"));
		ASSERT_EQ(n3, idx.resolve("node3b"));
	}
}

TEST(Index, shared)
{
	Manager mgr{1};
	Rooted<Managed> owner{new Managed(mgr)};
	Index idx;
	{
		NodeVector<Node, Index&> nodes1(owner, idx);
		NodeVector<Node, Index&> nodes2(owner, idx);

		ASSERT_EQ(&idx, &nodes1.getIndex());
		ASSERT_EQ(&idx, &nodes2.getIndex());

		Node* n1 = new Node(mgr, "node1");
		Node* n2 = new Node(mgr, "node2");
		Node* n3 = new Node(mgr, "node3");

		nodes1.push_back(n1);
		nodes1.push_back(n2);
		nodes2.push_back(n3);

		ASSERT_EQ(n1, idx.resolve("node1"));
		ASSERT_EQ(n2, idx.resolve("node2"));
		ASSERT_EQ(n3, idx.resolve("node3"));
		ASSERT_EQ(nullptr, idx.resolve("node4"));
	}
}

}
