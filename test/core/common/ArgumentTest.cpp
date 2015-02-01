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

#include <iostream>

#include <gtest/gtest.h>

#include <core/common/Argument.hpp>
#include <core/common/Function.hpp>
#include <core/common/RttiBuilder.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/managed/Managed.hpp>

namespace ousia {

// static Logger logger;
static TerminalLogger logger(std::cerr, true);

namespace {

class TestManaged1 : public Managed {
public:
	using Managed::Managed;
};

class TestManaged2 : public TestManaged1 {
public:
	using TestManaged1::TestManaged1;
};
}

namespace RttiTypes {
static const Rtti TestManaged1 =
    RttiBuilder<ousia::TestManaged1>("TestManaged1");
static const Rtti TestManaged2 =
    RttiBuilder<ousia::TestManaged2>("TestManaged2").parent(&TestManaged1);
}

TEST(Argument, validateAny)
{
	Argument a = Argument::Any("a");

	ASSERT_FALSE(a.hasDefault());

	{
		Variant v{true};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isBool());
		ASSERT_TRUE(v.asBool());
	}

	{
		Variant v{"test"};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("test", v.asString());
	}

	{
		Variant v{{1, 2, 3, 4}};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(Variant::arrayType({1, 2, 3, 4}), v.asArray());
	}
}

TEST(Argument, validateAnyDefault)
{
	Argument a = Argument::Any("a", true);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_TRUE(a.getDefaultValue().asBool());

	{
		Variant v{true};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isBool());
		ASSERT_TRUE(v.asBool());
	}

	{
		Variant v{"test"};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("test", v.asString());
	}

	{
		Variant v{{1, 2, 3, 4}};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(Variant::arrayType({1, 2, 3, 4}), v.asArray());
	}
}

TEST(Argument, validateBool)
{
	Argument a = Argument::Bool("a");

	ASSERT_FALSE(a.hasDefault());

	{
		Variant v{true};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isBool());
		ASSERT_TRUE(v.asBool());
	}

	{
		Variant v{false};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isBool());
		ASSERT_FALSE(v.asBool());
	}

	{
		Variant v{1};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isBool());
		ASSERT_FALSE(v.asBool());
	}
}

TEST(Argument, validateBoolDefault)
{
	Argument a = Argument::Bool("a", true);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_TRUE(a.getDefaultValue().asBool());

	{
		Variant v{true};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isBool());
		ASSERT_TRUE(v.asBool());
	}

	{
		Variant v{false};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isBool());
		ASSERT_FALSE(v.asBool());
	}

	{
		Variant v{1};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isBool());
		ASSERT_TRUE(v.asBool());
	}
}

TEST(Argument, validateInt)
{
	Argument a = Argument::Int("a");

	ASSERT_FALSE(a.hasDefault());

	{
		Variant v{123};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isInt());
		ASSERT_EQ(123, v.asInt());
	}

	{
		Variant v{1.1};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isInt());
		ASSERT_EQ(0, v.asInt());
	}
}

TEST(Argument, validateIntDefault)
{
	Argument a = Argument::Int("a", 42);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_EQ(42, a.getDefaultValue().asInt());

	{
		Variant v{123};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isInt());
		ASSERT_EQ(123, v.asInt());
	}

	{
		Variant v{1.1};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isInt());
		ASSERT_EQ(42, v.asInt());
	}
}

TEST(Argument, validateDouble)
{
	Argument a = Argument::Double("a");

	ASSERT_FALSE(a.hasDefault());

	{
		Variant v{123};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isDouble());
		ASSERT_EQ(123.0, v.asDouble());
	}

	{
		Variant v{1.1};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isDouble());
		ASSERT_EQ(1.1, v.asDouble());
	}

	{
		Variant v{"1.0"};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isDouble());
		ASSERT_EQ(0.0, v.asDouble());
	}
}

TEST(Argument, validateDoubleDefault)
{
	Argument a = Argument::Double("a", 42.0);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_EQ(42.0, a.getDefaultValue().asDouble());

	{
		Variant v{123};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isDouble());
		ASSERT_EQ(123.0, v.asDouble());
	}

	{
		Variant v{1.1};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isDouble());
		ASSERT_EQ(1.1, v.asDouble());
	}

	{
		Variant v{"1.0"};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isDouble());
		ASSERT_EQ(42.0, v.asDouble());
	}
}

