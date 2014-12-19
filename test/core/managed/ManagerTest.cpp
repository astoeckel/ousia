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

#include <core/managed/Managed.hpp>

#include "TestManaged.hpp"

namespace ousia {

/* Class ObjectDescriptor */

class TestObjectDescriptor : public Manager::ObjectDescriptor {
public:
	int refInCount() const
	{
		int res = 0;
		for (const auto &e : refIn) {
			res += e.second;
		}
		return res + rootRefCount;
	}

	int refOutCount() const
	{
		int res = 0;
		for (const auto &e : refOut) {
			res += e.second;
		}
		return res;
	}

	int refInCount(Managed *o) const
	{
		if (o == nullptr) {
			return rootRefCount;
		}

		const auto it = refIn.find(o);
		if (it != refIn.cend()) {
			return it->second;
		}
		return 0;
	}

	int refOutCount(Managed *o) const
	{
		const auto it = refOut.find(o);
		if (it != refOut.cend()) {
			return it->second;
		}
		return 0;
	}
};

TEST(ObjectDescriptor, degree)
{
	// Do not use actual Managed in this test -- we don't want to test their
	// behaviour
	TestObjectDescriptor nd;
	Managed *n1 = reinterpret_cast<Managed *>(intptr_t{0x10});
	Managed *n2 = reinterpret_cast<Managed *>(intptr_t{0x20});

	// Input degree
	ASSERT_EQ(0U, nd.refIn.size());
	ASSERT_EQ(0, nd.refInCount(n1));

	nd.incrDegree(Manager::RefDir::IN, n1);
	ASSERT_EQ(1, nd.refInCount());
	ASSERT_EQ(1, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(1U, nd.refIn.size());

	nd.incrDegree(Manager::RefDir::IN, n1);
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(1U, nd.refIn.size());

	nd.incrDegree(Manager::RefDir::IN, n2);
	ASSERT_EQ(3, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2U, nd.refIn.size());

	nd.incrDegree(Manager::RefDir::IN, nullptr);
	ASSERT_EQ(4, nd.refInCount());
	ASSERT_EQ(2, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2U, nd.refIn.size());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::IN, n1));
	ASSERT_EQ(3, nd.refInCount());
	ASSERT_EQ(1, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(2U, nd.refIn.size());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::IN, n1));
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(1, nd.refInCount(n2));
	ASSERT_EQ(1U, nd.refIn.size());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::IN, n2));
	ASSERT_EQ(1, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(0U, nd.refIn.size());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::IN, nullptr));
	ASSERT_EQ(0, nd.refInCount());
	ASSERT_EQ(0, nd.refInCount(n1));
	ASSERT_EQ(0, nd.refInCount(n2));
	ASSERT_EQ(0U, nd.refIn.size());

	// Output degree
	ASSERT_EQ(0U, nd.refOut.size());
	ASSERT_EQ(0, nd.refOutCount(n1));

	nd.incrDegree(Manager::RefDir::OUT, n1);
	ASSERT_EQ(1, nd.refOutCount());
	ASSERT_EQ(1, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(1U, nd.refOut.size());

	nd.incrDegree(Manager::RefDir::OUT, n1);
	ASSERT_EQ(2, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(1U, nd.refOut.size());

	nd.incrDegree(Manager::RefDir::OUT, n2);
	ASSERT_EQ(3, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2U, nd.refOut.size());

	nd.incrDegree(Manager::RefDir::OUT, nullptr);
	ASSERT_EQ(3, nd.refOutCount());
	ASSERT_EQ(2, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2U, nd.refOut.size());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::OUT, n1));
	ASSERT_EQ(2, nd.refOutCount());
	ASSERT_EQ(1, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(2U, nd.refOut.size());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::OUT, n1));
	ASSERT_EQ(1, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(1, nd.refOutCount(n2));
	ASSERT_EQ(1U, nd.refOut.size());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::OUT, n2));
	ASSERT_EQ(0, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(0U, nd.refOut.size());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::OUT, nullptr));
	ASSERT_EQ(0, nd.refOutCount());
	ASSERT_EQ(0, nd.refOutCount(n1));
	ASSERT_EQ(0, nd.refOutCount(n2));
	ASSERT_EQ(0U, nd.refOut.size());
}

