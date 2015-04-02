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

#include <core/parser/utils/TokenizedData.hpp>

#include "TokenizedDataTestUtils.hpp"

namespace ousia {

TEST(TokenizedData, dataWhitespacePreserve)
{
	TokenizedData data;
	ASSERT_EQ(16U, data.append(" test1   test2  "));
	//                          0123456789012345
	//                          0         1

	TokenizedDataReader reader = data.reader();
	assertText(reader, " test1   test2  ", TokenSet{}, WhitespaceMode::PRESERVE,
	           0, 16);
	assertEnd(reader);
}

TEST(TokenizedData, dataWhitespaceTrim)
{
	TokenizedData data;
	ASSERT_EQ(16U, data.append(" test1   test2  "));
	//                          0123456789012345
	//                          0         1

	TokenizedDataReader reader = data.reader();
	assertText(reader, "test1   test2", TokenSet{}, WhitespaceMode::TRIM, 1,
	           14);
	assertEnd(reader);
}

TEST(TokenizedData, dataWhitespaceCollapse)
{
	TokenizedData data;
	ASSERT_EQ(16U, data.append(" test1   test2  "));
	//                          0123456789012345
	//                          0         1

	TokenizedDataReader reader = data.reader();
	assertText(reader, "test1 test2", TokenSet{}, WhitespaceMode::COLLAPSE, 1,
	           14);
	assertEnd(reader);
}

TEST(TokenizedData, singleToken)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(5, 0, 2);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::COLLAPSE, 0, 2);
	assertEnd(reader);
}

TEST(TokenizedData, singleDisabledToken)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(5, 0, 2);

	TokenizedDataReader reader = data.reader();
	assertText(reader, "$$", TokenSet{}, WhitespaceMode::COLLAPSE, 0, 2);
	assertEnd(reader);
}

TEST(TokenizedData, dualToken)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(6, 0, 1);
	data.mark(5, 0, 2);
	data.mark(6, 1, 1);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5, 6}, WhitespaceMode::COLLAPSE, 0,
	            2);
	assertEnd(reader);
}

TEST(TokenizedData, dualTokenShorterEnabled)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(6, 0, 1);
	data.mark(5, 0, 2);
	data.mark(6, 1, 1);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 6, "$", TokenSet{6}, WhitespaceMode::COLLAPSE, 0, 1);
	assertToken(reader, 6, "$", TokenSet{6}, WhitespaceMode::COLLAPSE, 1, 2);
	assertEnd(reader);
}

TEST(TokenizedData, dualTokenLongerEnabled)
{
	TokenizedData data;
	ASSERT_EQ(2U, data.append("$$"));
	data.mark(6, 0, 1);
	data.mark(5, 0, 2);
	data.mark(6, 1, 1);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::COLLAPSE, 0, 2);
	assertEnd(reader);
}

TEST(TokenizedData, tokensAndDataPreserveWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(18U, data.append("$$ test    text $$"));
	//                          012345678901234567
	data.mark(5, 0, 2);
	data.mark(5, 2);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::PRESERVE, 0, 2);
	assertText(reader, " test    text ", TokenSet{5}, WhitespaceMode::PRESERVE,
	           2, 16);
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::PRESERVE, 16, 18);
	assertEnd(reader);
}

TEST(TokenizedData, tokensAndDataTrimWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(18U, data.append("$$ test    text $$"));
	//                          012345678901234567
	data.mark(5, 0, 2);
	data.mark(5, 2);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::TRIM, 0, 2);
	assertText(reader, "test    text", TokenSet{5}, WhitespaceMode::TRIM, 3,
	           15);
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::TRIM, 16, 18);
	assertEnd(reader);
}

TEST(TokenizedData, tokensAndDataCollapseWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(18U, data.append("$$ test    text $$"));
	//                          012345678901234567
	data.mark(5, 0, 2);
	data.mark(5, 2);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::COLLAPSE, 0, 2);
	assertText(reader, "test text", TokenSet{5}, WhitespaceMode::COLLAPSE, 3,
	           15);
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::COLLAPSE, 16, 18);
	assertEnd(reader);
}

TEST(TokenizedData, tokensAndWhitespacePreserveWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(8U, data.append("$$    $$"));
	//                         01234567
	data.mark(5, 0, 2);
	data.mark(5, 2);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::PRESERVE, 0, 2);
	assertText(reader, "    ", TokenSet{5}, WhitespaceMode::PRESERVE, 2, 6);
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::PRESERVE, 6, 8);
	assertEnd(reader);
}

TEST(TokenizedData, tokensAndWhitespaceTrimWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(8U, data.append("$$    $$"));
	//                         01234567
	data.mark(5, 0, 2);
	data.mark(5, 2);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::TRIM, 0, 2);
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::TRIM, 6, 8);
	assertEnd(reader);
}

