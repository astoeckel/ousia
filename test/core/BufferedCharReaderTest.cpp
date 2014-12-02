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

#include <core/BufferedCharReader.hpp>

namespace ousia{

TEST(BufferedCharReaderTest, SimpleReadTest)
{
	std::string testStr{"this is a test"};
	char c;

	// Feed a test string into the reader
	BufferedCharReader reader{testStr};

	// Try to read the test string
	std::string res;
	while (!reader.atEnd()) {
		ASSERT_TRUE(reader.read(&c));
		res.append(&c, 1);
	}

	// The two strings must equal
	ASSERT_STREQ(testStr.c_str(), res.c_str()) ;

	// We must now be at line 1, column 15
	ASSERT_EQ(1, reader.getLine());
	ASSERT_EQ(testStr.size() + 1, reader.getColumn());

	// If we call either read or peek, false is returned
	ASSERT_FALSE(reader.read(&c));
	ASSERT_FALSE(reader.peek(&c));
}

TEST(BufferedCharReaderTest, SimplePeekTest)
{
	std::string testStr{"this is a test"};
	char c;

	// Feed a test string into the reader
	BufferedCharReader reader{testStr};

	// Try to read the test string
	std::string res;
	while (reader.peek(&c)) {
		res.append(&c, 1);
	}

	// Peeking does not trigger the "atEnd" flag
	ASSERT_FALSE(reader.atEnd());

	// The two strings must equal
	ASSERT_STREQ(testStr.c_str(), res.c_str());

	// We must now be at line 1, column 1 and NOT at the end of the stream
	ASSERT_EQ(1, reader.getLine());
	ASSERT_EQ(1, reader.getColumn());
	ASSERT_FALSE(reader.atEnd());

	// If we consume the peek, we must be at line 1, column 15 and we should be
	// at the end of the stream
	reader.consumePeek();
	ASSERT_EQ(1, reader.getLine());
	ASSERT_EQ(testStr.size() + 1, reader.getColumn());
	ASSERT_TRUE(reader.atEnd());

	// If we call either read or peek, false is returned
	ASSERT_FALSE(reader.read(&c));
	ASSERT_FALSE(reader.peek(&c));
}

TEST(BufferedCharReaderTest, SplittedPeakTest)
{
	std::string testStr{"this is a test"};
	char c;

	// Feed a test string into the reader
	BufferedCharReader reader;

	// Try to peek the test string, feed char after char into the reader
	std::string res;
	for (unsigned int i = 0; i < testStr.length(); i++) {
		reader.feed(std::string(&testStr[i], 1));
		while (reader.peek(&c)) {
			res.append(&c, 1);
		}
	}
	reader.close();

	// Consume the peeked data
	ASSERT_FALSE(reader.atEnd());
	reader.consumePeek();
	ASSERT_TRUE(reader.atEnd());

	// The two strings must equal
	ASSERT_STREQ(testStr.c_str(), res.c_str()) ;

	// We must now be at line 1, column 15
	ASSERT_EQ(1, reader.getLine());
	ASSERT_EQ(testStr.size() + 1, reader.getColumn());

	// If we call either read or peek, false is returned
	ASSERT_FALSE(reader.read(&c));
	ASSERT_FALSE(reader.peek(&c));
}

TEST(BufferedCharReaderTest, RowColumnCounterTest)
{
	// Feed a test string into the reader
	BufferedCharReader reader{"1\n\r2\n3\r\n\n4"};

	// We should currently be in line 1, column 1
	ASSERT_EQ(1, reader.getLine());
	ASSERT_EQ(1, reader.getColumn());

	// Read two characters
	char c;
	for (int i = 0; i < 2; i++) reader.read(&c);
	ASSERT_EQ(2, reader.getLine());
	ASSERT_EQ(1, reader.getColumn());

	// Read two characters
	for (int i = 0; i < 2; i++) reader.read(&c);
	ASSERT_EQ(3, reader.getLine());
	ASSERT_EQ(1, reader.getColumn());

	// Read three characters
	for (int i = 0; i < 3; i++) reader.read(&c);
	ASSERT_EQ(5, reader.getLine());
	ASSERT_EQ(1, reader.getColumn());
}

TEST(BufferedCharReaderTest, LinebreakSubstitutionTest)
{
	// Feed a test string into the reader
	BufferedCharReader reader{"this\n\ris\n\rjust\na test\r\n\rtest\n\r"};

	// Read all characters from the test string
	std::string res;
	char c;
	while (reader.read(&c)) {
		res.append(&c, 1);
	}

	// Test for equality
	ASSERT_STREQ("this\nis\njust\na test\n\ntest\n", res.c_str());
}

TEST(BufferedCharReaderTest, RowColumnCounterUTF8Test)
{
	// Feed a test string with some umlauts into the reader
	BufferedCharReader reader{"\x61\xc3\x96\xc3\x84\xc3\x9c\xc3\x9f"};

	// Read all bytes
	char c;
	while (reader.read(&c));

	// The sequence above equals 5 UTF-8 characters (so after reading all the
	// cursor is at position 6)
	ASSERT_EQ(1, reader.getLine());
	ASSERT_EQ(6, reader.getColumn());
}

}

