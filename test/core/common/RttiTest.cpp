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
#include <string>
#include <iostream>

#include <gtest/gtest.h>

#include <core/common/Function.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/TypedRttiBuilder.hpp>
#include <core/common/Variant.hpp>

namespace ousia {
namespace {

class RttiTestClass1 {
};
class RttiTestClass2 {
};
class RttiTestClass3 {
};
class RttiTestClass4 {
};
class RttiTestClass5 {
};
class RttiTestClass6 {
};
class RttiTestClass7 {
};

extern const Rtti<RttiTestClass6> Type6;
extern const Rtti<RttiTestClass7> Type7;

const Rtti<RttiTestClass1> Type1 = RttiBuilder{"Type1"};
const Rtti<RttiTestClass2> Type2 = RttiBuilder{"Type2"};
const Rtti<RttiTestClass3> Type3 = RttiBuilder{"Type3"}.parent(&Type1);
const Rtti<RttiTestClass4> Type4 =
    RttiBuilder{"Type4"}.parent({&Type3, &Type2});
const Rtti<RttiTestClass5> Type5 =
    RttiBuilder{"Type5"}.composedOf({&Type6, &Type7});
const Rtti<RttiTestClass6> Type6 = RttiBuilder{"Type6"}.composedOf(&Type1);
const Rtti<RttiTestClass7> Type7 = RttiBuilder{"Type7"}.parent(&Type6);

TEST(Rtti, isa)
{
	ASSERT_TRUE(Type1.isa(Type1));
	ASSERT_FALSE(Type1.isa(Type2));
	ASSERT_FALSE(Type1.isa(Type3));
	ASSERT_FALSE(Type1.isa(Type4));

	ASSERT_FALSE(Type2.isa(Type1));
	ASSERT_TRUE(Type2.isa(Type2));
	ASSERT_FALSE(Type2.isa(Type3));
	ASSERT_FALSE(Type2.isa(Type4));

	ASSERT_TRUE(Type3.isa(Type1));
	ASSERT_FALSE(Type3.isa(Type2));
	ASSERT_TRUE(Type3.isa(Type3));
	ASSERT_FALSE(Type3.isa(Type4));

	ASSERT_TRUE(Type4.isa(Type1));
	ASSERT_TRUE(Type4.isa(Type2));
	ASSERT_TRUE(Type4.isa(Type3));
	ASSERT_TRUE(Type4.isa(Type4));
}

TEST(Rtti, composedOf)
{
	std::vector<const RttiType *> types{&Type1, &Type2, &Type3, &Type4};
	for (auto t : types) {
		ASSERT_FALSE(t->composedOf(Type1));
		ASSERT_FALSE(t->composedOf(Type2));
		ASSERT_FALSE(t->composedOf(Type3));
		ASSERT_FALSE(t->composedOf(Type4));
		ASSERT_FALSE(t->composedOf(Type5));
		ASSERT_FALSE(t->composedOf(Type6));
		ASSERT_FALSE(t->composedOf(Type7));
	}

	ASSERT_TRUE(Type5.composedOf(Type1));
	ASSERT_FALSE(Type5.composedOf(Type2));
	ASSERT_FALSE(Type5.composedOf(Type3));
	ASSERT_FALSE(Type5.composedOf(Type4));
	ASSERT_FALSE(Type5.composedOf(Type5));
	ASSERT_TRUE(Type5.composedOf(Type6));
	ASSERT_TRUE(Type5.composedOf(Type7));

	ASSERT_TRUE(Type6.composedOf(Type1));
	ASSERT_FALSE(Type6.composedOf(Type2));
	ASSERT_FALSE(Type6.composedOf(Type3));
	ASSERT_FALSE(Type6.composedOf(Type4));
	ASSERT_FALSE(Type6.composedOf(Type5));
	ASSERT_FALSE(Type6.composedOf(Type6));
	ASSERT_FALSE(Type6.composedOf(Type7));

	ASSERT_TRUE(Type7.composedOf(Type1));
	ASSERT_FALSE(Type7.composedOf(Type2));
	ASSERT_FALSE(Type7.composedOf(Type3));
	ASSERT_FALSE(Type7.composedOf(Type4));
	ASSERT_FALSE(Type7.composedOf(Type5));
	ASSERT_FALSE(Type7.composedOf(Type6));
	ASSERT_FALSE(Type7.composedOf(Type7));
}

class RttiMethodTestClass1 {
};
class RttiMethodTestClass2 {
};

static const Rtti<RttiMethodTestClass1> MType1 =
    RttiBuilder{"MType1"}
        .genericMethod(
             "a", std::make_shared<Method<RttiMethodTestClass1>>([](
                      Variant::arrayType &args,
                      RttiMethodTestClass1 *thisPtr) { return Variant{"a"}; }))
        .genericMethod(
             "b", std::make_shared<Method<RttiMethodTestClass1>>([](
                      Variant::arrayType &args,
                      RttiMethodTestClass1 *thisPtr) { return Variant{"b"}; }))
        .genericMethod(
            "c", std::make_shared<Method<RttiMethodTestClass1>>([](
                     Variant::arrayType &args,
                     RttiMethodTestClass1 *thisPtr) { return Variant{"c"}; }));

static const Rtti<RttiMethodTestClass2> MType2 =
    TypedRttiBuilder<RttiMethodTestClass2>{"MType2"}
        .parent(&MType1)
        .method("c",
                [](Variant::arrayType &args,
                   RttiMethodTestClass2 *thisPtr) { return Variant{"c2"}; })
        .method("d", [](Variant::arrayType &args,
                        RttiMethodTestClass2 *thisPtr) { return Variant{"d"}; })
        .method("e",
                {{Argument::Int("a"), Argument::Int("b")},
                 [](Variant::arrayType &args, RttiMethodTestClass2 *thisPtr) {
	                return Variant{args[0].asInt() * args[1].asInt()};
	            }});

TEST(Rtti, methods)
{
	auto methods = MType1.getMethods();
	ASSERT_TRUE(methods.count("a") > 0);
	ASSERT_TRUE(methods.count("b") > 0);
	ASSERT_TRUE(methods.count("c") > 0);

	ASSERT_FALSE(MType1.getMethod("a") == nullptr);
	ASSERT_FALSE(MType1.getMethod("b") == nullptr);
	ASSERT_FALSE(MType1.getMethod("c") == nullptr);
	ASSERT_TRUE(MType1.getMethod("d") == nullptr);

	ASSERT_EQ("a", MType1.getMethod("a")->call().asString());
	ASSERT_EQ("b", MType1.getMethod("b")->call().asString());
	ASSERT_EQ("c", MType1.getMethod("c")->call().asString());

	methods = MType2.getMethods();
	ASSERT_TRUE(methods.count("a") > 0);
	ASSERT_TRUE(methods.count("b") > 0);
	ASSERT_TRUE(methods.count("c") > 0);
	ASSERT_TRUE(methods.count("d") > 0);

	ASSERT_FALSE(MType2.getMethod("a") == nullptr);
	ASSERT_FALSE(MType2.getMethod("b") == nullptr);
	ASSERT_FALSE(MType2.getMethod("c") == nullptr);
	ASSERT_FALSE(MType2.getMethod("d") == nullptr);

	ASSERT_EQ("a", MType2.getMethod("a")->call().asString());
	ASSERT_EQ("b", MType2.getMethod("b")->call().asString());
	ASSERT_EQ("c2", MType2.getMethod("c")->call().asString());
	ASSERT_EQ("d", MType2.getMethod("d")->call().asString());
	ASSERT_EQ(42,
	          MType2.getMethod("e")->call(Variant::arrayType{6, 7}).asInt());
	ASSERT_THROW(MType2.getMethod("e")->call(Variant::arrayType{6, "7"}),
	             LoggableException);
}

class RttiPropertyTestClass1 {
public:
	int a;

