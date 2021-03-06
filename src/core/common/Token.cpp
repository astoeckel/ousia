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

#include "Token.hpp"

namespace ousia {

std::string Token::name() const
{
	if (isSpecial()) {
		return specialName(id);
	}
	return content;
}

const char* Token::specialName(TokenId id)
{
	switch (id) {
		case Tokens::Newline:
			return "newline";
		case Tokens::Paragraph:
			return "paragraph";
		case Tokens::Section:
			return "section";
		case Tokens::Indent:
			return "indent";
		case Tokens::Dedent:
			return "dedent";
	}
	return "";
}


}

