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

#include <sstream>
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

	// The cursor must be at zero
	ASSERT_EQ(0, buf.offset(cursor));

	// Try to read the test string
	std::string res;
	while (buf.read(cursor, c)) {
		res.append(&c, 1);
	}

	// The cursor must be at the end
	ASSERT_TRUE(buf.atEnd(cursor));

	// The cursor must be one byond the last byte
	ASSERT_EQ(testStr.size(), buf.offset(cursor));

	// The two strings must equal
	ASSERT_STREQ(testStr.c_str(), res.c_str());
}

TEST(Buffer, cursorManagement)
{
	Buffer buf{""};

	Buffer::CursorId c1 = buf.createCursor();
	Buffer::CursorId c2 = buf.createCursor();
	Buffer::CursorId c3 = buf.createCursor();

	ASSERT_EQ(0, c1);
	ASSERT_EQ(1, c2);
	ASSERT_EQ(2, c3);

	buf.deleteCursor(c2);
	Buffer::CursorId c4 = buf.createCursor();
	ASSERT_EQ(1, c4);
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

TEST(Buffer, copyCursors)
{
	std::string testStr{"test1 test2 test3"};

	// Create buffer with the test string
	char c;
	Buffer buf{testStr};

	// Create two read cursors
	Buffer::CursorId cur1 = buf.createCursor();
	Buffer::CursorId cur2 = buf.createCursor();

	ASSERT_FALSE(buf.atEnd(cur1));
	ASSERT_FALSE(buf.atEnd(cur2));

	// Read the first six characters with cursor one
	std::string res1;
	for (int i = 0; i < 6; i++) {
		if (buf.read(cur1, c)) {
			res1.append(&c, 1);
		}
	}
	ASSERT_EQ("test1 ", res1);
	ASSERT_FALSE(buf.atEnd(cur1));

	// Copy cur1 to cur2, free cur1
	buf.copyCursor(cur1, cur2);
	buf.deleteCursor(cur1);

	std::string res2;
	for (int i = 0; i < 6; i++) {
		if (buf.read(cur2, c)) {
			res2.append(&c, 1);
		}
	}
	ASSERT_EQ("test2 ", res2);
	ASSERT_FALSE(buf.atEnd(cur2));

	// Create a new cursor as copy of cur2
	Buffer::CursorId cur3 = buf.createCursor(cur2);
	std::string res3;
	for (int i = 0; i < 6; i++) {
		if (buf.read(cur3, c)) {
			res3.append(&c, 1);
		}
	}
	ASSERT_EQ("test3", res3);

	ASSERT_TRUE(buf.atEnd(cur3));
}

// Generates some pseudo-random data
// (inspired by "Numerical Recipes, Third Edition", Chapter 7.17)
static std::vector<char> generateData(size_t len)
{
	const uint32_t B1 = 17;
	const uint32_t B2 = 15;
	const uint32_t B3 = 5;
	uint32_t v = 0xF3A99148;
	std::vector<char> res;
	for (size_t i = 0; i < len; i++) {
		v = v ^ (v >> B1);
		v = v ^ (v << B2);
		v = v ^ (v >> B3);
		res.push_back(v & 0xFF);
	}
	return res;
}

struct VectorReadState {
	size_t offs;
	const std::vector<char> &data;

	VectorReadState(const std::vector<char> &data) : offs(0), data(data) {}
};

static size_t readFromVector(char *buf, size_t size, void *userData)
{
	VectorReadState &state = *(static_cast<VectorReadState *>(userData));
	size_t tar = std::min(state.offs + size, state.data.size());
	for (size_t i = state.offs; i < tar; i++) {
		*buf = state.data[i];
		buf++;
	}
	size_t res = tar - state.offs;

//	std::cout << "called readFromVector, read from " << state.offs << " to "
//	          << tar << ", total " << res << " byte, requested " << size
//	          << " byte" << std::endl;

	state.offs = tar;
	return res;
}

static constexpr size_t DATA_LENGTH = 256 * 1024 + 795;
static const std::vector<char> DATA = generateData(DATA_LENGTH);

TEST(Buffer, simpleStream)
{
	VectorReadState state(DATA);

	Buffer buf{readFromVector, &state};
	Buffer::CursorId cursor = buf.createCursor();

	char c;
	std::vector<char> res;
	while (buf.read(cursor, c)) {
		res.push_back(c);
	}

	// We must be at the end of the buffer and the cursor offset must be set
	// correctly
	ASSERT_TRUE(buf.atEnd(cursor));
	ASSERT_EQ(DATA_LENGTH, buf.offset(cursor));

	// The read data and the original data must be equal
	ASSERT_EQ(DATA, res);
}

TEST(Buffer, streamTwoCursors)
{
	VectorReadState state(DATA);

	Buffer buf{readFromVector, &state};
	Buffer::CursorId cur1 = buf.createCursor();
	Buffer::CursorId cur2 = buf.createCursor();

	char c;

	std::vector<char> res1;
	while (buf.read(cur1, c)) {
		res1.push_back(c);
	}

	ASSERT_TRUE(buf.atEnd(cur1));
	ASSERT_FALSE(buf.atEnd(cur2));
	ASSERT_EQ(DATA_LENGTH, buf.offset(cur1));
	ASSERT_EQ(0, buf.offset(cur2));

	std::vector<char> res2;
	while (buf.read(cur2, c)) {
		res2.push_back(c);
	}

	ASSERT_TRUE(buf.atEnd(cur1));
	ASSERT_TRUE(buf.atEnd(cur2));
	ASSERT_EQ(DATA_LENGTH, buf.offset(cur1));
	ASSERT_EQ(DATA_LENGTH, buf.offset(cur2));

	// The read data and the original data must be equal
	ASSERT_EQ(DATA, res1);
	ASSERT_EQ(DATA, res2);
}

TEST(Buffer, streamTwoCursorsInterleaved)
{
	VectorReadState state(DATA);

	Buffer buf{readFromVector, &state};
	Buffer::CursorId cur1 = buf.createCursor();
	Buffer::CursorId cur2 = buf.createCursor();

	char c;

	std::vector<char> res1;
	std::vector<char> res2;
	while (!buf.atEnd(cur1) || !buf.atEnd(cur2)) {
		for (int i = 0; i < 100; i++) {
			if (buf.read(cur1, c)) {
				res1.push_back(c);
			}
		}
		for (int i = 0; i < 120; i++) {
			if (buf.read(cur2, c)) {
				res2.push_back(c);
			}
		}
	}

	ASSERT_EQ(DATA_LENGTH, buf.offset(cur1));
	ASSERT_EQ(DATA_LENGTH, buf.offset(cur2));

	// The read data and the original data must be equal
	ASSERT_EQ(DATA, res1);
	ASSERT_EQ(DATA, res2);
}

}
}

