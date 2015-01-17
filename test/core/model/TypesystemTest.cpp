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

static TerminalLogger logger(std::cerr, true);
//static ConcreteLogger logger;

/* Class StringType */

TEST(StringType, rtti)
{
	Manager mgr;
	Rooted<StringType> strType{new StringType(mgr, nullptr)};
	ASSERT_TRUE(strType->isa(typeOf<Type>()));
	ASSERT_TRUE(strType->isa(typeOf<Node>()));
	ASSERT_TRUE(strType->isa(RttiTypes::StringType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::Type));
	ASSERT_FALSE(strType->composedOf(RttiTypes::StringType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::IntType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::DoubleType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::BoolType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::EnumType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::UnknownType));
	ASSERT_FALSE(strType->composedOf(RttiTypes::Constant));
	ASSERT_FALSE(strType->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(strType->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(strType->composedOf(RttiTypes::SystemTypesystem));
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
	ASSERT_FALSE(intType->composedOf(RttiTypes::Type));
	ASSERT_FALSE(intType->composedOf(RttiTypes::StringType));
	ASSERT_FALSE(intType->composedOf(RttiTypes::IntType));
	ASSERT_FALSE(intType->composedOf(RttiTypes::DoubleType));
	ASSERT_FALSE(intType->composedOf(RttiTypes::BoolType));
	ASSERT_FALSE(intType->composedOf(RttiTypes::EnumType));
	ASSERT_FALSE(intType->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(intType->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(intType->composedOf(RttiTypes::UnknownType));
	ASSERT_FALSE(intType->composedOf(RttiTypes::Constant));
	ASSERT_FALSE(intType->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(intType->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(intType->composedOf(RttiTypes::SystemTypesystem));
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
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::Type));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::StringType));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::IntType));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::DoubleType));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::BoolType));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::EnumType));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::UnknownType));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::Constant));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(doubleType->composedOf(RttiTypes::SystemTypesystem));
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
	ASSERT_FALSE(boolType->composedOf(RttiTypes::Type));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::StringType));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::IntType));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::DoubleType));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::BoolType));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::EnumType));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::UnknownType));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::Constant));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(boolType->composedOf(RttiTypes::SystemTypesystem));
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
	Manager mgr;
	Rooted<EnumType> enumType{EnumType::createValidated(
	    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};
	ASSERT_TRUE(enumType->isa(RttiTypes::EnumType));
	ASSERT_TRUE(enumType->isa(typeOf<Type>()));
	ASSERT_TRUE(enumType->isa(typeOf<Node>()));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::Type));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::StringType));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::IntType));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::DoubleType));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::BoolType));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::EnumType));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::UnknownType));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::Constant));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(enumType->composedOf(RttiTypes::SystemTypesystem));
}

TEST(EnumType, creation)
{
	Manager mgr;
	Rooted<EnumType> enumType{EnumType::createValidated(
	    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};
	Variant val = enumType->create();
	ASSERT_TRUE(val.isInt());
	ASSERT_EQ(0, val.asInt());
}

TEST(EnumType, conversion)
{
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
		logger.reset();
		Rooted<EnumType> enumType{EnumType::createValidated(
		    mgr, "enum", nullptr, {"a", "b", "c"}, logger)};
		ASSERT_EQ(Severity::DEBUG, logger.getMaxEncounteredSeverity());
	}

	{
		logger.reset();
		Rooted<EnumType> enumType{EnumType::createValidated(
		    mgr, "enum", nullptr, {"a", "a", "c"}, logger)};
		ASSERT_EQ(Severity::ERROR, logger.getMaxEncounteredSeverity());
	}

	{
		logger.reset();
		Rooted<EnumType> enumType{
		    EnumType::createValidated(mgr, "enum", nullptr, {}, logger)};
		ASSERT_EQ(Severity::ERROR, logger.getMaxEncounteredSeverity());
	}

	{
		logger.reset();
		Rooted<EnumType> enumType{
		    EnumType::createValidated(mgr, "enum", nullptr, {"a a"}, logger)};
		ASSERT_EQ(Severity::ERROR, logger.getMaxEncounteredSeverity());
	}
}

TEST(EnumType, nameOf)
{
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
	    mgr, "struct2", nullptr, parent,
	    NodeVector<Attribute>{new Attribute{mgr, "aa", stringType, "value1"},
	                          new Attribute{mgr, "bc", intType, 42},
	                          new Attribute{mgr, "cd", parent}},
	    logger)};
	return structType;
}

