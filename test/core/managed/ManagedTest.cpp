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

#include <core/managed/Managed.hpp>

#include "TestManaged.hpp"

namespace ousia {

TEST(Managed, data)
{
	Manager mgr{1};

	Rooted<Managed> n{new Managed{mgr}};

	Managed *m1 = new Managed{mgr};
	n->storeData("info", m1);
	ASSERT_TRUE(n->hasDataKey("info"));
	ASSERT_FALSE(n->hasDataKey("test"));

	Managed *m2 = new Managed{mgr};
	n->storeData("test", m2);
	ASSERT_TRUE(n->hasDataKey("info"));
	ASSERT_TRUE(n->hasDataKey("test"));

	ASSERT_TRUE(n->deleteData("info"));
	ASSERT_FALSE(n->deleteData("info"));
	ASSERT_FALSE(n->hasDataKey("info"));
	ASSERT_TRUE(n->hasDataKey("test"));
}

}