TEST(TokenizedData, tokensAndWhitespaceCollapseWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(8U, data.append("$$    $$"));
	//                         01234567
	data.mark(5, 0, 2);
	data.mark(5, 2);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::COLLAPSE, 0, 2);
	assertToken(reader, 5, "$$", TokenSet{5}, WhitespaceMode::COLLAPSE, 6, 8);
	assertEnd(reader);
}

TEST(TokenizedData, appendChars)
{
	TokenizedData data;
	ASSERT_EQ(1U, data.append('t', 5, 7));
	ASSERT_EQ(2U, data.append('e', 7, 8));
	ASSERT_EQ(3U, data.append('s', 8, 10));
	ASSERT_EQ(4U, data.append('t', 10, 12));

	TokenizedDataReader reader = data.reader();
	assertText(reader, "test", TokenSet{5}, WhitespaceMode::COLLAPSE, 5, 12);
	assertEnd(reader);
}

TEST(TokenizedData, protectedWhitespace)
{
	TokenizedData data;
	ASSERT_EQ(4U, data.append("test", 10));
	ASSERT_EQ(11U, data.append("   test", 14, true));

	TokenizedDataReader reader = data.reader();
	assertText(reader, "test   test", TokenSet{5}, WhitespaceMode::COLLAPSE, 10,
	           21);
	assertEnd(reader);
}

TEST(TokenizedData, specialNewlineToken)
{
	TokenizedData data;
	data.append("a\nb\n   \nc\n");
	//           0 12 3456 78 9

	const TokenSet tokens{Tokens::Newline};

	TokenizedDataReader reader = data.reader();
	assertText(reader, "a", tokens, WhitespaceMode::COLLAPSE, 0, 1);
	assertToken(reader, Tokens::Newline, "\n", tokens, WhitespaceMode::COLLAPSE,
	            1, 2);
	assertText(reader, "b", tokens, WhitespaceMode::COLLAPSE, 2, 3);
	assertToken(reader, Tokens::Newline, "\n", tokens, WhitespaceMode::COLLAPSE,
	            3, 4);
	assertToken(reader, Tokens::Newline, "\n", tokens, WhitespaceMode::COLLAPSE,
	            7, 8);
	assertText(reader, "c", tokens, WhitespaceMode::COLLAPSE, 8, 9);
	assertToken(reader, Tokens::Newline, "\n", tokens, WhitespaceMode::COLLAPSE,
	            9, 10);
	assertEnd(reader);
}

TEST(TokenizedData, specialParagraphToken)
{
	TokenizedData data;
	data.append("a\nb\n   \nc\n");
	//           0 12 3456 78 9

	const TokenSet tokens{Tokens::Paragraph};

	TokenizedDataReader reader = data.reader();
	assertText(reader, "a b", tokens, WhitespaceMode::COLLAPSE, 0, 3);
	assertToken(reader, Tokens::Paragraph, "\n   \n", tokens,
	            WhitespaceMode::COLLAPSE, 3, 8);
	assertText(reader, "c", tokens, WhitespaceMode::COLLAPSE, 8, 9);
	assertEnd(reader);
}

TEST(TokenizedData, specialSectionToken)
{
	TokenizedData data;
	data.append("a\nb\n   \n  \t \n");
	//           0 12 3456 789 01 2
	//           0             1

	const TokenSet tokens{Tokens::Section};

	TokenizedDataReader reader = data.reader();
	assertText(reader, "a b", tokens, WhitespaceMode::COLLAPSE, 0, 3);
	assertToken(reader, Tokens::Section, "\n   \n  \t \n", tokens,
	            WhitespaceMode::COLLAPSE, 3, 13);
	assertEnd(reader);
}

TEST(TokenizedData, specialTokenPrecedence)
{
	TokenizedData data;
	data.append("a\nb\n\nc\n\n\nd");
	//           0 12 3 45 6 7 89

	const TokenSet tokens{Tokens::Newline, Tokens::Paragraph, Tokens::Section};

	TokenizedDataReader reader = data.reader();
	assertText(reader, "a", tokens, WhitespaceMode::COLLAPSE, 0, 1);
	assertToken(reader, Tokens::Newline, "\n", tokens, WhitespaceMode::COLLAPSE,
	            1, 2);
	assertText(reader, "b", tokens, WhitespaceMode::COLLAPSE, 2, 3);
	assertToken(reader, Tokens::Paragraph, "\n\n", tokens,
	            WhitespaceMode::COLLAPSE, 3, 5);
	assertText(reader, "c", tokens, WhitespaceMode::COLLAPSE, 5, 6);
	assertToken(reader, Tokens::Section, "\n\n\n", tokens,
	            WhitespaceMode::COLLAPSE, 6, 9);
	assertText(reader, "d", tokens, WhitespaceMode::COLLAPSE, 9, 10);
	assertEnd(reader);
}

