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

#include <core/Managed.hpp>

#include "TestManaged.hpp"

namespace ousia {

/* Class ObjectDescriptor */

TEST(ObjectDescriptor, Degree)
{
	// Do not use actual Managed in this test -- we don't want to test their
	// behaviour
	ObjectDescriptor nd;
	Managed *n1 = reinterpret_cast<Managed *>(intptr_t{0x10});
	Managed *n2 = reinterpret_cast<Managed *>(intptr_t{0x20});

	// Input degree
	ASSERT_EQ(0, nd.refIn.size());
	ASSERT_EQ(0, nd.refInCount(n1));

	nd.incrDegree(RefDir::IN, n1);
	ASSERT_EQ(1, nd.refInCount());
	ASSERT_EQ(1, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(1, nd.refIn.size());

	nd.incrDegree(RefDir::IN, n1);
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(1, nd.refIn.size());

	nd.incrDegree(RefDir::IN, n2);
	ASSERT_EQ(3, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2, nd.refIn.size());

	nd.incrDegree(RefDir::IN, nullptr);
	ASSERT_EQ(4, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2, nd.refIn.size());

	ASSERT_TRUE(nd.decrDegree(RefDir::IN, n1));
	ASSERT_EQ(3, nd.refInCount());
	ASSERT_EQ(1, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2, nd.refIn.size());

	ASSERT_TRUE(nd.decrDegree(RefDir::IN, n1));
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(1, nd.refIn.size());

	ASSERT_TRUE(nd.decrDegree(RefDir::IN, n2));
	ASSERT_EQ(1, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(0, nd.refIn.size());

	ASSERT_TRUE(nd.decrDegree(RefDir::IN, nullptr));
	ASSERT_EQ(0, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(0, nd.refIn.size());

	// Output degree
	ASSERT_EQ(0, nd.refOut.size());
	ASSERT_EQ(0, nd.refOutCount(n1));

	nd.incrDegree(RefDir::OUT, n1);
	ASSERT_EQ(1, nd.refOutCount());
	ASSERT_EQ(1, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(1, nd.refOut.size());

	nd.incrDegree(RefDir::OUT, n1);
	ASSERT_EQ(2, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(1, nd.refOut.size());

	nd.incrDegree(RefDir::OUT, n2);
	ASSERT_EQ(3, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2, nd.refOut.size());

	nd.incrDegree(RefDir::OUT, nullptr);
	ASSERT_EQ(3, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2, nd.refOut.size());

	ASSERT_TRUE(nd.decrDegree(RefDir::OUT, n1));
	ASSERT_EQ(2, nd.refOutCount());
	ASSERT_EQ(1, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2, nd.refOut.size());

	ASSERT_TRUE(nd.decrDegree(RefDir::OUT, n1));
	ASSERT_EQ(1, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(1, nd.refOut.size());

	ASSERT_TRUE(nd.decrDegree(RefDir::OUT, n2));
	ASSERT_EQ(0, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(0, nd.refOut.size());

	ASSERT_TRUE(nd.decrDegree(RefDir::OUT, nullptr));
	ASSERT_EQ(0, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(0, nd.refOut.size());
}

TEST(ObjectDescriptor, rootRefCount)
{
	ObjectDescriptor nd;
	ASSERT_EQ(0, nd.rootRefCount);

	nd.incrDegree(RefDir::IN, nullptr);
	ASSERT_EQ(1, nd.rootRefCount);

	nd.incrDegree(RefDir::OUT, nullptr);
	ASSERT_EQ(2, nd.rootRefCount);

	ASSERT_EQ(2, nd.refInCount(nullptr));
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(0, nd.refOutCount(nullptr));
	ASSERT_EQ(0, nd.refOutCount());

	ASSERT_TRUE(nd.decrDegree(RefDir::OUT, nullptr));
	ASSERT_EQ(1, nd.rootRefCount);

	ASSERT_TRUE(nd.decrDegree(RefDir::IN, nullptr));
	ASSERT_EQ(0, nd.rootRefCount);

	ASSERT_FALSE(nd.decrDegree(RefDir::IN, nullptr));
	ASSERT_EQ(0, nd.rootRefCount);
}

/* Class Owned */

TEST(Owned, equalsAndAssign)
{
	Manager mgr(1);

	Managed *n1 = new Managed(mgr), *n2 = new Managed(mgr);

	Rooted<Managed> rh1{n1};
	Rooted<Managed> rh2{n2};

	Owned<Managed> h2{n2, n1};

	// Equals operator
	ASSERT_TRUE(rh1 == n1);
	ASSERT_TRUE(n1 == rh1);
	ASSERT_FALSE(rh1 == rh2);
	ASSERT_TRUE(rh2 == h2);
	ASSERT_TRUE(h2 == rh2);

	// Assignment operator
	Rooted<Managed> rh2b;

	ASSERT_FALSE(rh2b == rh2);
	rh2b = rh2;
	ASSERT_TRUE(rh2b == rh2);
	ASSERT_TRUE(rh2b == h2);

	rh2b = h2;
	ASSERT_TRUE(rh2b == h2);

	Owned<Managed> h2b;
	ASSERT_FALSE(rh2 == h2b);
	ASSERT_FALSE(h2 == h2b);
	h2b = h2;
	ASSERT_TRUE(rh2 == h2b);
	ASSERT_TRUE(h2 == h2b);

	Owned<Managed> h2c{h2b, n1};
	ASSERT_TRUE(h2b == h2c);
}

/* Class Manager */

TEST(Manager, linearDependencies)
{
	std::array<bool, 4> a;

	Manager mgr(1);
	{
		TestManaged *n1, *n2, *n3;
		n1 = new TestManaged(mgr, a[1]);
		n2 = new TestManaged(mgr, a[2]);
		n3 = new TestManaged(mgr, a[3]);

		{
			Rooted<TestManaged> hr{new TestManaged(mgr, a[0])};

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

TEST(Manager, cyclicDependencies)
{
	std::array<bool, 4> a;

	Manager mgr(1);
	{
		TestManaged *n1, *n2, *n3;
		n1 = new TestManaged(mgr, a[1]);
		n2 = new TestManaged(mgr, a[2]);
		n3 = new TestManaged(mgr, a[3]);

		{
			Rooted<TestManaged> hr{new TestManaged(mgr, a[0])};

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

TEST(Manager, selfReferentialCyclicDependencies)
{
	std::array<bool, 2> a;

	Manager mgr(1);
	{
		TestManaged *n1;
		n1 = new TestManaged(mgr, a[1]);

		{
			Rooted<TestManaged> hr{new TestManaged(mgr, a[0])};
			ASSERT_TRUE(a[0] && a[1]);
			hr->addRef(n1);
			n1->addRef(n1);
		}

		// All nodes must have set their "alive" flag to false
		ASSERT_FALSE(a[0] || a[1]);
	}
}

TEST(Manager, doubleRooted)
{
	std::array<bool, 4> a;

	Manager mgr(1);
	{
		TestManaged *n1, *n2;
		n1 = new TestManaged(mgr, a[1]);
		n2 = new TestManaged(mgr, a[2]);

		{
			Rooted<TestManaged> hr1{new TestManaged(mgr, a[0])};
			{
				Rooted<TestManaged> hr2{new TestManaged(mgr, a[3])};

				// All nodes must have set their "alive" flag to true
				for (bool v : a) {
					ASSERT_TRUE(v);
				}

				// Reference n1 and n2 in the rooted nodes
				hr1->addRef(n1);
				hr2->addRef(n2);

				// Create cyclical dependency between n2 and n1
				n1->addRef(n2);
				n2->addRef(n1);
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

TEST(Manager, disconnectSubgraph)
{
	std::array<bool, 4> a;

	Manager mgr(1);
	{
		TestManaged *n1, *n2, *n3;
		n1 = new TestManaged(mgr, a[1]);
		n2 = new TestManaged(mgr, a[2]);
		n3 = new TestManaged(mgr, a[3]);

		{
			Rooted<TestManaged> hr{new TestManaged(mgr, a[0])};

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

TEST(Manager, disconnectDoubleRootedSubgraph)
{
	std::array<bool, 5> a;

	Manager mgr(1);
	{
		TestManaged *n1, *n2, *n3;
		n1 = new TestManaged(mgr, a[1]);
		n2 = new TestManaged(mgr, a[2]);
		n3 = new TestManaged(mgr, a[3]);

		{
			Rooted<TestManaged> hr1{new TestManaged(mgr, a[0])};
			{
				Rooted<TestManaged> hr2{new TestManaged(mgr, a[4])};

				// Create a cyclic dependency chain with two rooted nodes
				hr1->addRef(n1);
				n1->addRef(n2);
				n2->addRef(n3);
				n3->addRef(n1);
				hr2->addRef(n3);

				// All nodes must have set their "alive" flag to true
				for (bool v : a) {
					ASSERT_TRUE(v);
				}

				// Remove the reference from n3 to n1
				n3->deleteRef(n1);

				// Still all nodes must have set their "alive" flag to true
				for (bool v : a) {
					ASSERT_TRUE(v);
				}

				// Remove the reference from n1 to n2
				n1->deleteRef(n2);

				// Managed 2 must be dead, all others alive
				ASSERT_FALSE(a[2]);
				ASSERT_TRUE(a[0] && a[1] && a[3] && a[4]);
			}

			// Managed 2, 3, hr2 must be dead, all others alive
			ASSERT_FALSE(a[2] || a[3] || a[4]);
			ASSERT_TRUE(a[0] && a[1]);
		}

		// All nodes must have set their "alive" flag to false
		for (bool v : a) {
			ASSERT_FALSE(v);
		}
	}
}

Rooted<TestManaged> createFullyConnectedGraph(Manager &mgr, int nElem,
                                             bool alive[])
{
	std::vector<Rooted<TestManaged>> nodes;

	// Create the nodes
	for (int i = 0; i < nElem; i++) {
		nodes.push_back(Rooted<TestManaged>{new TestManaged{mgr, alive[i]}});
	}

	// Add all connections
	for (int i = 0; i < nElem; i++) {
		for (int j = 0; j < nElem; j++) {
			nodes[i]->addRef(nodes[j]);
		}
	}

	return nodes[0];
}

TEST(Manager, fullyConnectedGraph)
{
	constexpr int nElem = 64;
	std::array<bool, nElem> a;

	Manager mgr(1);
	{
		Rooted<TestManaged> n = createFullyConnectedGraph(mgr, nElem, &a[0]);
		for (bool v : a) {
			ASSERT_TRUE(v);
		}
	}

	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}

class HidingTestManaged : public TestManaged {

private:
	Rooted<Managed> hidden;

public:

	HidingTestManaged(Manager &mgr, bool &alive) : TestManaged(mgr, alive) {};

	void setHiddenRef(Handle<Managed> t) {
		hidden = t;
	}

};

TEST(Manager, hiddenRootedGraph)
{
	constexpr int nElem = 16;
	std::array<bool, nElem> a;
	bool b;
	Manager mgr(1);

	{
		Rooted<HidingTestManaged> n{new HidingTestManaged{mgr, b}};
		n->setHiddenRef(createFullyConnectedGraph(mgr, nElem, &a[0]));

		ASSERT_TRUE(b);
		for (bool v : a) {
			ASSERT_TRUE(v);
		}
	}

	ASSERT_FALSE(b);
	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}

}