	RttiPropertyTestClass1() : a(0) {}

	static Variant getA(const RttiPropertyTestClass1 *obj) { return obj->a; }

	static void setA(const Variant &value, RttiPropertyTestClass1 *obj)
	{
		obj->a = value.asInt();
	}
};

class RttiPropertyTestClass2 : public RttiPropertyTestClass1 {
public:
	int b;

	RttiPropertyTestClass2() : b(0) {}

	static Variant getB(const RttiPropertyTestClass2 *obj) { return obj->b; }

	static void setB(const Variant &value, RttiPropertyTestClass2 *obj)
	{
		obj->b = value.asInt();
	}
};

static const Rtti<RttiPropertyTestClass1> PType1 =
    TypedRttiBuilder<RttiPropertyTestClass1>{"PType1"}.property(
        "a", {RttiTypes::Int, RttiPropertyTestClass1::getA,
              RttiPropertyTestClass1::setA});

static const Rtti<RttiMethodTestClass2> PType2 =
    TypedRttiBuilder<RttiPropertyTestClass2>{"PType2"}.parent(&PType1).property(
        "b", {RttiTypes::Int, RttiPropertyTestClass2::getB,
              RttiPropertyTestClass2::setB});

TEST(Rtti, properties)
{
	RttiPropertyTestClass2 obj;

	auto properties = PType1.getProperties();
	ASSERT_TRUE(properties.count("a") > 0);
	ASSERT_FALSE(properties.count("b") > 0);

	ASSERT_FALSE(PType1.getProperty("a") == nullptr);
	ASSERT_TRUE(PType1.getProperty("b") == nullptr);

	ASSERT_EQ(0, PType1.getProperty("a")->get(&obj).asInt());
	PType1.getProperty("a")->set(4, &obj);
	ASSERT_EQ(4, PType1.getProperty("a")->get(&obj).asInt());

	properties = PType2.getProperties();
	ASSERT_TRUE(properties.count("a") > 0);
	ASSERT_TRUE(properties.count("b") > 0);

	ASSERT_FALSE(PType2.getProperty("a") == nullptr);
	ASSERT_FALSE(PType2.getProperty("b") == nullptr);

	ASSERT_EQ(4, PType2.getProperty("a")->get(&obj).asInt());
	PType2.getProperty("a")->set(8, &obj);
	ASSERT_EQ(8, PType2.getProperty("a")->get(&obj).asInt());

	ASSERT_EQ(0, PType2.getProperty("b")->get(&obj).asInt());
	PType2.getProperty("b")->set(5, &obj);
	ASSERT_EQ(5, PType2.getProperty("b")->get(&obj).asInt());
}
}
}

