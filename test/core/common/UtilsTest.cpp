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
	ASSERT_TRUE(Utils::isIdentifier("test"));
	ASSERT_TRUE(Utils::isIdentifier("t0-_est"));
	ASSERT_TRUE(Utils::isIdentifier("_t0-_EST"));
	ASSERT_FALSE(Utils::isIdentifier("-t0-_EST"));
	ASSERT_FALSE(Utils::isIdentifier("0t-_EST"));
}

TEST(Utils, trim)
{
	ASSERT_EQ("hello world", Utils::trim("\t hello world   \n\r\t"));
	ASSERT_EQ("hello world", Utils::trim("hello world   \n\r\t"));
	ASSERT_EQ("hello world", Utils::trim("   hello world"));
	ASSERT_EQ("hello world", Utils::trim("hello world"));
}

}

