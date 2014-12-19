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

#include <core/model/Typesystem.hpp>

namespace ousia {
namespace model {

TEST(Type, rtti)
{
	Manager mgr(1);
	Rooted<StringType> strType{new StringType(mgr, nullptr)};

	ASSERT_TRUE(typeOf(*strType).isa(typeOf<Type>()));
}

TEST(StringType, creation)
{
	Manager mgr(1);
	Rooted<StringType> strType{new StringType(mgr, nullptr)};

	Variant val = strType->create();
	ASSERT_TRUE(val.isString());
	ASSERT_EQ("", val.asString());
}

TEST(StringType, conversion)
{
	Logger logger;
	Manager mgr(1);
	Rooted<StringType> strType{new StringType(mgr, nullptr)};

	{
		Variant val{42};
		ASSERT_TRUE(strType->build(val, logger));
		ASSERT_TRUE(val.isString());
		ASSERT_EQ("42", val.asString());
	}

	{
		Variant val{42.5};
		ASSERT_TRUE(strType->build(val, logger));
		ASSERT_TRUE(val.isString());
		ASSERT_EQ("42.5", val.asString());
	}

	{
		Variant val{true};
		ASSERT_TRUE(strType->build(val, logger));
		ASSERT_TRUE(val.isString());
		ASSERT_EQ("true", val.asString());
	}

	{
		Variant val{false};
		ASSERT_TRUE(strType->build(val, logger));
		ASSERT_TRUE(val.isString());
		ASSERT_EQ("false", val.asString());
	}

	{
		Variant val{nullptr};
		ASSERT_TRUE(strType->build(val, logger));
		ASSERT_TRUE(val.isString());
		ASSERT_EQ("null", val.asString());
	}

	{
		Variant val{"test"};
		ASSERT_TRUE(strType->build(val, logger));
		ASSERT_TRUE(val.isString());
		ASSERT_EQ("test", val.asString());
	}

	{
		Variant val{{1, 2, true, false}};
		ASSERT_FALSE(strType->build(val, logger));
		ASSERT_TRUE(val.isString());
		ASSERT_EQ("", val.asString());
	}

}

}
}