TEST(Argument, validateString)
{
	Argument a = Argument::String("a");

	ASSERT_FALSE(a.hasDefault());

	{
		Variant v{"test"};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("test", v.asString());
	}

	{
		Variant v{true};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("true", v.asString());
	}

	{
		Variant v{nullptr};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("null", v.asString());
	}

	{
		Variant v{42};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("42", v.asString());
	}

	{
		Variant v{42.5};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("42.5", v.asString());
	}

	{
		Variant v{{1, 2, 3}};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("", v.asString());
	}
}

TEST(Argument, validateStringDefault)
{
	Argument a = Argument::String("a", "test2");

	ASSERT_TRUE(a.hasDefault());
	ASSERT_EQ("test2", a.getDefaultValue().asString());

	{
		Variant v{"test"};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("test", v.asString());
	}

	{
		Variant v{true};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("true", v.asString());
	}

	{
		Variant v{nullptr};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("null", v.asString());
	}

	{
		Variant v{42};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("42", v.asString());
	}

	{
		Variant v{42.5};
		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("42.5", v.asString());
	}

	{
		Variant v{{1, 2, 3}};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("test2", v.asString());
	}
}

TEST(Argument, validateObject)
{
	Manager mgr;
	Argument a = Argument::Object("a", RttiTypes::TestManaged1);

	ASSERT_FALSE(a.hasDefault());

	{
		Rooted<Managed> m{new Managed(mgr)};
		Variant v = Variant::fromObject(m);

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(nullptr, v.asObject());
	}

	{
		Rooted<TestManaged1> m{new TestManaged1(mgr)};
		Variant v = Variant::fromObject(m);

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(m, v.asObject());
	}

	{
		Rooted<TestManaged2> m{new TestManaged2(mgr)};
		Variant v = Variant::fromObject(m);

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(m, v.asObject());
	}

	{
		Rooted<TestManaged1> m1{nullptr};
		Variant v = Variant::fromObject(m1);

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(nullptr, v.asObject());
	}

	{
		Variant v("test");

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(nullptr, v.asObject());
	}
}

TEST(Argument, validateObjectDefault)
{
	Manager mgr;
	Argument a = Argument::Object("a", RttiTypes::TestManaged1, nullptr);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_TRUE(a.getDefaultValue().isObject());
	ASSERT_EQ(nullptr, a.getDefaultValue().asObject());

	{
		Rooted<Managed> m{new Managed(mgr)};
		Variant v = Variant::fromObject(m);

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(nullptr, v.asObject());
	}

	{
		Rooted<TestManaged1> m{new TestManaged1(mgr)};
		Variant v = Variant::fromObject(m);

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(m, v.asObject());
	}

	{
		Rooted<TestManaged2> m{new TestManaged2(mgr)};
		Variant v = Variant::fromObject(m);

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(m, v.asObject());
	}

	{
		Rooted<TestManaged1> m1{nullptr};
		Variant v = Variant::fromObject(m1);

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(nullptr, v.asObject());
	}

	{
		Variant v("test");

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isObject());
		ASSERT_EQ(nullptr, v.asObject());
	}
}

static std::shared_ptr<Function> helloWorldFun{new Method<void>{
    [](Variant::arrayType &arr, void *) { return Variant{"Hello World"}; }}};

static std::shared_ptr<Function> goodbyeWorldFun{
    new Method<void>{[](Variant::arrayType &arr,
                        void *) { return Variant{"Goodbye Cruel World"}; }}};

TEST(Argument, validateFunction)
{
	Argument a = Argument::Function("a");

	ASSERT_FALSE(a.hasDefault());

	{
		Variant v = Variant::fromFunction(helloWorldFun);

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isFunction());
		ASSERT_EQ("Hello World", v.asFunction()->call().asString());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isFunction());
		ASSERT_TRUE(v.asFunction()->call().isNull());
	}
}

TEST(Argument, validateFunctionDefault)
{
	Argument a = Argument::Function("a", goodbyeWorldFun);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_TRUE(a.getDefaultValue().isFunction());
	ASSERT_EQ(goodbyeWorldFun, a.getDefaultValue().asFunction());

	{
		Variant v = Variant::fromFunction(helloWorldFun);

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isFunction());
		ASSERT_EQ("Hello World", v.asFunction()->call().asString());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isFunction());
		ASSERT_EQ("Goodbye Cruel World", v.asFunction()->call().asString());
	}
}

