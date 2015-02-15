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

#include <core/parser/generic/ParserStateCallbacks.hpp>
#include <core/parser/generic/ParserStateStack.hpp>

#include "OsdmParser.hpp"
#include "OsdmStreamParser.hpp"

namespace ousia {

namespace {

/**
 * The OsdmParserImplementation class contains the actual implementation of the
 * parsing process and is created in the "doParse" function of the OsdmParser.
 
 */
class OsdmParserImplementation : public ParserStateCallbacks {
private:
	/**
	 * OsdmStreamParser instance.
	 */
	OsdmStreamParser parser;

	/**
	 * Instance of the ParserStateStack.
	 */
	ParserStateStack stack;

public:
	OsdmParserImplementation parser(reader, ctx) : parser(reader), stack(ctx, std::multimap)
};
}

void OsdmParser::doParse(CharReader &reader, ParserContext &ctx)
{
	OsdmParserImplementation parser(reader, ctx);
	parser.parse();
}

}
