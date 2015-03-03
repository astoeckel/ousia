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

#include <core/common/Utils.hpp>

namespace ousia {

TEST(Utils, isIdentifier)
{
	EXPECT_TRUE(Utils::isIdentifier("test"));
	EXPECT_TRUE(Utils::isIdentifier("t0-_est"));
	EXPECT_FALSE(Utils::isIdentifier("_t0-_EST"));
	EXPECT_FALSE(Utils::isIdentifier("-t0-_EST"));
	EXPECT_FALSE(Utils::isIdentifier("0t-_EST"));
	EXPECT_FALSE(Utils::isIdentifier("_A"));
	EXPECT_FALSE(Utils::isIdentifier("invalid key"));
	EXPECT_FALSE(Utils::isIdentifier(""));
}


TEST(Utils, isNamespacedIdentifier)
{
	EXPECT_TRUE(Utils::isNamespacedIdentifier("test"));
	EXPECT_TRUE(Utils::isNamespacedIdentifier("t0-_est"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("_t0-_EST"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("-t0-_EST"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("0t-_EST"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("invalid key"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("_A"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier(""));
	EXPECT_FALSE(Utils::isNamespacedIdentifier(":"));
	EXPECT_TRUE(Utils::isNamespacedIdentifier("test:a"));
	EXPECT_TRUE(Utils::isNamespacedIdentifier("t0-_est:b"));
	EXPECT_TRUE(Utils::isNamespacedIdentifier("test:test"));
	EXPECT_TRUE(Utils::isNamespacedIdentifier("t0-_est:t0-_est"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("test:_A"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("test::a"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier(":test"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("t0-_est:_t0-_EST"));
	EXPECT_FALSE(Utils::isNamespacedIdentifier("t0-_est: b"));
}


TEST(Utils, split)
{
	ASSERT_EQ(std::vector<std::string>({"ab"}), Utils::split("ab", '.'));
	ASSERT_EQ(std::vector<std::string>({"a", ""}), Utils::split("a.", '.'));
	ASSERT_EQ(std::vector<std::string>({"", ""}), Utils::split(".", '.'));
	ASSERT_EQ(std::vector<std::string>({"a", "b"}), Utils::split("a.b", '.'));
	ASSERT_EQ(std::vector<std::string>({"a", "b"}), Utils::split("a.b", '.'));
	ASSERT_EQ(std::vector<std::string>({"a", "b", "c"}),
	          Utils::split("a.b.c", '.'));
	ASSERT_EQ(std::vector<std::string>({"", "a", "b", "c"}),
	          Utils::split(".a.b.c", '.'));
	ASSERT_EQ(std::vector<std::string>({"", "a", "be", "c", ""}),
	          Utils::split(".a.be.c.", '.'));
}

TEST(Utils, toLower)
{
	ASSERT_EQ("", Utils::toLower(""));
	ASSERT_EQ("foo00", Utils::toLower("foo00"));
	ASSERT_EQ("foo00", Utils::toLower("fOO00"));
}

TEST(Utils, extractFileExtension)
{
	ASSERT_EQ("", Utils::extractFileExtension(""));
	ASSERT_EQ("", Utils::extractFileExtension("test"));
	ASSERT_EQ("ext", Utils::extractFileExtension("test.ext"));
	ASSERT_EQ("", Utils::extractFileExtension("foo.bar/test"));
	ASSERT_EQ("", Utils::extractFileExtension("foo.bar\\test"));
	ASSERT_EQ("ext", Utils::extractFileExtension("foo.bar/test.ext"));
	ASSERT_EQ("ext", Utils::extractFileExtension("foo.bar/test.EXT"));
}

TEST(Utils, startsWith)
{
	ASSERT_TRUE(Utils::startsWith("foobar", "foo"));
	ASSERT_TRUE(Utils::startsWith("foo", "foo"));
	ASSERT_FALSE(Utils::startsWith("foo", "foobar"));
	ASSERT_FALSE(Utils::startsWith("foobar", "bar"));
	ASSERT_TRUE(Utils::startsWith("foo", ""));
}

TEST(Utils, endsWith)
{
	ASSERT_FALSE(Utils::endsWith("foobar", "foo"));
	ASSERT_TRUE(Utils::endsWith("foo", "foo"));
	ASSERT_FALSE(Utils::endsWith("foo", "foobar"));
	ASSERT_TRUE(Utils::endsWith("foobar", "bar"));
	ASSERT_TRUE(Utils::endsWith("foo", ""));
}

TEST(Utils, trim)
{
	ASSERT_EQ("", Utils::trim(""));
	ASSERT_EQ("", Utils::trim("        "));
	ASSERT_EQ("test", Utils::trim("test"));
	ASSERT_EQ("test", Utils::trim("   test "));
	ASSERT_EQ("test", Utils::trim("   test"));
	ASSERT_EQ("test", Utils::trim("test  "));
	ASSERT_EQ("long    test", Utils::trim("     long    test   "));
}

TEST(Utils, collapse)
{
	ASSERT_EQ("", Utils::collapse(""));
	ASSERT_EQ("", Utils::collapse("        "));
	ASSERT_EQ("test", Utils::collapse("test"));
	ASSERT_EQ("test", Utils::collapse("   test "));
	ASSERT_EQ("test", Utils::collapse("   test"));
	ASSERT_EQ("test", Utils::collapse("test  "));
	ASSERT_EQ("long test", Utils::collapse("     long    test   "));
}

TEST(Utils, isUserDefinedToken)
{
	EXPECT_FALSE(Utils::isUserDefinedToken(""));
	EXPECT_FALSE(Utils::isUserDefinedToken("a"));
	EXPECT_TRUE(Utils::isUserDefinedToken(":"));
	EXPECT_TRUE(Utils::isUserDefinedToken("::"));
	EXPECT_TRUE(Utils::isUserDefinedToken("!?"));
	EXPECT_TRUE(Utils::isUserDefinedToken("."));
	EXPECT_TRUE(Utils::isUserDefinedToken("<<"));
	EXPECT_TRUE(Utils::isUserDefinedToken(">>"));
	EXPECT_TRUE(Utils::isUserDefinedToken("''"));
	EXPECT_TRUE(Utils::isUserDefinedToken("``"));
	EXPECT_TRUE(Utils::isUserDefinedToken("´´"));
	EXPECT_TRUE(Utils::isUserDefinedToken("´"));
	EXPECT_TRUE(Utils::isUserDefinedToken("`"));
	EXPECT_TRUE(Utils::isUserDefinedToken("<"));
	EXPECT_TRUE(Utils::isUserDefinedToken(">"));
	EXPECT_TRUE(Utils::isUserDefinedToken("<+>"));
	EXPECT_FALSE(Utils::isUserDefinedToken("a:"));
	EXPECT_FALSE(Utils::isUserDefinedToken("a:a"));
	EXPECT_FALSE(Utils::isUserDefinedToken(":a"));
	EXPECT_FALSE(Utils::isUserDefinedToken("{"));
	EXPECT_FALSE(Utils::isUserDefinedToken("{{"));
	EXPECT_FALSE(Utils::isUserDefinedToken("}}"));
	EXPECT_FALSE(Utils::isUserDefinedToken("{{}{}"));
	EXPECT_FALSE(Utils::isUserDefinedToken("<\\"));
	EXPECT_FALSE(Utils::isUserDefinedToken("\\>"));
	EXPECT_FALSE(Utils::isUserDefinedToken("{!"));
	EXPECT_FALSE(Utils::isUserDefinedToken("< + >"));
}

}