TEST(Argument, validateArray)
{
	Argument a = Argument::Array("a");

	ASSERT_FALSE(a.hasDefault());

	{
		Variant::arrayType arr{1, "a", nullptr};
		Variant v{arr};

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(arr, v.asArray());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(Variant::arrayType{}, v.asArray());
	}
}

TEST(Argument, validateArrayDefault)
{
	Variant::arrayType arrDefault{1, "a", nullptr};
	Argument a = Argument::Array("a", arrDefault);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_TRUE(a.getDefaultValue().isArray());
	ASSERT_EQ(arrDefault, a.getDefaultValue().asArray());

	{
		Variant::arrayType arr{"test1", 42.5};
		Variant v{arr};

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(arr, v.asArray());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(arrDefault, v.asArray());
	}
}

TEST(Argument, validateArrayInner)
{
	Argument a = Argument::Array("a", RttiTypes::String);

	ASSERT_FALSE(a.hasDefault());

	{
		Variant::arrayType arr{1, "a", nullptr};
		Variant v{arr};

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(Variant::arrayType({"1", "a", "null"}), v.asArray());
	}

	{
		Variant::arrayType arr{1, Variant::fromObject(nullptr), "a"};
		Variant v{arr};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(Variant::arrayType({"1", "", "a"}), v.asArray());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(Variant::arrayType{}, v.asArray());
	}
}

TEST(Argument, validateArrayInnerDefault)
{
	Variant::arrayType arrDefault{1, "a", nullptr};
	Argument a = Argument::Array("a", RttiTypes::String, arrDefault);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_TRUE(a.getDefaultValue().isArray());
	ASSERT_EQ(arrDefault, a.getDefaultValue().asArray());

	{
		Variant::arrayType arr{"test1", 42.5};
		Variant v{arr};

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(Variant::arrayType({"test1", "42.5"}), v.asArray());
	}

	{
		Variant::arrayType arr{"test1", 42.5, Variant::fromObject(nullptr)};
		Variant v{arr};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(arrDefault, v.asArray());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isArray());
		ASSERT_EQ(arrDefault, v.asArray());
	}
}

TEST(Argument, validateMap)
{
	Argument a = Argument::Map("a");

	ASSERT_FALSE(a.hasDefault());

	{
		Variant::mapType map{{"key1", 1}, {"key2", "a"}, {"key3", nullptr}};
		Variant v{map};

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(map, v.asMap());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(Variant::mapType{}, v.asMap());
	}
}

TEST(Argument, validateMapDefault)
{
	Variant::mapType mapDefault{{"key1", 1}, {"key2", "a"}, {"key3", nullptr}};
	Argument a = Argument::Map("a", mapDefault);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_TRUE(a.getDefaultValue().isMap());
	ASSERT_EQ(mapDefault, a.getDefaultValue().asMap());

	{
		Variant::mapType map{{"a", true}, {"b", "a"}};
		Variant v{map};

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(map, v.asMap());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(mapDefault, v.asMap());
	}
}

TEST(Argument, validateMapInnerType)
{
	Argument a = Argument::Map("a", RttiTypes::String);

	ASSERT_FALSE(a.hasDefault());

	{
		Variant::mapType map{{"key1", 1}, {"key2", "a"}, {"key3", nullptr}};
		Variant v{map};

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(
		    Variant::mapType({{"key1", "1"}, {"key2", "a"}, {"key3", "null"}}),
		    v.asMap());
	}

	{
		Variant::mapType map{
		    {"key1", 1}, {"key2", Variant::fromObject(nullptr)}, {"key3", "a"}};
		Variant v{map};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(
		    Variant::mapType({{"key1", "1"}, {"key2", ""}, {"key3", "a"}}),
		    v.asMap());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(Variant::mapType{}, v.asMap());
	}
}

TEST(Argument, validateMapInnerTypeDefault)
{
	Variant::mapType mapDefault{{"key1", "1"}};
	Argument a = Argument::Map("a", RttiTypes::String, mapDefault);

	ASSERT_TRUE(a.hasDefault());
	ASSERT_TRUE(a.getDefaultValue().isMap());
	ASSERT_EQ(mapDefault, a.getDefaultValue().asMap());

	{
		Variant::mapType map{{"key1", 1}, {"key2", "a"}, {"key3", nullptr}};
		Variant v{map};

		ASSERT_TRUE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(
		    Variant::mapType({{"key1", "1"}, {"key2", "a"}, {"key3", "null"}}),
		    v.asMap());
	}

	{
		Variant::mapType map{
		    {"key1", 1}, {"key2", Variant::fromObject(nullptr)}, {"key3", "a"}};
		Variant v{map};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(mapDefault, v.asMap());
	}

	{
		Variant v{"foo"};

		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isMap());
		ASSERT_EQ(mapDefault, v.asMap());
	}
}

