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

/* Test data */

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
		while (true) {
			// Advance the random seed
			v = v ^ (v >> B1);
			v = v ^ (v << B2);
			v = v ^ (v >> B3);

			// Replace \n and \r in order to avoid line break processing by the
			// CharReader
			char c = v & 0xFF;
			if (c != '\n' && c != '\r') {
				res.push_back(c);
				break;
			}
		}
	}
	return res;
}

// For performance tests only
// static constexpr size_t DATA_LENGTH = 16 * 1024 * 1024 + 795;
static constexpr size_t DATA_LENGTH = 256 * 1024 + 795;
static const std::vector<char> DATA = generateData(DATA_LENGTH);

/* Buffer Test */

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
	ASSERT_EQ(0U, buf.offset(cursor));

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
	ASSERT_EQ(testStr, res);
}

TEST(Buffer, cursorManagement)
{
	Buffer buf{""};

	Buffer::CursorId c1 = buf.createCursor();
	Buffer::CursorId c2 = buf.createCursor();
	Buffer::CursorId c3 = buf.createCursor();

	ASSERT_EQ(0U, c1);
	ASSERT_EQ(1U, c2);
	ASSERT_EQ(2U, c3);

	buf.deleteCursor(c2);
	Buffer::CursorId c4 = buf.createCursor();
	ASSERT_EQ(1U, c4);
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

TEST(Buffer, moveCursor)
{
	std::string testStr{"test1 test2 test3"};

	// Create buffer with the test string
	char c;
	Buffer buf{testStr};
	Buffer::CursorId cursor = buf.createCursor();

	// Read the first six characters with cursor one
	{
		std::string res;
		for (int i = 0; i < 6; i++) {
			if (buf.read(cursor, c)) {
				res.append(&c, 1);
			}
		}
		ASSERT_EQ("test1 ", res);
	}

	// Move six bytes backward
	ASSERT_EQ(-6, buf.moveCursor(cursor, -6));
	{
		std::string res;
		for (int i = 0; i < 6; i++) {
			if (buf.read(cursor, c)) {
				res.append(&c, 1);
			}
		}
		ASSERT_EQ("test1 ", res);
	}

	// Move more than six bytes backward
	ASSERT_EQ(-6, buf.moveCursor(cursor, -1000));
	{
		std::string res;
		for (int i = 0; i < 6; i++) {
			if (buf.read(cursor, c)) {
				res.append(&c, 1);
			}
		}
		ASSERT_EQ("test1 ", res);
	}

	// Move six bytes forward
	ASSERT_EQ(6, buf.moveCursor(cursor, 6));
	{
		std::string res;
		for (int i = 0; i < 6; i++) {
			if (buf.read(cursor, c)) {
				res.append(&c, 1);
			}
		}
		ASSERT_EQ("test3", res);
	}
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
	state.offs = tar;
	return res;
}

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
	ASSERT_EQ(0U, buf.offset(cur2));

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

TEST(Buffer, streamTwoCursorsMovingInterleaved)
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

		// Move cur2 120 bytes backward and read the content again
		res2.resize(res2.size() - 120);
		ASSERT_EQ(-120, buf.moveCursor(cur2, -120));
		for (int i = 0; i < 120; i++) {
			if (buf.read(cur2, c)) {
				res2.push_back(c);
			}
		}

		// Move cur1 60 bytes forward and backward
		buf.moveCursor(cur1, -buf.moveCursor(cur1, 60));

		// Make sure the cursor position is correct
		ASSERT_EQ(res1.size(), buf.offset(cur1));
		ASSERT_EQ(res2.size(), buf.offset(cur2));
	}

	ASSERT_EQ(DATA_LENGTH, buf.offset(cur1));
	ASSERT_EQ(DATA_LENGTH, buf.offset(cur2));

	// The read data and the original data must be equal
	ASSERT_EQ(DATA, res1);
	ASSERT_EQ(DATA, res2);
}

TEST(Buffer, streamMoveForward)
{
	VectorReadState state(DATA);

	std::vector<char> partialData;
	partialData.resize(100);
	std::copy(DATA.end() - partialData.size(), DATA.end(), partialData.begin());

	Buffer buf{readFromVector, &state};
	Buffer::CursorId cursor = buf.createCursor();
	ASSERT_EQ(ssize_t(DATA_LENGTH) - 100,
	          buf.moveCursor(cursor, DATA_LENGTH - 100));

	char c;
	std::vector<char> res;
	while (buf.read(cursor, c)) {
		res.push_back(c);
	}
	ASSERT_EQ(partialData, res);
}

/* CharReader Test */

