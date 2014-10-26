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

/*
 * This file contains generic tests for JavaScript script engines. JavaScript
 * script engine bindings should have their own test project in which the
 * GENERIC_JS_TEST_NAME, GENERIC_JS_TEST_SCOPE macros are defined and this
 * file is included.
 *
 * TODO: The macros are more of a dirty hack -- probably use gtest fixtures and
 * templates here.
 */

#ifndef _GENERIC_JS_SCRIPT_ENGINE_TEST_HPP_
#define _GENERIC_JS_SCRIPT_ENGINE_TEST_HPP_

#ifndef GENERIC_JS_TEST_NAME
#error "Macro GENERIC_JS_TEST_NAME is not set!"
#endif /* GENERIC_JS_TEST_NAME */

#ifndef GENERIC_JS_TEST_SCOPE
#error "Macro GENERIC_JS_TEST_SCOPE is not set!"
#endif /* GENERIC_JS_TEST_SCOPE */

TEST(GENERIC_JS_TEST_NAME, returnNull)
{
	GENERIC_JS_TEST_SCOPE
	Variant res = scope->run("null;");
	ASSERT_EQ(VariantType::null, res.getType());
}

TEST(GENERIC_JS_TEST_NAME, returnBoolean)
{
	GENERIC_JS_TEST_SCOPE
	Variant res = scope->run("true;");
	ASSERT_EQ(VariantType::boolean, res.getType());
	ASSERT_TRUE(res.getBooleanValue());
}

TEST(GENERIC_JS_TEST_NAME, returnInteger)
{
	GENERIC_JS_TEST_SCOPE
	Variant res = scope->run("42;");
	ASSERT_EQ(VariantType::integer, res.getType());
	ASSERT_EQ(42, res.getIntegerValue());
}

TEST(GENERIC_JS_TEST_NAME, returnNumber)
{
	GENERIC_JS_TEST_SCOPE
	Variant res = scope->run("42.5;");
	ASSERT_EQ(VariantType::number, res.getType());
	ASSERT_EQ(42.5, res.getNumberValue());
}

TEST(GENERIC_JS_TEST_NAME, returnString)
{
	GENERIC_JS_TEST_SCOPE
	Variant res = scope->run("\"Hello World\";");
	ASSERT_EQ(VariantType::string, res.getType());
	ASSERT_EQ("Hello World", res.getStringValue());
}

TEST(GENERIC_JS_TEST_NAME, returnArray)
{
	GENERIC_JS_TEST_SCOPE
	Variant res = scope->run("[42, \"Hello World\", false];");
	ASSERT_EQ(VariantType::array, res.getType());

	std::vector<Variant> a = res.getArrayValue();
	ASSERT_EQ(3, a.size());

	ASSERT_EQ(VariantType::integer, a[0].getType());
	ASSERT_EQ(42, a[0].getIntegerValue());

	ASSERT_EQ(VariantType::string, a[1].getType());
	ASSERT_EQ("Hello World", a[1].getStringValue());

	ASSERT_EQ(VariantType::boolean, a[2].getType());
	ASSERT_FALSE(a[2].getBooleanValue());
}

TEST(GENERIC_JS_TEST_NAME, returnObject)
{
	GENERIC_JS_TEST_SCOPE
	Variant res = scope->run(
	    "var obj = {\"key1\": 42, \"key2\": \"Hello World\", \"key3\": false}; obj");
	ASSERT_EQ(VariantType::map, res.getType());

	std::map<std::string, Variant> m = res.getMapValue();
	ASSERT_EQ(3, m.size());

	ASSERT_TRUE(m.find("key1") != m.end());
	ASSERT_TRUE(m.find("key2") != m.end());
	ASSERT_TRUE(m.find("key3") != m.end());

	ASSERT_EQ(VariantType::integer, m["key1"].getType());
	ASSERT_EQ(42, m["key1"].getIntegerValue());

	ASSERT_EQ(VariantType::string, m["key2"].getType());
	ASSERT_EQ("Hello World", m["key2"].getStringValue());

	ASSERT_EQ(VariantType::boolean, m["key3"].getType());
	ASSERT_FALSE(m["key3"].getBooleanValue());
}

TEST(GENERIC_JS_TEST_NAME, returnFunction)
{
	GENERIC_JS_TEST_SCOPE
	Variant res = scope->run("(function () {return \"Hello World\";})");
	ASSERT_EQ(VariantType::function, res.getType());

	Variant fres = res.getFunctionValue()->call();
	ASSERT_EQ(VariantType::string, fres.getType());
	ASSERT_EQ("Hello World", fres.getStringValue());
}

#endif /* _GENERIC_JS_SCRIPT_ENGINE_TEST_HPP_ */