TEST(Arguments, construction)
{
	// This should work without exception
	Arguments{Argument::Int("a"), Argument::Any("b")};

	// This should throw an exception
	ASSERT_THROW(Arguments({Argument::Int("a"), Argument::Any("a")}),
	             OusiaException);
	ASSERT_THROW(Arguments({Argument::Int("test test")}), OusiaException);
}

TEST(Arguments, invalid)
{
	Arguments argsInvalid{};

	Arguments argsValid{{}};

	Variant::arrayType arr{1};

	ASSERT_TRUE(argsInvalid.validateArray(arr, logger)); // No error message
	ASSERT_FALSE(argsValid.validateArray(arr, logger)); // Too many arguments
}

TEST(Arguments, validateArray)
{
	Arguments args{Argument::Int("a"), Argument::String("b", "test"),
	               Argument::Bool("c", true)};

	{
		Variant::arrayType arr{1, 5, false};
		ASSERT_TRUE(args.validateArray(arr, logger));
		ASSERT_EQ(Variant::arrayType({1, "5", false}), arr);
	}

	{
		Variant::arrayType arr{1, 5};
		ASSERT_TRUE(args.validateArray(arr, logger));
		ASSERT_EQ(Variant::arrayType({1, "5", true}), arr);
	}

	{
		Variant::arrayType arr{1};
		ASSERT_TRUE(args.validateArray(arr, logger));
		ASSERT_EQ(Variant::arrayType({1, "test", true}), arr);
	}

	{
		Variant::arrayType arr{};
		ASSERT_FALSE(args.validateArray(arr, logger));
		ASSERT_EQ(Variant::arrayType({0, "test", true}), arr);
	}

	{
		Variant::arrayType arr{1, "bla", false, 42};
		ASSERT_FALSE(args.validateArray(arr, logger));
		ASSERT_EQ(Variant::arrayType({1, "bla", false}), arr);
	}
}

TEST(Arguments, validateMap)
{
	Arguments args{Argument::Int("a"), Argument::String("b", "test"),
	               Argument::Bool("c", true)};

	{
		Variant::mapType map{{"a", 2}, {"b", 5}, {"c", true}};
		ASSERT_TRUE(args.validateMap(map, logger, false));
		ASSERT_EQ(Variant::mapType({{"a", 2}, {"b", "5"}, {"c", true}}), map);
	}

	{
		Variant::mapType map{{"a", 2}, {"c", false}};
		ASSERT_TRUE(args.validateMap(map, logger, false));
		ASSERT_EQ(Variant::mapType({{"a", 2}, {"b", "test"}, {"c", false}}),
		          map);
	}

	{
		Variant::mapType map{{"a", 2}};
		ASSERT_TRUE(args.validateMap(map, logger, false));
		ASSERT_EQ(Variant::mapType({{"a", 2}, {"b", "test"}, {"c", true}}),
		          map);
	}

	{
		Variant::mapType map{};
		ASSERT_FALSE(args.validateMap(map, logger, false));
		ASSERT_EQ(Variant::mapType({{"a", 0}, {"b", "test"}, {"c", true}}),
		          map);
	}

	{
		Variant::mapType map{{"a", 2}, {"d", nullptr}};
		ASSERT_FALSE(args.validateMap(map, logger, false));
		ASSERT_EQ(Variant::mapType(
		              {{"a", 2}, {"b", "test"}, {"c", true}, {"d", nullptr}}),
		          map);
	}

	{
		Variant::mapType map{{"a", 2}, {"d", nullptr}};
		ASSERT_TRUE(args.validateMap(map, logger, true));
		ASSERT_EQ(Variant::mapType(
		              {{"a", 2}, {"b", "test"}, {"c", true}, {"d", nullptr}}),
		          map);
	}
}

TEST(Arguments, validateMissing)
{
	Arguments args{Argument::String("a")};

	{
		Variant::mapType map{};
		ASSERT_FALSE(args.validateMap(map, logger, false));
		ASSERT_EQ(Variant::mapType({{"a", ""}}), map);
	}

	{
		Variant::arrayType arr{};
		ASSERT_FALSE(args.validateArray(arr, logger));
		ASSERT_EQ(Variant::arrayType({""}), arr);
	}
}

}

