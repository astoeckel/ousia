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

#include <core/common/Logger.hpp>
#include <core/common/Argument.hpp>

#include <core/managed/Managed.hpp>

namespace ousia {

//static Logger logger;
static TerminalLogger logger(std::cerr, true);

TEST(Argument, validateBool)
{
	Argument a = Argument::Bool("a");

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

	Variant v{1};
	ASSERT_FALSE(a.validate(v, logger));
	ASSERT_TRUE(v.isBool());
	ASSERT_TRUE(v.asBool());
}

TEST(Argument, validateInt)
{
	Argument a = Argument::Int("a");

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

	Variant v{1.1};
	ASSERT_FALSE(a.validate(v, logger));
	ASSERT_TRUE(v.isInt());
	ASSERT_EQ(42, v.asInt());
}

TEST(Argument, validateDouble)
{
	Argument a = Argument::Double("a");

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

	Variant v{"1.0"};
	ASSERT_FALSE(a.validate(v, logger));
	ASSERT_TRUE(v.isDouble());
	ASSERT_EQ(42.0, v.asDouble());
}

TEST(Argument, validateString)
{
	Argument a = Argument::String("a");

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

	{
		Variant v{{1, 2, 3}};
		ASSERT_FALSE(a.validate(v, logger));
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("test2", v.asString());
	}
}

}