TEST(CharReader, simpleRead)
{
	std::string testStr{"this is a test"};
	char c;

	// Feed a test string into the reader
	CharReader reader{testStr};

	// Try to read the test string
	std::string res;
	while (!reader.atEnd()) {
		ASSERT_TRUE(reader.read(c));
		res.append(&c, 1);
	}

	// The two strings must equal
	ASSERT_EQ(testStr, res);

	// We must now be at line 1, column 15
	ASSERT_EQ(1U, reader.getLine());
	ASSERT_EQ(testStr.size() + 1, reader.getColumn());

	// If we call either read or peek, false is returned
	ASSERT_FALSE(reader.read(c));
	ASSERT_FALSE(reader.peek(c));
}

TEST(CharReader, simplePeek)
{
	std::string testStr{"this is a test"};
	char c;

	// Feed a test string into the reader
	CharReader reader{testStr};

	// Try to read the test string
	std::string res;
	while (reader.peek(c)) {
		res.append(&c, 1);
	}

	// Peeking does not trigger the "atEnd" flag
	ASSERT_FALSE(reader.atEnd());

	// The two strings must equal
	ASSERT_EQ(testStr, res);

	// We must now be at line 1, column 1 and NOT at the end of the stream
	ASSERT_EQ(1U, reader.getLine());
	ASSERT_EQ(1U, reader.getColumn());
	ASSERT_FALSE(reader.atEnd());

	// If we consume the peek, we must be at line 1, column 15 and we should be
	// at the end of the stream
	reader.consumePeek();
	ASSERT_EQ(1U, reader.getLine());
	ASSERT_EQ(testStr.size() + 1, reader.getColumn());
	ASSERT_TRUE(reader.atEnd());

	// If we call either read or peek, false is returned
	ASSERT_FALSE(reader.read(c));
	ASSERT_FALSE(reader.peek(c));
}

TEST(CharReader, rowColumnCounter)
{
	// Feed a test string into the reader
	CharReader reader{"1\n\r2\n3\r\n\n4"};

	// We should currently be in line 1, column 1
	ASSERT_EQ(1U, reader.getLine());
	ASSERT_EQ(1U, reader.getColumn());

	// Read two characters
	char c;
	for (int i = 0; i < 2; i++)
		reader.read(c);
	ASSERT_EQ(2U, reader.getLine());
	ASSERT_EQ(1U, reader.getColumn());

	// Read two characters
	for (int i = 0; i < 2; i++)
		reader.read(c);
	ASSERT_EQ(3U, reader.getLine());
	ASSERT_EQ(1U, reader.getColumn());

	// Read three characters
	for (int i = 0; i < 3; i++)
		reader.read(c);
	ASSERT_EQ(5U, reader.getLine());
	ASSERT_EQ(1U, reader.getColumn());
}

TEST(CharReader, rowColumnCounterTest)
{
	// Feed a test string into the reader
	CharReader reader{"1\n\r2\n3\r\n\n4", 4, 10};

	// We should currently be in line 1, column 1
	ASSERT_EQ(4U, reader.getLine());
	ASSERT_EQ(10U, reader.getColumn());

	// Read two characters
	char c;
	for (int i = 0; i < 2; i++)
		reader.read(c);
	ASSERT_EQ(5U, reader.getLine());
	ASSERT_EQ(1U, reader.getColumn());

	// Read two characters
	for (int i = 0; i < 2; i++)
		reader.read(c);
	ASSERT_EQ(6U, reader.getLine());
	ASSERT_EQ(1U, reader.getColumn());

	// Read three characters
	for (int i = 0; i < 3; i++)
		reader.read(c);
	ASSERT_EQ(8U, reader.getLine());
	ASSERT_EQ(1U, reader.getColumn());
}

TEST(CharReader, linebreakSubstitution)
{
	// Feed a test string into the reader and read all characters back
	CharReader reader{"this\n\ris\n\rjust\na test\r\n\rtest\n\r"};
	std::string res;
	char c;
	while (reader.read(c)) {
		res.append(&c, 1);
	}

	// Test for equality
	ASSERT_EQ("this\nis\njust\na test\n\ntest\n", res);
}

TEST(CharReader, rowColumnCounterUTF8)
{
	// Feed a test string with some umlauts into the reader
	CharReader reader{"\x61\xc3\x96\xc3\x84\xc3\x9c\xc3\x9f"};

	// Read all bytes
	char c;
	while (reader.read(c)) {
		// Do nothing
	}

	// The sequence above equals 5 UTF-8 characters (so after reading all the
	// cursor is at position 6)
	ASSERT_EQ(1U, reader.getLine());
	ASSERT_EQ(6U, reader.getColumn());
}

