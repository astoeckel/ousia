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

#include <core/common/Variant.hpp>
#include <core/managed/Managed.hpp>

#include "TestManaged.hpp"

namespace ousia {

TEST(Variant, simpleManagedObject)
{
	Manager mgr(1);
	bool a = false;
	{
		Handle<TestManaged> p{new TestManaged{mgr, a}};
		Variant v(p);
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(p, v.asObject());
		ASSERT_TRUE(a);
	}
	ASSERT_FALSE(a);
}

TEST(Variant, managedObjectCopy)
{
	Manager mgr(1);
	bool a = false;
	{
		Handle<TestManaged> p{new TestManaged{mgr, a}};
		Variant v1(p);
		{
			Variant v2 = v1;
			ASSERT_TRUE(v2.isObject());
			ASSERT_EQ(p, v2.asObject());
			ASSERT_TRUE(a);
		}
		ASSERT_TRUE(a);
	}
	ASSERT_FALSE(a);
}

TEST(Variant, managedObjectMove)
{
	Manager mgr(1);
	bool a = false;
	{
		Handle<TestManaged> p{new TestManaged{mgr, a}};
		Variant v1(p);
		{
			Variant v2 = std::move(v1);
			ASSERT_TRUE(v2.isObject());
			ASSERT_EQ(p, v2.asObject());
			ASSERT_TRUE(a);
		}
		ASSERT_FALSE(a);
	}
	ASSERT_FALSE(a);
}

}