TEST(StructType, rtti)
{
	Manager mgr;
	Rooted<StructType> structType = createStructType(mgr, logger);
	ASSERT_TRUE(structType->isa(RttiTypes::StructType));
	ASSERT_TRUE(structType->isa(typeOf<Type>()));
	ASSERT_TRUE(structType->isa(typeOf<Node>()));
	ASSERT_FALSE(structType->composedOf(RttiTypes::Type));
	ASSERT_FALSE(structType->composedOf(RttiTypes::StringType));
	ASSERT_FALSE(structType->composedOf(RttiTypes::IntType));
	ASSERT_FALSE(structType->composedOf(RttiTypes::DoubleType));
	ASSERT_FALSE(structType->composedOf(RttiTypes::BoolType));
	ASSERT_FALSE(structType->composedOf(RttiTypes::EnumType));
	ASSERT_FALSE(structType->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(structType->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(structType->composedOf(RttiTypes::UnknownType));
	ASSERT_FALSE(structType->composedOf(RttiTypes::Constant));
	ASSERT_TRUE(structType->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(structType->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(structType->composedOf(RttiTypes::SystemTypesystem));
}

TEST(StructType, creation)
{
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
	Manager mgr{1};
	Rooted<StructType> structType = createStructType(mgr, logger);
	{
	Rooted<StructType> structWithParentType =
	    createStructTypeWithParent(structType, mgr, logger);
#ifdef MANAGER_GRAPHVIZ_EXPORT
	mgr.exportGraphviz("structTypeTest1.dot");
#endif

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
#ifdef MANAGER_GRAPHVIZ_EXPORT
	mgr.exportGraphviz("structTypeTest2.dot");
#endif
}

TEST(StructType, derivedFrom)
{
	Manager mgr;
	Rooted<StructType> structType = createStructType(mgr, logger);
	Rooted<StructType> structWithParentType =
	    createStructTypeWithParent(structType, mgr, logger);

	ASSERT_TRUE(structType->derivedFrom(structType));
	ASSERT_TRUE(structWithParentType->derivedFrom(structType));
	ASSERT_TRUE(structWithParentType->derivedFrom(structWithParentType));
	ASSERT_FALSE(structType->derivedFrom(structWithParentType));
}

TEST(StructType, createValidated)
{
	Manager mgr;
	Rooted<StringType> stringType{new StringType(mgr, nullptr)};
	Rooted<IntType> intType{new IntType(mgr, nullptr)};

	{
		logger.reset();
		Rooted<StructType> structType{StructType::createValidated(
			mgr, "struct", nullptr, nullptr,
			NodeVector<Attribute>{
			    new Attribute{mgr, "d", stringType, "attr1default"},
			    new Attribute{mgr, "b", stringType},
			    new Attribute{mgr, "c", intType, 3},
			    new Attribute{mgr, "a", intType}},
			logger)};
		ASSERT_TRUE(structType->validate(logger));
		ASSERT_EQ(Severity::DEBUG, logger.getMaxEncounteredSeverity());
	}

	{
		logger.reset();
		Rooted<StructType> structType{StructType::createValidated(
			mgr, "struct", nullptr, nullptr,
			NodeVector<Attribute>{
				new Attribute{mgr, "d", stringType, "attr1default"},
				new Attribute{mgr, "b", stringType},
				new Attribute{mgr, "a", intType, 3},
				new Attribute{mgr, "a", intType}},
			logger)};
		ASSERT_FALSE(structType->validate(logger));
		ASSERT_EQ(Severity::ERROR, logger.getMaxEncounteredSeverity());
	}

	{
		logger.reset();
		Rooted<StructType> structType{StructType::createValidated(
			mgr, "struct", nullptr, nullptr,
			NodeVector<Attribute>{
			    new Attribute{mgr, "d", stringType, "attr1default"},
			    new Attribute{mgr, "b", stringType},
			    new Attribute{mgr, "a", intType, 3},
			    new Attribute{mgr, "a a", intType}},
			logger)};
		ASSERT_FALSE(structType->validate(logger));
		ASSERT_EQ(Severity::ERROR, logger.getMaxEncounteredSeverity());
	}
}

TEST(StructType, cast)
{
	Manager mgr;
	Rooted<StructType> structType = createStructType(mgr, logger);
	Rooted<StructType> structWithParentType =
	    createStructTypeWithParent(structType, mgr, logger);

	Variant val = structWithParentType->create();

	ASSERT_TRUE(structType->cast(val, logger));
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

TEST(StructType, indexOf)
{
	Manager mgr;
	Rooted<StructType> structType = createStructType(mgr, logger);
	ASSERT_EQ(0, structType->indexOf("d"));
	ASSERT_EQ(1, structType->indexOf("b"));
	ASSERT_EQ(2, structType->indexOf("c"));
	ASSERT_EQ(3, structType->indexOf("a"));
	ASSERT_EQ(-1, structType->indexOf("#0"));
}

TEST(StructType, build)
{
	Manager mgr;
	Rooted<StructType> structType = createStructType(mgr, logger);

	// All mandatory attributes given as map
	{
		Variant var{{{"b", 42}, {"a", 5}}};
		ASSERT_TRUE(structType->build(var, logger));

		const auto &arr = var.asArray();
		ASSERT_EQ(4U, arr.size());
		ASSERT_EQ("attr1default", arr[0].asString());
		ASSERT_EQ("42", arr[1].asString());
		ASSERT_EQ(3, arr[2].asInt());
		ASSERT_EQ(5, arr[3].asInt());
	}


	// All mandatory attributes given as array
	{
		Variant var{{"v1", 2, 3, 4}};
		ASSERT_TRUE(structType->build(var, logger));

		const auto &arr = var.asArray();
		ASSERT_EQ(4U, arr.size());
		ASSERT_EQ("v1", arr[0].asString());
		ASSERT_EQ("2", arr[1].asString());
		ASSERT_EQ(3, arr[2].asInt());
		ASSERT_EQ(4, arr[3].asInt());
	}

	// Too few attributes
	{
		Variant var{Variant::mapType{{"a", 5}}};
		ASSERT_FALSE(structType->build(var, logger));

		const auto &arr = var.asArray();
		ASSERT_EQ(4U, arr.size());
		ASSERT_EQ("attr1default", arr[0].asString());
		ASSERT_EQ("", arr[1].asString());
		ASSERT_EQ(3, arr[2].asInt());
		ASSERT_EQ(5, arr[3].asInt());
	}

	// Too few attributes
	{
		Variant var{Variant::arrayType{}};
		ASSERT_FALSE(structType->build(var, logger));

		const auto &arr = var.asArray();
		ASSERT_EQ(4U, arr.size());
		ASSERT_EQ("attr1default", arr[0].asString());
		ASSERT_EQ("", arr[1].asString());
		ASSERT_EQ(3, arr[2].asInt());
		ASSERT_EQ(0, arr[3].asInt());
	}

	// Too few attributes
	{
		Variant var{{"v1", 2}};
		ASSERT_FALSE(structType->build(var, logger));

		const auto &arr = var.asArray();
		ASSERT_EQ(4U, arr.size());
		ASSERT_EQ("v1", arr[0].asString());
		ASSERT_EQ("2", arr[1].asString());
		ASSERT_EQ(3, arr[2].asInt());
		ASSERT_EQ(0, arr[3].asInt());
	}

	{
		Variant var{{{"b", 42}, {"#3", 5}, {"#0", "foo"}}};
		ASSERT_TRUE(structType->build(var, logger));

		const auto &arr = var.asArray();
		ASSERT_EQ(4U, arr.size());
		ASSERT_EQ("foo", arr[0].asString());
		ASSERT_EQ("42", arr[1].asString());
		ASSERT_EQ(3, arr[2].asInt());
		ASSERT_EQ(5, arr[3].asInt());
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
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::Type));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::StringType));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::IntType));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::DoubleType));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::BoolType));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::EnumType));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::UnknownType));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::Constant));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(arrayType->composedOf(RttiTypes::SystemTypesystem));
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
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::Type));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::StringType));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::IntType));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::DoubleType));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::BoolType));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::EnumType));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::UnknownType));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::Constant));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(unknownType->composedOf(RttiTypes::SystemTypesystem));
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
	Manager mgr;
	Rooted<UnknownType> unknownType{new UnknownType(mgr, "unknown")};

	{
		Variant val1{{1, "test", false, 42.5}};
		Variant val2 = val1;
		ASSERT_TRUE(unknownType->build(val1, logger));
		ASSERT_EQ(val1, val2);
	}
}

/* Class Typesystem */

TEST(Typesystem, rtti)
{
	Manager mgr{1};
	Rooted<Typesystem> typesystem{new Typesystem{mgr, "typesystem"}};
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::Type));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::StringType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::IntType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::DoubleType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::BoolType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::EnumType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(typesystem->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(typesystem->composedOf(RttiTypes::UnknownType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::Constant));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(typesystem->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(typesystem->composedOf(RttiTypes::SystemTypesystem));
}

/* Class SystemTypesystem */

TEST(SystemTypesystem, rtti)
{
	Manager mgr{1};
	Rooted<SystemTypesystem> typesystem{new SystemTypesystem{mgr}};
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::Type));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::StringType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::IntType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::DoubleType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::BoolType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::EnumType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::StructType));
	ASSERT_FALSE(typesystem->composedOf(RttiTypes::ArrayType));
	ASSERT_FALSE(typesystem->composedOf(RttiTypes::UnknownType));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::Constant));
	ASSERT_TRUE(typesystem->composedOf(RttiTypes::Attribute));
	ASSERT_FALSE(typesystem->composedOf(RttiTypes::Typesystem));
	ASSERT_FALSE(typesystem->composedOf(RttiTypes::SystemTypesystem));
}


}
}