TEST(CharReader, stream)
{
	// Copy the test data to a string stream
	std::stringstream ss;
	std::copy(DATA.begin(), DATA.end(), std::ostream_iterator<char>(ss));

	// Read the data back from the stream
	std::vector<char> res;
	char c;
	CharReader reader{ss};
	while (reader.read(c)) {
		res.push_back(c);
	}
	ASSERT_EQ(DATA_LENGTH, res.size());
	ASSERT_EQ(DATA, res);
}

TEST(CharReader, fork)
{
	std::string testStr{"first line\n\n\rsecond line\n\rlast line"};
	//                   0123456789 0   123456789012   3456789012
	//                   0         1             2              3

	char c;
	CharReader reader{testStr};

	// Read a few characters
	for (int i = 0; i < 4; i++)
		reader.read(c);

	// Peek a few characters
	for (int i = 4; i < 7; i++)
		reader.peek(c);

	// Fork the reader
	{
		CharReaderFork fork = reader.fork();

		ASSERT_EQ(1U, fork.getLine());
		ASSERT_EQ(5U, fork.getColumn());

		fork.peek(c);
		ASSERT_EQ('i', c);

		fork.read(c);
		ASSERT_EQ('t', c);

		ASSERT_EQ(1U, fork.getLine());
		ASSERT_EQ(6U, fork.getColumn());

		ASSERT_EQ(1U, reader.getLine());
		ASSERT_EQ(5U, reader.getColumn());

		reader.read(c);
		reader.read(c);
		ASSERT_EQ(' ', c);

		fork.commit();
	}
	ASSERT_EQ(1U, reader.getLine());
	ASSERT_EQ(6U, reader.getColumn());
}

TEST(CharReaderTest, context)
{
	std::string testStr{"first line\n\n\rsecond line\n\rlast line"};
	//                   0123456789 0   123456789012   3456789012
	//                   0         1             2              3

	// Retrieval at beginning of stream
	{
		CharReader reader{testStr};
		CharReader::Context ctx = reader.getContext(80);
		ASSERT_EQ("first line", ctx.line);
		ASSERT_EQ(0U, ctx.relPos);
		ASSERT_FALSE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}

	// Retrieval in middle of line
	{
		CharReader reader{testStr};
		CharReader::Context ctx = reader.getContext(80);

		char c;
		for (int i = 0; i < 5; i++)
			reader.read(c);

		ASSERT_EQ("first line", ctx.line);
		ASSERT_EQ(0U, ctx.relPos);
		ASSERT_FALSE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}

	// Retrieval in whitespace sequence
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 11; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(80);
		ASSERT_EQ("first line", ctx.line);
		ASSERT_EQ(10U, ctx.relPos);
		ASSERT_FALSE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}

	// Truncation of text
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 5; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(3);
		ASSERT_EQ("t l", ctx.line);
		ASSERT_EQ(1U, ctx.relPos);
		ASSERT_TRUE(ctx.truncatedStart);
		ASSERT_TRUE(ctx.truncatedEnd);
	}

	// Second line
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 12; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(80);
		ASSERT_EQ("second line", ctx.line);
		ASSERT_EQ(0U, ctx.relPos);
		ASSERT_FALSE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}

	// End of second line
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 23; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(80);
		ASSERT_EQ("second line", ctx.line);
		ASSERT_EQ(11U, ctx.relPos);
		ASSERT_FALSE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}

	// Last line
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 24; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(80);
		ASSERT_EQ("last line", ctx.line);
		ASSERT_EQ(0U, ctx.relPos);
		ASSERT_FALSE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}

	// Middle of last line
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 28; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(80);
		ASSERT_EQ("last line", ctx.line);
		ASSERT_EQ(4U, ctx.relPos);
		ASSERT_FALSE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}

	// Middle of last line truncated
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 28; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(3);
		ASSERT_EQ("t l", ctx.line);
		ASSERT_EQ(1U, ctx.relPos);
		ASSERT_TRUE(ctx.truncatedStart);
		ASSERT_TRUE(ctx.truncatedEnd);
	}

	// End of stream
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 100; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(80);
		ASSERT_EQ("last line", ctx.line);
		ASSERT_EQ(9U, ctx.relPos);
		ASSERT_FALSE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}

	// End of stream truncated
	{
		CharReader reader{testStr};

		char c;
		for (int i = 0; i < 100; i++)
			reader.read(c);

		CharReader::Context ctx = reader.getContext(4);
		ASSERT_EQ("line", ctx.line);
		ASSERT_EQ(4U, ctx.relPos);
		ASSERT_TRUE(ctx.truncatedStart);
		ASSERT_FALSE(ctx.truncatedEnd);
	}
}
}
}

