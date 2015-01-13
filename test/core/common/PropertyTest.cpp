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

#include <core/common/Rtti.hpp>
#include <core/common/Property.hpp>

namespace ousia {

namespace {
struct TestObject {
	TestObject(int a) : a(a) {}
	int a;

	static Variant getA(const TestObject *obj) { return Variant{obj->a}; }

	static void setA(const Variant &value, TestObject *obj)
	{
		if (value.isInt()) {
			obj->a = value.asInt();
		}
	}
};
}

static Variant getString(const TestObject *obj) { return "foo"; }

TEST(Getter, construction)
{
	{
		Getter<TestObject> getter{};
		ASSERT_FALSE(getter.isValid());
	}

	{
		Getter<TestObject> getter{nullptr};
		ASSERT_FALSE(getter.isValid());
	}

	{
		Getter<TestObject> getter{TestObject::getA};
		ASSERT_TRUE(getter.isValid());
	}
}

TEST(Getter, validation)
{
	const PropertyType type{RttiTypes::Int};
	TestObject obj{123};

	{
		// No specifiy type set, strings can be returned
		Getter<TestObject> getter{getString};
		ASSERT_EQ("foo", getter.get(&obj));
	}

	{
		// Int type set, returning strings is an exception
		Getter<TestObject> getter{getString};
		getter.propertyType = &type;
		ASSERT_THROW(getter.get(&obj), LoggableException);
	}

	{
		Getter<TestObject> getter{TestObject::getA};

		// Basic functionality
		ASSERT_EQ(123, getter.call(Variant::arrayType{}, &obj));

		// Exception should be thrown if an argument is explicitly given
		ASSERT_THROW(getter.call(Variant::arrayType{1}, &obj),
		             PropertyException);
	}
}

TEST(Setter, construction)
{
	{
		Setter<TestObject> setter{};
		ASSERT_FALSE(setter.isValid());
	}

	{
		Setter<TestObject> setter{nullptr};
		ASSERT_FALSE(setter.isValid());
	}

	{
		Setter<TestObject> setter{TestObject::setA};
		ASSERT_TRUE(setter.isValid());
	}
}

TEST(Setter, validation)
{
	const PropertyType type{RttiTypes::Int};
	TestObject obj{123};

	Setter<TestObject> setter{TestObject::setA};

	// An exception should be thrown if not exactly one argument is passed to
	// the setter
	ASSERT_THROW(setter.call(Variant::arrayType{}, &obj), PropertyException);
	ASSERT_THROW(setter.call(Variant::arrayType{1, 2}, &obj),
	             PropertyException);

	setter.call(Variant::arrayType{42}, &obj);  // OK
	ASSERT_EQ(42, obj.a);

	// No specifiy type set, any value can be given (does not crash because of
	// explicity type check in the callback function, see above).
	setter.set("foo", &obj);
	ASSERT_EQ(42, obj.a);

	setter.propertyType = &type;
	ASSERT_THROW(setter.set("foo", &obj), LoggableException);

	setter.set(123, &obj);
	ASSERT_EQ(123, obj.a);
}

TEST(Property, construction)
{
	TestObject obj{123};

	{
		ASSERT_THROW(Property<TestObject> property{nullptr}, PropertyException);
	}

	{
		Property<TestObject> property{TestObject::getA};
		ASSERT_TRUE(property.isReadonly());
		ASSERT_THROW(property.set(42, &obj), LoggableException);
	}

	{
		Property<TestObject> property{TestObject::getA, TestObject::setA};
		ASSERT_FALSE(property.isReadonly());
		ASSERT_EQ(123, property.get(&obj).asInt());

		property.set(42, &obj);
		ASSERT_EQ(42, property.get(&obj).asInt());

		property.set("bla", &obj);
		ASSERT_EQ(42, property.get(&obj).asInt());
	}

	{
		Property<TestObject> property{RttiTypes::Int, TestObject::getA,
		                              TestObject::setA};
		ASSERT_FALSE(property.isReadonly());

		ASSERT_EQ(42, property.get(&obj).asInt());

		property.set(123, &obj);
		ASSERT_EQ(123, property.get(&obj).asInt());

		ASSERT_THROW(property.set("bla", &obj), LoggableException);
	}
}
}

