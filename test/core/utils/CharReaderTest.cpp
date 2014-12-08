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

#include <string>
#include <iostream>

#include "gtest/gtest.h"

#include <core/utils/CharReader.hpp>

namespace ousia {
namespace utils {

TEST(Buffer, simpleRead)
{
	std::string testStr{"this is a test"};

	// Create buffer with the test string
	char c;
	Buffer buf{testStr};

	// Create a read cursor
	Buffer::CursorId cursor = buf.createCursor();

	// We're not at the end of the stream
	ASSERT_FALSE(buf.atEnd(cursor));

	// Try to read the test string
	std::string res;
	while (buf.read(cursor, c)) {
		res.append(&c, 1);
	}

	// The cursor must be at the end
	ASSERT_TRUE(buf.atEnd(cursor));

	// The two strings must equal
	ASSERT_STREQ(testStr.c_str(), res.c_str());
}

TEST(Buffer, twoCursors)
{
	std::string testStr{"this is a test"};

	// Create buffer with the test string
	char c;
	Buffer buf{testStr};

	// Create two read cursors
	Buffer::CursorId cur1 = buf.createCursor();
	Buffer::CursorId cur2 = buf.createCursor();

	ASSERT_FALSE(buf.atEnd(cur1));
	ASSERT_FALSE(buf.atEnd(cur2));

	// Try to read the test string with the first cursor
	std::string res1;
	while (buf.read(cur1, c)) {
		res1.append(&c, 1);
	}

	// The first cursor must be at the end
	ASSERT_TRUE(buf.atEnd(cur1));
	ASSERT_FALSE(buf.atEnd(cur2));

	// Try to read the test string with the second cursor
	std::string res2;
	while (buf.read(cur2, c)) {
		res2.append(&c, 1);
	}

	// The first cursor must be at the end
	ASSERT_TRUE(buf.atEnd(cur1));
	ASSERT_TRUE(buf.atEnd(cur2));

	// The two strings must equal
	ASSERT_EQ(testStr, res1);
	ASSERT_EQ(testStr, res2);
}

}
}

