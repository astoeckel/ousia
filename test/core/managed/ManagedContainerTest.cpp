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

#include <gtest/gtest.h>

#include <core/managed/Managed.hpp>
#include <core/managed/ManagedContainer.hpp>

#include "TestManaged.hpp"

namespace ousia {

TEST(ManagedVector, managedVector)
{
	// TODO: This test is highly incomplete

	constexpr int nElem = 16;
	std::array<bool, nElem> a;

	Manager mgr(1);
	{
		Rooted<Managed> root{new Managed{mgr}};

		std::vector<TestManaged*> elems;
		for (int i = 0; i < nElem; i++) {
			elems.push_back(new TestManaged{mgr, a[i]});
		}

		for (bool v : a) {
			ASSERT_TRUE(v);
		}

		ManagedVector<TestManaged> v(root, elems.begin(), elems.end());

		// Remove the last element from the list. It should be garbage collected.
		v.pop_back();
		ASSERT_FALSE(a[nElem - 1]);

		// Insert a new element into the list.
		v.push_back(new TestManaged{mgr, a[nElem - 1]});
		ASSERT_TRUE(a[nElem - 1]);

		// Erase element 10
		{
			auto it = v.find(elems[10]);
			ASSERT_TRUE(it != v.end());
			v.erase(it);
			ASSERT_FALSE(a[10]);
		}

		// Erase element 3 - 5
		v.erase(v.find(elems[3]), v.find(elems[5]));
		ASSERT_FALSE(a[3] || a[4]);
		ASSERT_TRUE(a[5]);

		{
			// Copy the managed vector to another managed vector
			ManagedVector<TestManaged> v2(v);
			v2.push_back(new TestManaged{mgr, a[3]});
			ASSERT_TRUE(a[3]);
		}
		ASSERT_FALSE(a[3]);
		ASSERT_TRUE(a[5]);
	}

	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}


TEST(ManagedVector, moveAssignment)
{
	constexpr int nElem = 16;
	std::array<bool, nElem> a;

	Manager mgr(1);
	{
		Rooted<Managed> root{new Managed{mgr}};
		ManagedVector<TestManaged> v1(root);
		{
			ManagedVector<TestManaged> v2(root);

			for (int i = 0; i < nElem; i++) {
				v2.push_back(new TestManaged{mgr, a[i]});
			}
			for (bool v : a) {
				ASSERT_TRUE(v);
			}

			v1 = std::move(v2);
			ASSERT_EQ(nullptr, v2.getOwner());
		}
		for (bool v : a) {
			ASSERT_TRUE(v);
		}
	}

	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}

TEST(ManagedVector, copyAssignment)
{
	constexpr int nElem = 16;
	std::array<bool, nElem> a;

	Manager mgr(1);
	{
		Rooted<Managed> root{new Managed{mgr}};
		ManagedVector<TestManaged> v1(root);
		{
			ManagedVector<TestManaged> v2(root);

			for (int i = 0; i < nElem; i++) {
				v2.push_back(new TestManaged{mgr, a[i]});
			}
			for (bool v : a) {
				ASSERT_TRUE(v);
			}

			v1 = v2;
			ASSERT_TRUE(v1 == v2);
		}
		for (bool v : a) {
			ASSERT_TRUE(v);
		}
	}

	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}

TEST(ManagedVector, copyWithNewOwner)
{
	constexpr int nElem = 16;
	std::array<bool, nElem> a;

	Manager mgr(1);
	{
		Rooted<Managed> root{new Managed{mgr}};
		ManagedVector<TestManaged> v1(root);
		{
			Rooted<Managed> root2{new Managed{mgr}};
			ManagedVector<TestManaged> v2(root2);

			for (int i = 0; i < nElem; i++) {
				v2.push_back(new TestManaged{mgr, a[i]});
			}
			for (bool v : a) {
				ASSERT_TRUE(v);
			}

			ManagedVector<TestManaged> v3{root, v2};
			v1 = std::move(v3);
			ASSERT_EQ(nullptr, v3.getOwner());
			ASSERT_TRUE(v1 != v2);
		}
		for (bool v : a) {
			ASSERT_TRUE(v);
		}
	}

	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}

TEST(ManagedVector, moveWithNewOwner)
{
	constexpr int nElem = 16;
	std::array<bool, nElem> a;

	Manager mgr(1);
	{
		Rooted<Managed> root{new Managed{mgr}};
		ManagedVector<TestManaged> v1(root);
		{
			Rooted<Managed> root2{new Managed{mgr}};
			ManagedVector<TestManaged> v2(root2);

			for (int i = 0; i < nElem; i++) {
				v2.push_back(new TestManaged{mgr, a[i]});
			}
			for (bool v : a) {
				ASSERT_TRUE(v);
			}

			ManagedVector<TestManaged> v3{root, std::move(v2)};
			v1 = std::move(v3);
			ASSERT_EQ(nullptr, v2.getOwner());
			ASSERT_EQ(nullptr, v3.getOwner());
		}
		for (bool v : a) {
			ASSERT_TRUE(v);
		}
	}

	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}

class TestManagedWithContainer : public Managed {

public:
	ManagedVector<TestManaged> elems;

	TestManagedWithContainer(Manager &mgr) : Managed(mgr), elems(this) {};

};

TEST(ManagedVector, embedded) {
	// Note: This test depends on the correct deletion order -- otherwise
	// valgrind shows an error
	bool a;
	Manager mgr(1);
	{
		Rooted<TestManagedWithContainer> a1{new TestManagedWithContainer(mgr)};
		{
			Rooted<TestManaged> a2{new TestManaged(mgr, a)};

			ASSERT_TRUE(a);

			a1->elems.push_back(a2);
		}
		ASSERT_TRUE(a);
	}
	ASSERT_FALSE(a);
}


TEST(ManagedMap, managedMap)
{
	// TODO: This test is highly incomplete

	constexpr int nElem = 16;
	std::array<bool, nElem> a;

	Manager mgr(1);
	{
		Rooted<Managed> root{new Managed{mgr}};

		std::map<int, TestManaged*> elems;
		for (int i = 0; i < nElem; i++) {
			elems.insert(std::make_pair(i, new TestManaged{mgr, a[i]}));
		}

		for (bool v : a) {
			ASSERT_TRUE(v);
		}

		ManagedMap<int, TestManaged> m(root, elems.begin(), elems.end());

		// Remove the entry with the number 4
		m.erase(m.find(10));
		ASSERT_FALSE(a[10]);

		// Insert a new element
		m.insert(std::make_pair(nElem + 1, new TestManaged{mgr, a[10]}));
		ASSERT_TRUE(a[10]);

		// Erase element 3 - 5
		m.erase(m.find(3), m.find(5));
		ASSERT_FALSE(a[3] || a[4]);
		ASSERT_TRUE(a[5]);

		{
			// Copy the managed map to another managed map vector
			ManagedMap<int, TestManaged> m2(m);
			m2.insert(std::make_pair(3, new TestManaged{mgr, a[3]}));
			ASSERT_TRUE(a[3]);
		}
		ASSERT_FALSE(a[3]);
		ASSERT_TRUE(a[5]);
	}

	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}

}

