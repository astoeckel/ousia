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

	Rooted<Managed> m1{new Managed{mgr}};
	n->storeData("info", m1);
	ASSERT_TRUE(n->hasDataKey("info"));
	ASSERT_FALSE(n->hasDataKey("test"));

	Rooted<Managed> m2{new Managed{mgr}};
	n->storeData("test", m2);
	ASSERT_TRUE(n->hasDataKey("info"));
	ASSERT_TRUE(n->hasDataKey("test"));

	ASSERT_TRUE(n->deleteData("info"));
	ASSERT_FALSE(n->deleteData("info"));
	ASSERT_FALSE(n->hasDataKey("info"));
	ASSERT_TRUE(n->hasDataKey("test"));

	n->storeData("info2", m1);

	std::map<std::string, Rooted<Managed>> m = n->readData();
	ASSERT_TRUE(m.find("info2") != m.end());
	ASSERT_TRUE(m.find("test") != m.end());

	ASSERT_EQ(m1, m.find("info2")->second);
	ASSERT_EQ(m2, m.find("test")->second);
}

class TypeTestManaged1 : public Managed {
	using Managed::Managed;
};

class TypeTestManaged2 : public Managed {
	using Managed::Managed;
};

class TypeTestManaged3 : public Managed {
	using Managed::Managed;
};

class TypeTestManaged4 : public Managed {
	using Managed::Managed;
};

class TypeTestManaged5 : public Managed {
	using Managed::Managed;
};

static const Rtti<TypeTestManaged1> Type1("Type1");
static const Rtti<TypeTestManaged2> Type2("Type2");
static const Rtti<TypeTestManaged3> Type3("Type3", {&Type1});
static const Rtti<TypeTestManaged4> Type4("Type4", {&Type3, &Type2});

TEST(Managed, type)
{
	Manager mgr(1);

	Rooted<TypeTestManaged1> m1{new TypeTestManaged1(mgr)};
	Rooted<TypeTestManaged2> m2{new TypeTestManaged2(mgr)};
	Rooted<TypeTestManaged3> m3{new TypeTestManaged3(mgr)};
	Rooted<TypeTestManaged4> m4{new TypeTestManaged4(mgr)};
	Rooted<TypeTestManaged5> m5{new TypeTestManaged5(mgr)};

	ASSERT_EQ(&Type1, &m1->type());
	ASSERT_EQ(&Type2, &m2->type());
	ASSERT_EQ(&Type3, &m3->type());
	ASSERT_EQ(&Type4, &m4->type());
	ASSERT_EQ(&RttiTypes::None, &m5->type());

	ASSERT_EQ(&Type1, &typeOf<TypeTestManaged1>());
	ASSERT_EQ(&Type1, &typeOf(*m1));
}
}

