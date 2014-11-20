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

#include "BufferedCharReader.hpp"
#include "CodeTokenizer.hpp"
#include "Tokenizer.hpp"

#include "CSSParser.hpp"

namespace ousia {
namespace utils {

// CSS code tokens
static const int CURLY_OPEN = 1;
static const int CURLY_CLOSE = 2;
static const int COLON = 3;
static const int SEMICOLON = 4;
static const int HASH = 5;
static const int BRACKET_OPEN = 6;
static const int BRACKET_CLOSE = 7;
static const int PAREN_OPEN = 8;
static const int PAREN_CLOSE = 9;
// comments
static const int COMMENT = 100;
static const int COMMENT_OPEN = 101;
static const int COMMENT_CLOSE = 102;
// strings
static const int STRING = 200;
static const int SINGLE_QUOTE = 201;
static const int DOUBLE_QUOTE = 202;
static const int ESCAPE = 203;
// general syntax
static const int LINEBREAK = 300;

static const TokenTreeNode CSS_ROOT{{{"{", CURLY_OPEN},
                                     {"}", CURLY_CLOSE},
                                     {":", COLON},
                                     {";", SEMICOLON},
                                     {"#", HASH},
                                     {"[", BRACKET_OPEN},
                                     {"]", BRACKET_CLOSE},
                                     {"(", PAREN_OPEN},
                                     {")", PAREN_CLOSE},
                                     {"/*", COMMENT_OPEN},
                                     {"*/", COMMENT_CLOSE},
                                     {"\\", ESCAPE},
                                     {"\''", SINGLE_QUOTE},
                                     {"\"", DOUBLE_QUOTE},
                                     {"\n", LINEBREAK}}};

static const std::map<int, CodeTokenDescriptor> CSS_DESCRIPTORS = {
    {COMMENT_OPEN, {CodeTokenMode::BLOCK_COMMENT_START, COMMENT}},
    {COMMENT_CLOSE, {CodeTokenMode::BLOCK_COMMENT_END, COMMENT}},
    {SINGLE_QUOTE, {CodeTokenMode::STRING_START_END, STRING}},
    {DOUBLE_QUOTE, {CodeTokenMode::STRING_START_END, STRING}},
    {ESCAPE, {CodeTokenMode::ESCAPE, ESCAPE}},
    {LINEBREAK, {CodeTokenMode::LINEBREAK, LINEBREAK}}};

StyleNode CSSParser::parse(BufferedCharReader &input)
{
	CodeTokenizer tokenizer{input, CSS_ROOT, CSS_DESCRIPTORS};
	tokenizer.ignoreComments = true;
	// TODO: implement
}
}
}
