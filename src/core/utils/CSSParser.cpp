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
#include "Tokenizer.hpp"

#include "CSSParser.hpp"

namespace ousia {
namespace utils {

static const int CURLY_OPEN = 1;
static const int CURLY_CLOSE = 2;
static const int COLON = 3;
static const int SEMICOLON = 4;
static const int HASH = 5;
static const int BRACKET_OPEN = 6;
static const int BRACKET_CLOSE = 7;
static const int COMMENT_OPEN = 8;
static const int COMMENT_CLOSE = 9;

static const TokenTreeNode CSS_ROOT{{{"{", CURLY_OPEN},
                                     {"}", CURLY_CLOSE},
                                     {":", COLON},
                                     {";", SEMICOLON},
                                     {"#", HASH},
                                     {"[", BRACKET_OPEN},
                                     {"]", BRACKET_CLOSE},
                                     {"/*", COMMENT_OPEN},
                                     {"*/", COMMENT_CLOSE}}};

StyleNode CSSParser::parse(BufferedCharReader &input) {
	Tokenizer tokenizer {input, CSS_ROOT};
	//TODO: implement
	
}



}
}
