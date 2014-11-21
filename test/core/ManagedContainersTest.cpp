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

#include <core/Managed.hpp>
#include <core/ManagedContainers.hpp>

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

		ManagedVector<TestManaged> v(root, elems);

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
	}

	for (bool v : a) {
		ASSERT_FALSE(v);
	}
}

}

