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

/* Class DoubleType */

TEST(DoubleType, rtti)
{
	Manager mgr;
	Rooted<DoubleType> doubleType{new DoubleType(mgr, nullptr)};
	ASSERT_TRUE(doubleType->isa(RttiTypes::DoubleType));
	ASSERT_TRUE(doubleType->isa(typeOf<Type>()));
	ASSERT_TRUE(doubleType->isa(typeOf<Node>()));
}

TEST(DoubleType, creation)
{
	Manager mgr;
	Rooted<DoubleType> doubleType{new DoubleType(mgr, nullptr)};
	Variant val = doubleType->create();
	ASSERT_TRUE(val.isDouble());
	ASSERT_EQ(0.0, val.asDouble());
}

TEST(DoubleType, conversion)
{
	Logger logger;
	Manager mgr;
	Rooted<DoubleType> doubleType{new DoubleType(mgr, nullptr)};

	{
		Variant val{3.14};
		ASSERT_TRUE(doubleType->build(val, logger));
		ASSERT_TRUE(val.isDouble());
		ASSERT_EQ(3.14, val.asDouble());
	}

	{
		Variant val{314};
		ASSERT_TRUE(doubleType->build(val, logger));
		ASSERT_TRUE(val.isDouble());
		ASSERT_EQ(314.0, val.asDouble());
	}

	{
		Variant val{"1"};
		ASSERT_FALSE(doubleType->build(val, logger));
		ASSERT_TRUE(val.isDouble());
		ASSERT_EQ(0.0, val.asDouble());
	}
}

/* Class BoolType */

TEST(BoolType, rtti)
{
	Manager mgr;
	Rooted<BoolType> boolType{new BoolType(mgr, nullptr)};
	ASSERT_TRUE(boolType->isa(RttiTypes::BoolType));
	ASSERT_TRUE(boolType->isa(typeOf<Type>()));
	ASSERT_TRUE(boolType->isa(typeOf<Node>()));
}

TEST(BoolType, creation)
{
	Manager mgr;
	Rooted<BoolType> boolType{new BoolType(mgr, nullptr)};
	Variant val = boolType->create();
	ASSERT_TRUE(val.isBool());
	ASSERT_FALSE(val.asBool());
}

TEST(BoolType, conversion)
{
	Logger logger;
	Manager mgr;
	Rooted<BoolType> boolType{new BoolType(mgr, nullptr)};

	{
		Variant val{true};
		ASSERT_TRUE(boolType->build(val, logger));
		ASSERT_TRUE(val.isBool());
		ASSERT_TRUE(val.asBool());
	}

	{
		Variant val{false};
		ASSERT_TRUE(boolType->build(val, logger));
		ASSERT_TRUE(val.isBool());
		ASSERT_FALSE(val.asBool());
	}

	{
		Variant val{314};
		ASSERT_FALSE(boolType->build(val, logger));
		ASSERT_TRUE(val.isBool());
		ASSERT_FALSE(val.asBool());
	}
}

/* Class EnumType */

