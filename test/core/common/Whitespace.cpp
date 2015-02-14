/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#include <core/common/Whitespace.hpp>

namespace ousia {

TEST(Whitespace, trim)
{
	ASSERT_EQ("hello world", Whitespace::trim("\t hello world   \n\r\t"));
	ASSERT_EQ("hello world", Whitespace::trim("hello world   \n\r\t"));
	ASSERT_EQ("hello world", Whitespace::trim("   hello world"));
	ASSERT_EQ("hello world", Whitespace::trim("hello world"));
}

TEST(Whitespace, collapse)
{
	ASSERT("hello world", Whitespace::collapse(" hello \n\t\r  world  \n\r\t"));
	ASSERT("hello world", Whitespace::collapse("hello \n\t\r  world   \n\r\t"));
	ASSERT("hello world", Whitespace::collapse("hello \n\t\r     world"));
	ASSERT("hello world", Whitespace::collapse("hello world"));
}
}

