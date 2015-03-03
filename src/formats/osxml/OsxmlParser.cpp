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

#include <core/common/Variant.hpp>
#include <core/common/CharReader.hpp>
#include <core/parser/stack/Callbacks.hpp>
#include <core/parser/stack/GenericParserStates.hpp>
#include <core/parser/stack/Stack.hpp>
#include <core/parser/ParserContext.hpp>

#include "OsxmlEventParser.hpp"
#include "OsxmlParser.hpp"

namespace ousia {

using namespace parser_stack;

/**
 * Class containing the actual OsxmlParser implementation.
 */
class OsxmlParserImplementation : public OsxmlEvents, ParserCallbacks {
private:
	/**
	 * Actual xml parser -- converts the xml stream into a set of events.
	 */
	OsxmlEventParser parser;

	/**
	 * Pushdown automaton responsible for converting the xml events into an
	 * actual Node tree.
	 */
	Stack stack;

public:
	/**
	 * Constructor of the OsxmlParserImplementation class.
	 *
	 * @param reader is a reference to the CharReader instance from which the
	 * XML should be read.
	 * @param ctx is a reference to the ParserContext instance that should be
	 * used.
	 */
	OsxmlParserImplementation(CharReader &reader, ParserContext &ctx)
	    : parser(reader, *this, ctx.getLogger()),
	      stack(*this, ctx, GenericParserStates)
	{
	}

	/**
	 * Starts the actual parsing process.
	 */
	void parse() { parser.parse(); }

	void commandStart(const Variant &name,
	                  const Variant::mapType &args) override
	{
		stack.commandStart(name, args, true);
	}

	void annotationStart(const Variant &name,
	                     const Variant::mapType &args) override
	{
		stack.annotationStart(name, args, true);
	}

	void annotationEnd(const Variant &className,
	                   const Variant &elementName) override
	{
		stack.annotationEnd(className, elementName);
	}

	void rangeEnd() override { stack.rangeEnd(); }

	void data(const TokenizedData &data) override { stack.data(data); }

	TokenId registerToken(const std::string &token) override
	{
		return Tokens::Empty;
	}

	void unregisterToken(TokenId id) override
	{
		// Do nothing here
	}
};

/* Class OsxmlParser */

void OsxmlParser::doParse(CharReader &reader, ParserContext &ctx)
{
	OsxmlParserImplementation impl(reader, ctx);
	impl.parse();
}
}