TEST(ObjectDescriptor, rootRefCount)
{
	TestObjectDescriptor nd;
	ASSERT_EQ(0, nd.rootRefCount);

	nd.incrDegree(Manager::RefDir::IN, nullptr);
	ASSERT_EQ(1, nd.rootRefCount);

	nd.incrDegree(Manager::RefDir::OUT, nullptr);
	ASSERT_EQ(2, nd.rootRefCount);

	ASSERT_EQ(2, nd.refInCount(nullptr));
	ASSERT_EQ(2, nd.refInCount());
	ASSERT_EQ(0, nd.refOutCount(nullptr));
	ASSERT_EQ(0, nd.refOutCount());

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::OUT, nullptr));
	ASSERT_EQ(1, nd.rootRefCount);

	ASSERT_TRUE(nd.decrDegree(Manager::RefDir::IN, nullptr));
	ASSERT_EQ(0, nd.rootRefCount);

	ASSERT_FALSE(nd.decrDegree(Manager::RefDir::IN, nullptr));
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
	HidingTestManaged(Manager &mgr, bool &alive) : TestManaged(mgr, alive){};

	void setHiddenRef(Handle<Managed> t) { hidden = t; }
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

TEST(Manager, storeData)
{
	Manager mgr(1);

	std::array<bool, 5> a;

	{
		Rooted<TestManaged> n{new TestManaged{mgr, a[0]}};

		mgr.storeData(n.get(), "key1", new TestManaged{mgr, a[1]});

		Managed *m2 = new TestManaged{mgr, a[2]};
		mgr.storeData(n.get(), "key2", m2);

		ASSERT_TRUE(a[0] && a[1] && a[2]);

		ASSERT_TRUE(mgr.deleteData(n.get(), "key1"));
		ASSERT_FALSE(a[1]);
		ASSERT_FALSE(mgr.deleteData(n.get(), "key1"));

		mgr.storeData(n.get(), "key1", new TestManaged{mgr, a[3]});
		ASSERT_TRUE(a[3]);

		Managed *m = new TestManaged{mgr, a[4]};
		mgr.storeData(n.get(), "key1", m);
		ASSERT_FALSE(a[3]);
		ASSERT_TRUE(a[4]);

		ASSERT_EQ(m, mgr.readData(n.get(), "key1"));
		ASSERT_EQ(m2, mgr.readData(n.get(), "key2"));

		auto map = mgr.readData(n.get());
		ASSERT_EQ(2U, map.size());
		ASSERT_TRUE(map.find("key1") != map.end());
		ASSERT_TRUE(map.find("key2") != map.end());
	}

	ASSERT_FALSE(a[0] || a[1] || a[2] || a[3] || a[4]);
}

class TestDeleteOrderManaged : public Managed {
private:
	const int id;
	std::vector<int> &ids;
	std::vector<Owned<Managed>> refs;

public:
	TestDeleteOrderManaged(Manager &mgr, int id, std::vector<int> &ids)
	    : Managed(mgr), id(id), ids(ids)
	{
	}

	~TestDeleteOrderManaged() override { ids.push_back(id); }

	void addRef(Handle<Managed> h) { refs.push_back(acquire(h)); }
};

TEST(Manager, deleteOrder)
{
	std::vector<int> ids;
	{
		Manager mgr;
		{
			Rooted<TestDeleteOrderManaged> root{
				new TestDeleteOrderManaged{mgr, 0, ids}};
			{
				Rooted<TestDeleteOrderManaged> m1{
					new TestDeleteOrderManaged{mgr, 1, ids}};
				Rooted<TestDeleteOrderManaged> m2{
					new TestDeleteOrderManaged{mgr, 2, ids}};
				Rooted<TestDeleteOrderManaged> m3{
					new TestDeleteOrderManaged{mgr, 3, ids}};
				Rooted<TestDeleteOrderManaged> m4{
					new TestDeleteOrderManaged{mgr, 4, ids}};
				Rooted<TestDeleteOrderManaged> m5{
					new TestDeleteOrderManaged{mgr, 5, ids}};
				Rooted<TestDeleteOrderManaged> m6{
					new TestDeleteOrderManaged{mgr, 6, ids}};
				Rooted<TestDeleteOrderManaged> m7{
					new TestDeleteOrderManaged{mgr, 7, ids}};

				root->addRef(m7);
				m7->addRef(m2);
				m2->addRef(m5);
				m5->addRef(m1);
				m1->addRef(m3);
				m3->addRef(m6);
				m6->addRef(m4);
			}
		}
	}

	std::vector<int> expected{0, 7, 2, 5, 1, 3, 6, 4};
	ASSERT_EQ(expected, ids);
}
}

