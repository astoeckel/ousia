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

#include <array>
#include <string>
#include <iostream>

#include <gtest/gtest.h>

#include <core/dom/Node.hpp>

namespace ousia {
namespace dom {

/* Class NodeDescriptor */

TEST(NodeDescriptor, nodeDegree)
{
	// Do not use actual Node in this test -- we don't want to test their
	// behaviour
	NodeDescriptor nd;
	Node *n1 = reinterpret_cast<Node*>(intptr_t{0x10});
	Node *n2 = reinterpret_cast<Node*>(intptr_t{0x20});

	// Input degree
	ASSERT_EQ(0, nd.refIn.size());
	ASSERT_EQ(0, nd.refInCount(n1));

	nd.incrNodeDegree(RefDir::in, n1);
	ASSERT_EQ(1, nd.refInCount());
	ASSERT_EQ(1, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(1, nd.refIn.size());

	nd.incrNodeDegree(RefDir::in, n1);
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(1, nd.refIn.size());

	nd.incrNodeDegree(RefDir::in, n2);
	ASSERT_EQ(3, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2, nd.refIn.size());

	nd.incrNodeDegree(RefDir::in, nullptr);
	ASSERT_EQ(4, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2, nd.refIn.size());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::in, n1));
	ASSERT_EQ(3, nd.refInCount());
	ASSERT_EQ(1, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2, nd.refIn.size());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::in, n1));
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(1, nd.refIn.size());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::in, n2));
	ASSERT_EQ(1, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(0, nd.refIn.size());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::in, nullptr));
	ASSERT_EQ(0, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(0, nd.refIn.size());

	// Output degree
	ASSERT_EQ(0, nd.refOut.size());
	ASSERT_EQ(0, nd.refOutCount(n1));

	nd.incrNodeDegree(RefDir::out, n1);
	ASSERT_EQ(1, nd.refOutCount());
	ASSERT_EQ(1, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(1, nd.refOut.size());

	nd.incrNodeDegree(RefDir::out, n1);
	ASSERT_EQ(2, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(1, nd.refOut.size());

	nd.incrNodeDegree(RefDir::out, n2);
	ASSERT_EQ(3, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2, nd.refOut.size());

	nd.incrNodeDegree(RefDir::out, nullptr);
	ASSERT_EQ(3, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2, nd.refOut.size());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::out, n1));
	ASSERT_EQ(2, nd.refOutCount());
	ASSERT_EQ(1, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2, nd.refOut.size());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::out, n1));
	ASSERT_EQ(1, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(1, nd.refOut.size());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::out, n2));
	ASSERT_EQ(0, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(0, nd.refOut.size());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::out, nullptr));
	ASSERT_EQ(0, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(0, nd.refOut.size());
}

TEST(NodeDescriptor, rootRefCount)
{
	NodeDescriptor nd;
	ASSERT_EQ(0, nd.rootRefCount);

	nd.incrNodeDegree(RefDir::in, nullptr);
	ASSERT_EQ(1, nd.rootRefCount);

	nd.incrNodeDegree(RefDir::out, nullptr);
	ASSERT_EQ(2, nd.rootRefCount);

	ASSERT_EQ(2, nd.refInCount(nullptr));
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(0, nd.refOutCount(nullptr));
	ASSERT_EQ(0, nd.refOutCount());

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::out, nullptr));
	ASSERT_EQ(1, nd.rootRefCount);

	ASSERT_TRUE(nd.decrNodeDegree(RefDir::in, nullptr));
	ASSERT_EQ(0, nd.rootRefCount);

	ASSERT_FALSE(nd.decrNodeDegree(RefDir::in, nullptr));
	ASSERT_EQ(0, nd.rootRefCount);
}

/* Class Handle */

TEST(Handle, equalsAndAssign)
{
	NodeManager mgr(1);

	Node *n1 = new Node(mgr), *n2 = new Node(mgr);

	RootedNode rh1{n1};
	RootedNode rh2{n2};

	NodeHandle h2{n2, n1};

	// Equals operator
	ASSERT_TRUE(rh1 == n1);
	ASSERT_TRUE(n1 == rh1);
	ASSERT_FALSE(rh1 == rh2);
	ASSERT_TRUE(rh2 == h2);
	ASSERT_TRUE(h2 == rh2);

	// Assignment operator
	RootedNode rh2b;

	ASSERT_FALSE(rh2b == rh2);
	rh2b = rh2;
	ASSERT_TRUE(rh2b == rh2);
	ASSERT_TRUE(rh2b == h2);

	rh2b = h2;
	ASSERT_TRUE(rh2b == h2);

	NodeHandle h2b;
	ASSERT_FALSE(rh2 == h2b);
	ASSERT_FALSE(h2 == h2b);
	h2b = h2;
	ASSERT_TRUE(rh2 == h2b);
	ASSERT_TRUE(h2 == h2b);

	NodeHandle h2c{h2b, n1};
	ASSERT_TRUE(h2b == h2c);
}

