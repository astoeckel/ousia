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

#include <core/common/Rtti.hpp>
#include <core/model/Typesystem.hpp>

namespace ousia {
namespace model {

/* Class StringType */

TEST(StringType, rtti)
{
	Manager mgr;
	Rooted<StringType> strType{new StringType(mgr, nullptr)};
	ASSERT_TRUE(strType->isa(typeOf<Type>()));
	ASSERT_TRUE(strType->isa(typeOf<Node>()));
	ASSERT_TRUE(strType->isa(RttiTypes::StringType));
}

TEST(StringType, creation)
{
	Manager mgr;
	Rooted<StringType> strType{new StringType(mgr, nullptr)};

	Variant val = strType->create();
	ASSERT_TRUE(val.isString());
	ASSERT_EQ("", val.asString());
}

TEST(StringType, conversion)
{
	Logger logger;
	Manager mgr;
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

/* Class IntType */

TEST(IntType, rtti)
{
	Manager mgr;
	Rooted<IntType> intType{new IntType(mgr, nullptr)};
	ASSERT_TRUE(intType->isa(RttiTypes::IntType));
	ASSERT_TRUE(intType->isa(typeOf<Type>()));
	ASSERT_TRUE(intType->isa(typeOf<Node>()));
}

TEST(IntType, creation)
{
	Manager mgr;
	Rooted<IntType> intType{new IntType(mgr, nullptr)};
	Variant val = intType->create();
	ASSERT_TRUE(val.isInt());
	ASSERT_EQ(0, val.asInt());
}

TEST(IntType, conversion)
{
	Logger logger;
	Manager mgr;
	Rooted<IntType> intType{new IntType(mgr, nullptr)};

	{
		Variant val{314};
		ASSERT_TRUE(intType->build(val, logger));
		ASSERT_TRUE(val.isInt());
		ASSERT_EQ(314, val.asInt());
	}

	{
		Variant val{"1"};
		ASSERT_FALSE(intType->build(val, logger));
		ASSERT_TRUE(val.isInt());
		ASSERT_EQ(0, val.asInt());
	}
}

/* Class ArrayType */

TEST(ArrayType, rtti)
{
	Manager mgr;
	Rooted<StringType> stringType{new StringType(mgr, nullptr)};
	Rooted<ArrayType> arrayType{new ArrayType(mgr, stringType)};
	ASSERT_TRUE(arrayType->isa(RttiTypes::ArrayType));
	ASSERT_TRUE(arrayType->isa(typeOf<Type>()));
	ASSERT_TRUE(arrayType->isa(typeOf<Node>()));
}

TEST(ArrayType, creation)
{
	Manager mgr;
	Rooted<StringType> stringType{new StringType(mgr, nullptr)};
	Rooted<ArrayType> arrayType{new ArrayType(mgr, stringType)};

	Variant val = arrayType->create();
	ASSERT_TRUE(val.isArray());
	ASSERT_TRUE(val.asArray().empty());
}

TEST(ArrayType, conversion)
{
	Logger logger;
	Manager mgr;
	Rooted<StringType> stringType{new StringType(mgr, nullptr)};
	Rooted<ArrayType> arrayType{new ArrayType(mgr, stringType)};

	{
		Variant val{{1, "test", false, 42.5}};
		ASSERT_TRUE(arrayType->build(val, logger));
		ASSERT_TRUE(val.isArray());

		auto arr = val.asArray();
		ASSERT_EQ(4U, arr.size());
		for (size_t i = 0; i < arr.size(); i++) {
			ASSERT_TRUE(arr[i].isString());
		}
		ASSERT_EQ("1", arr[0].asString());
		ASSERT_EQ("test", arr[1].asString());
		ASSERT_EQ("false", arr[2].asString());
		ASSERT_EQ("42.5", arr[3].asString());
	}

	{
		Variant val{1};
		ASSERT_FALSE(arrayType->build(val, logger));
		ASSERT_TRUE(val.isArray());
		ASSERT_TRUE(val.asArray().empty());
	}
}

}
}
