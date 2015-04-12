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

#ifndef _OUSIA_TOKENIZED_DATA_TEST_UTILS_HPP_
#define _OUSIA_TOKENIZED_DATA_TEST_UTILS_HPP_

namespace ousia {

inline void assertToken(TokenizedDataReader &reader, TokenId id,
                        const std::string &text,
                        const TokenSet &tokens = TokenSet{},
                        WhitespaceMode mode = WhitespaceMode::TRIM,
                        SourceOffset start = InvalidSourceOffset,
                        SourceOffset end = InvalidSourceOffset,
                        SourceId sourceId = InvalidSourceId,
                        bool endAtWhitespace = false)
{
	Token token;
	ASSERT_TRUE(reader.read(token, tokens, mode, endAtWhitespace));
	EXPECT_EQ(id, token.id);
	EXPECT_EQ(text, token.content);
	if (start != InvalidSourceOffset) {
		EXPECT_EQ(start, token.getLocation().getStart());
	}
	if (end != InvalidSourceOffset) {
		EXPECT_EQ(end, token.getLocation().getEnd());
	}
	EXPECT_EQ(sourceId, token.getLocation().getSourceId());
}

inline void assertText(TokenizedDataReader &reader, const std::string &text,
                       const TokenSet &tokens = TokenSet{},
                       WhitespaceMode mode = WhitespaceMode::TRIM,
                       SourceOffset start = InvalidSourceOffset,
                       SourceOffset end = InvalidSourceOffset,
                       SourceId id = InvalidSourceId)
{
	assertToken(reader, Tokens::Data, text, tokens, mode, start, end, id);
}

inline void assertTextEndAtWhitespace(
    TokenizedDataReader &reader, const std::string &text,
    const TokenSet &tokens = TokenSet{},
    WhitespaceMode mode = WhitespaceMode::TRIM,
    SourceOffset start = InvalidSourceOffset,
    SourceOffset end = InvalidSourceOffset, SourceId id = InvalidSourceId)
{
	assertToken(reader, Tokens::Data, text, tokens, mode, start, end, id, true);
}

inline void assertEnd(TokenizedDataReader &reader)
{
	Token token;
	ASSERT_TRUE(reader.atEnd());
	ASSERT_FALSE(reader.read(token));
}
}

#endif /* _OUSIA_TOKENIZED_DATA_TEST_UTILS_HPP_ */