/* Class NodeManager */

class TestNode : public Node {
private:
	bool &alive;

	std::vector<Handle<Node>> refs;

public:
	TestNode(NodeManager &mgr, bool &alive) : Node(mgr), alive(alive)
	{
		alive = true;
	}

	~TestNode() override { alive = false; }

	void addRef(BaseHandle<Node> h) { refs.push_back(acquire(h)); }

	void deleteRef(BaseHandle<Node> h) {
		for (auto it = refs.begin(); it != refs.end();) {
			if (*it == h) {
				it = refs.erase(it);
			} else {
				it++;
			}
		}
	}
};

TEST(NodeManager, linearDependencies)
{
	std::array<bool, 4> a;
	a.fill(false);

	NodeManager mgr(1);
	{
		TestNode *n1, *n2, *n3;
		n1 = new TestNode(mgr, a[1]);
		n2 = new TestNode(mgr, a[2]);
		n3 = new TestNode(mgr, a[3]);

		{
			RootedHandle<TestNode> hr{new TestNode(mgr, a[0])};

			// All nodes must have set their "alive" flag to true
			for (bool v : a) {
				ASSERT_TRUE(v);
			}

			// Create a linear dependency chain
			hr->addRef(n1);
			n1->addRef(n2);
			n2->addRef(n3);
		}

		// All nodes must have set their "alive" flag to false
		for (bool v : a) {
			ASSERT_FALSE(v);
		}
	}
}

TEST(NodeManager, cyclicDependencies)
{
	std::array<bool, 4> a;
	a.fill(false);

	NodeManager mgr(1);
	{
		TestNode *n1, *n2, *n3;
		n1 = new TestNode(mgr, a[1]);
		n2 = new TestNode(mgr, a[2]);
		n3 = new TestNode(mgr, a[3]);

		{
			RootedHandle<TestNode> hr{new TestNode(mgr, a[0])};

			// All nodes must have set their "alive" flag to true
			for (bool v : a) {
				ASSERT_TRUE(v);
			}

			// Create a linear dependency chain
			hr->addRef(n1);
			n1->addRef(n2);
			n2->addRef(n3);
			n3->addRef(n1);
		}

		// All nodes must have set their "alive" flag to false
		for (bool v : a) {
			ASSERT_FALSE(v);
		}
	}
}

TEST(NodeManager, doubleRooted)
{
	std::array<bool, 4> a;
	a.fill(false);

	NodeManager mgr(1);
	{
		TestNode *n1, *n2;
		n1 = new TestNode(mgr, a[1]);
		n2 = new TestNode(mgr, a[2]);

		{
			RootedHandle<TestNode> hr1{new TestNode(mgr, a[0])};
			{
				RootedHandle<TestNode> hr2{new TestNode(mgr, a[3])};

				// All nodes must have set their "alive" flag to true
				for (bool v : a) {
					ASSERT_TRUE(v);
				}

				// Create cyclical dependency between n2 and n1
				n1->addRef(n2);
				n2->addRef(n1);

				// Reference n1 and n2 in the rooted nodes
				hr1->addRef(n1);
				hr2->addRef(n2);
			}

			// hr2 is dead, all other nodes are still alive
			ASSERT_FALSE(a[3]);
			ASSERT_TRUE(a[0] && a[1] && a[2]);
		}

		// All nodes are dead
		for (bool v : a) {
			ASSERT_FALSE(v);
		}
	}
}

TEST(NodeManager, disconnectSubgraph)
{
	std::array<bool, 4> a;
	a.fill(false);

	NodeManager mgr(1);
	{
		TestNode *n1, *n2, *n3;
		n1 = new TestNode(mgr, a[1]);
		n2 = new TestNode(mgr, a[2]);
		n3 = new TestNode(mgr, a[3]);

		{
			RootedHandle<TestNode> hr{new TestNode(mgr, a[0])};

			// Create a linear dependency chain
			hr->addRef(n1);
			n1->addRef(n2);
			n2->addRef(n3);

			// All nodes must have set their "alive" flag to true
			for (bool v : a) {
				ASSERT_TRUE(v);
			}

			// Remove the reference from n1 to n2
			n1->deleteRef(n2);

			// Nodes 2 and 3 must be dead, all others alive
			ASSERT_FALSE(a[2] || a[3]);
			ASSERT_TRUE(a[0] && a[1]);
		}

		// All nodes must have set their "alive" flag to false
		for (bool v : a) {
			ASSERT_FALSE(v);
		}
	}
}


}
}