TEST(EnumType, rtti)
{
	Logger logger;
	Manager mgr;
	Rooted<EnumType> enumType{EnumType::createValidated(
	    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};
	ASSERT_TRUE(enumType->isa(RttiTypes::EnumType));
	ASSERT_TRUE(enumType->isa(typeOf<Type>()));
	ASSERT_TRUE(enumType->isa(typeOf<Node>()));
}

TEST(EnumType, creation)
{
	Logger logger;
	Manager mgr;
	Rooted<EnumType> enumType{EnumType::createValidated(
	    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};
	Variant val = enumType->create();
	ASSERT_TRUE(val.isInt());
	ASSERT_EQ(0, val.asInt());
}

TEST(EnumType, conversion)
{
	Logger logger;
	Manager mgr;
	Rooted<EnumType> enumType{EnumType::createValidated(
	    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};

	{
		Variant val{1};
		ASSERT_TRUE(enumType->build(val, logger));
		ASSERT_TRUE(val.isInt());
		ASSERT_EQ(1, val.asInt());
	}

	{
		Variant val;
		val.setMagic("a");
		ASSERT_TRUE(enumType->build(val, logger));
		ASSERT_TRUE(val.isInt());
		ASSERT_EQ(0, val.asInt());
	}

	{
		Variant val;
		val.setMagic("c");
		ASSERT_TRUE(enumType->build(val, logger));
		ASSERT_TRUE(val.isInt());
		ASSERT_EQ(2, val.asInt());
	}

	{
		Variant val{"c"};
		ASSERT_FALSE(enumType->build(val, logger));
		ASSERT_TRUE(val.isInt());
		ASSERT_EQ(0, val.asInt());
	}

	{
		Variant val;
		val.setMagic("d");
		ASSERT_FALSE(enumType->build(val, logger));
		ASSERT_TRUE(val.isInt());
		ASSERT_EQ(0, val.asInt());
	}

	{
		Variant val{5};
		ASSERT_FALSE(enumType->build(val, logger));
		ASSERT_TRUE(val.isInt());
		ASSERT_EQ(0, val.asInt());
	}
}

TEST(EnumType, createValidated)
{
	Manager mgr;

	{
		Logger logger;
		Rooted<EnumType> enumType{EnumType::createValidated(
		    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};
		ASSERT_EQ(Severity::DEBUG, logger.getMaxEncounteredSeverity());
	}

	{
		Logger logger;
		Rooted<EnumType> enumType{EnumType::createValidated(
		    mgr, "enum", nullptr, {"a", "a", "c"}, logger)};
		ASSERT_EQ(Severity::ERROR, logger.getMaxEncounteredSeverity());
	}

	{
		Logger logger;
		Rooted<EnumType> enumType{
		    EnumType::createValidated(mgr, "enum", nullptr, {}, logger)};
		ASSERT_EQ(Severity::ERROR, logger.getMaxEncounteredSeverity());
	}

	{
		Logger logger;
		Rooted<EnumType> enumType{
		    EnumType::createValidated(mgr, "enum", nullptr, {"a a"}, logger)};
		ASSERT_EQ(Severity::ERROR, logger.getMaxEncounteredSeverity());
	}
}

TEST(EnumType, nameOf)
{
	Logger logger;
	Manager mgr;

	Rooted<EnumType> enumType{EnumType::createValidated(
	    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};

	ASSERT_EQ("a", enumType->nameOf(0));
	ASSERT_EQ("b", enumType->nameOf(1));
	ASSERT_EQ("c", enumType->nameOf(2));
	ASSERT_THROW(enumType->nameOf(-1), LoggableException);
	ASSERT_THROW(enumType->nameOf(3), LoggableException);
}

TEST(EnumType, valueOf)
{
	Logger logger;
	Manager mgr;

	Rooted<EnumType> enumType{EnumType::createValidated(
	    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};

	ASSERT_EQ(0, enumType->valueOf("a"));
	ASSERT_EQ(1, enumType->valueOf("b"));
	ASSERT_EQ(2, enumType->valueOf("c"));
	ASSERT_THROW(enumType->valueOf("d"), LoggableException);
	ASSERT_THROW(enumType->valueOf("e"), LoggableException);
}

/* Class StructType */

static Rooted<StructType> createStructType(Manager &mgr, Logger &logger)
{
	Rooted<StringType> stringType{new StringType(mgr, nullptr)};
	Rooted<IntType> intType{new IntType(mgr, nullptr)};
	Rooted<StructType> structType{StructType::createValidated(
	    mgr, "struct", nullptr, nullptr,
	    NodeVector<Attribute>{
	        new Attribute{mgr, "d", stringType, "attr1default"},
	        new Attribute{mgr, "b", stringType},
	        new Attribute{mgr, "c", intType, 3},
	        new Attribute{mgr, "a", intType}},
	    logger)};
	return structType;
}

static Rooted<StructType> createStructTypeWithParent(Handle<StructType> parent,
                                                     Manager &mgr,
                                                     Logger &logger)
{
	Rooted<StringType> stringType{new StringType(mgr, nullptr)};
	Rooted<IntType> intType{new IntType(mgr, nullptr)};
	Rooted<StructType> structType{StructType::createValidated(
	    mgr, "struct", nullptr, parent,
	    NodeVector<Attribute>{new Attribute{mgr, "aa", stringType, "value1"},
	                          new Attribute{mgr, "bc", intType, 42},
	                          new Attribute{mgr, "cd", parent}},
	    logger)};
	return structType;
}

TEST(StructType, rtti)
{
	Logger logger;
	Manager mgr;
	Rooted<StructType> structType = createStructType(mgr, logger);
	ASSERT_TRUE(structType->isa(RttiTypes::StructType));
	ASSERT_TRUE(structType->isa(typeOf<Type>()));
	ASSERT_TRUE(structType->isa(typeOf<Node>()));
}

TEST(StructType, creation)
{
	Logger logger;
	Manager mgr;
	Rooted<StructType> structType = createStructType(mgr, logger);
	Variant val = structType->create();
	ASSERT_TRUE(val.isArray());
	ASSERT_EQ(4U, val.asArray().size());

	const auto &arr = val.asArray();
	ASSERT_TRUE(arr[0].isString());
	ASSERT_TRUE(arr[1].isString());
	ASSERT_TRUE(arr[2].isInt());
	ASSERT_TRUE(arr[3].isInt());

	ASSERT_EQ("attr1default", arr[0].asString());
	ASSERT_EQ("", arr[1].asString());
	ASSERT_EQ(3, arr[2].asInt());
	ASSERT_EQ(0, arr[3].asInt());
}

TEST(StructType, creationWithParent)
{
	Logger logger;
	Manager mgr;
	Rooted<StructType> structType = createStructType(mgr, logger);
	Rooted<StructType> structWithParentType =
	    createStructTypeWithParent(structType, mgr, logger);

	Variant val = structWithParentType->create();
	ASSERT_TRUE(val.isArray());
	ASSERT_EQ(7U, val.asArray().size());

	const auto &arr = val.asArray();
	ASSERT_TRUE(arr[0].isString());
	ASSERT_TRUE(arr[1].isString());
	ASSERT_TRUE(arr[2].isInt());
	ASSERT_TRUE(arr[3].isInt());
	ASSERT_TRUE(arr[4].isString());
	ASSERT_TRUE(arr[5].isInt());
	ASSERT_TRUE(arr[6].isArray());

	ASSERT_EQ("attr1default", arr[0].asString());
	ASSERT_EQ("", arr[1].asString());
	ASSERT_EQ(3, arr[2].asInt());
	ASSERT_EQ(0, arr[3].asInt());
	ASSERT_EQ("value1", arr[4].asString());
	ASSERT_EQ(42, arr[5].asInt());
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

/* Class UnknownType */

TEST(UnknownType, rtti)
{
	Manager mgr;
	Rooted<UnknownType> unknownType{new UnknownType(mgr, "unknown")};
	ASSERT_TRUE(unknownType->isa(RttiTypes::UnknownType));
	ASSERT_TRUE(unknownType->isa(typeOf<Type>()));
	ASSERT_TRUE(unknownType->isa(typeOf<Node>()));
}

TEST(UnknownType, creation)
{
	Manager mgr;
	Rooted<UnknownType> unknownType{new UnknownType(mgr, "unknown")};

	Variant val = unknownType->create();
	ASSERT_TRUE(val.isNull());
}

TEST(UnknownType, conversion)
{
	Logger logger;
	Manager mgr;
	Rooted<UnknownType> unknownType{new UnknownType(mgr, "unknown")};

	{
		Variant val1{{1, "test", false, 42.5}};
		Variant val2 = val1;
		ASSERT_TRUE(unknownType->build(val1, logger));
		ASSERT_EQ(val1, val2);
	}
}
}
}