TEST(TokenizedData, specialTokenPrecedence2)
{
	TokenizedData data;
	data.append("\nb\n\nc\n\n\n");
	//            0 12 3 45 6 7

	const TokenSet tokens{Tokens::Newline, Tokens::Paragraph, Tokens::Section};

	TokenizedDataReader reader = data.reader();
	assertToken(reader, Tokens::Newline, "\n", tokens, WhitespaceMode::COLLAPSE,
	            0, 1);
	assertText(reader, "b", tokens, WhitespaceMode::COLLAPSE, 1, 2);
	assertToken(reader, Tokens::Paragraph, "\n\n", tokens,
	            WhitespaceMode::COLLAPSE, 2, 4);
	assertText(reader, "c", tokens, WhitespaceMode::COLLAPSE, 4, 5);
	assertToken(reader, Tokens::Section, "\n\n\n", tokens,
	            WhitespaceMode::COLLAPSE, 5, 8);
	assertEnd(reader);
}

TEST(TokenizedData, specialTokenIndent)
{
	TokenizedData data;
	data.append("    test\n\ttest2\n        test3  \ttest4\ntest5");
	//           01234567 8 901234 5678901234567890 123456 789012
	//           0           1          2         3           4
	const TokenSet tokens{Tokens::Indent, Tokens::Dedent};

	TokenizedDataReader reader = data.reader();
	assertToken(reader, Tokens::Indent, "", tokens, WhitespaceMode::COLLAPSE,
	            4, 4);
	assertText(reader, "test", tokens, WhitespaceMode::COLLAPSE, 4, 8);
	assertToken(reader, Tokens::Indent, "", tokens, WhitespaceMode::COLLAPSE,
	            10, 10);
	assertText(reader, "test2 test3 test4", tokens, WhitespaceMode::COLLAPSE, 10, 37);
	assertToken(reader, Tokens::Dedent, "", tokens, WhitespaceMode::COLLAPSE,
	            37, 37);
	assertToken(reader, Tokens::Dedent, "", tokens, WhitespaceMode::COLLAPSE,
	            37, 37);
	assertText(reader, "test5", tokens, WhitespaceMode::COLLAPSE, 38, 43);
	assertEnd(reader);
}

TEST(TokenizedData, specialTokenIndent2)
{
	TokenizedData data;
	data.append("a\n\tb\n\t\tc\n\t\t\td\n\te\nf\n");
	//           0 1 23 4 5 67 8 9 0 12 3 45 67 8
	//           0                 1
	const TokenSet tokens{Tokens::Indent, Tokens::Dedent};

	TokenizedDataReader reader = data.reader();
	assertText(reader, "a", tokens, WhitespaceMode::COLLAPSE, 0, 1);
	assertToken(reader, Tokens::Indent, "", tokens, WhitespaceMode::COLLAPSE,
	            3, 3);
	assertText(reader, "b", tokens, WhitespaceMode::COLLAPSE, 3, 4);
	assertToken(reader, Tokens::Indent, "", tokens, WhitespaceMode::COLLAPSE,
	            7, 7);
	assertText(reader, "c", tokens, WhitespaceMode::COLLAPSE, 7, 8);
	assertToken(reader, Tokens::Indent, "", tokens, WhitespaceMode::COLLAPSE,
	            12, 12);
	assertText(reader, "d", tokens, WhitespaceMode::COLLAPSE, 12, 13);
	assertToken(reader, Tokens::Dedent, "", tokens, WhitespaceMode::COLLAPSE,
	            13, 13);
	assertToken(reader, Tokens::Dedent, "", tokens, WhitespaceMode::COLLAPSE,
	            13, 13);
	assertText(reader, "e", tokens, WhitespaceMode::COLLAPSE, 15, 16);
	assertToken(reader, Tokens::Dedent, "", tokens, WhitespaceMode::COLLAPSE,
	            16, 16);
	assertText(reader, "f", tokens, WhitespaceMode::COLLAPSE, 17, 18);
	assertEnd(reader);
}

TEST(TokenizedData, specialTokenIndentOverlap)
{
	TokenizedData data;
	data.append("    test\n\ttest2\n        test3  \ttest4\ntest5");
	//           01234567 8 901234 5678901234567890 123456 789012
	//           0           1          2         3           4
	const TokenSet tokens{Tokens::Indent, Tokens::Dedent, 5};

	data.mark(5, 4, 4);

	TokenizedDataReader reader = data.reader();
	assertToken(reader, Tokens::Indent, "", tokens, WhitespaceMode::COLLAPSE,
	            4, 4);
	assertToken(reader, 5, "test", tokens, WhitespaceMode::COLLAPSE, 4, 8);
	assertToken(reader, Tokens::Indent, "", tokens, WhitespaceMode::COLLAPSE,
	            10, 10);
	assertText(reader, "test2 test3 test4", tokens, WhitespaceMode::COLLAPSE, 10, 37);
	assertToken(reader, Tokens::Dedent, "", tokens, WhitespaceMode::COLLAPSE,
	            37, 37);
	assertToken(reader, Tokens::Dedent, "", tokens, WhitespaceMode::COLLAPSE,
	            37, 37);
	assertText(reader, "test5", tokens, WhitespaceMode::COLLAPSE, 38, 43);
	assertEnd(reader);
}

}

