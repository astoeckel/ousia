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

#include <memory>
#include <string>
#include <unordered_map>

#include <core/common/CharReader.hpp>

#include "DynamicTokenizer.hpp"

namespace ousia {

/**
 * The TokenDescriptor class is a simple wrapper around a standard string
 * containing the character sequence of the token.
 */
class TokenDescriptor {
	/**
	 * The character sequence of the token.
	 */
	std::string str;

	/**
	 * Default constructor of the TokenDescriptor class. Used to describe
	 * special tokens.
	 */
	TokenDescriptor();

	/**
	 * Constructor initializing the character sequence of the token.
	 */
	TokenDescriptor(const std::string &str) : str(str) {}
};

/* Class DynamicTokenizer */

void DynamicTokenizer:setWhitespaceMode(WhitespaceMode mode)
{
	whitespaceMode = mode;
}

WhitespaceMode DynamicTokenizer::getWhitespaceMode()
{
	return whitespaceMode;
}


/* Constant initializations */

static const TokenDescriptor Empty;
static const TokenDescriptor Text;
static const TokenDescriptor* DynamicTokenizer::Empty = &Empty;
static const TokenDescriptor* DynamicTokenizer::Token = &Text;


}

