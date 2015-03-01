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

#include <core/common/Logger.hpp>

#include <core/model/Document.hpp>

#include <core/parser/stack/GenericParserStates.hpp>
#include <core/parser/stack/Stack.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/parser/ParserScope.hpp>

#include "OsmlParser.hpp"
#include "OsmlStreamParser.hpp"

namespace ousia {

using namespace parser_stack;

/**
 * The OsmlParserImplementation class contains the actual implementation of the
 * parsing process and is created in the "doParse" function of the OsmlParser.
 */
class OsmlParserImplementation {
private:
	/**
	 * Reference at the logger.
	 */
	Logger &logger;

	/**
	 * Reference at the parser context.
	 */
	ParserContext &ctx;

	/**
	 * OsmlStreamParser instance responsible for converting the input stream
	 * into a series of osml events that are relayed to the Stack class.
	 */
	OsmlStreamParser parser;

	/**
	 * Pushdown automaton responsible for converting the osml events into an
	 * actual Node tree.
	 */
	Stack stack;

public:
	/**
	 * Constructor of the OsmlParserImplementation class.
	 *
	 * @param reader is a reference to the CharReader instance from which the
	 * osml should be read.
	 * @param ctx is a reference to the ParserContext instance that should be
	 * used.
	 */
	OsmlParserImplementation(CharReader &reader, ParserContext &ctx)
	    : logger(ctx.getLogger()),
	      ctx(ctx),
	      parser(reader, logger),
	      stack(ctx, GenericParserStates)
	{
	}

	/**
	 * Starts the actual parsing process.
	 */
	void parse()
	{
		// Flag set to true if a "document" element needs to be created
		bool needsDocument = ctx.getScope().select<Document>() == nullptr;
		while (true) {
			OsmlStreamParser::State state = parser.parse();
			logger.setDefaultLocation(parser.getLocation());
			switch (state) {
				case OsmlStreamParser::State::COMMAND_START: {
					// Implicitly create a "document" element if the first
					// command is not any other top-level command
					if (needsDocument) {
						const std::string &cmd =
						    parser.getCommandName().asString();
						if (cmd != "typesystem" && cmd != "document" &&
						    cmd != "domain") {
							stack.commandStart("document", Variant::mapType{},
							                   false);
						}
						needsDocument = false;
					}
					stack.commandStart(parser.getCommandName(),
					                   parser.getCommandArguments().asMap(),
					                   parser.inRangeCommand());
					break;
				}
				case OsmlStreamParser::State::RANGE_END:
					stack.rangeEnd();
					break;
				case OsmlStreamParser::State::ANNOTATION_START:
					stack.annotationStart(parser.getCommandName(),
					                      parser.getCommandArguments().asMap(),
					                      parser.inRangeCommand());
					break;
				case OsmlStreamParser::State::ANNOTATION_END: {
					Variant elementName = Variant::fromString(std::string{});
					const auto &args = parser.getCommandArguments().asMap();
					auto it = args.find("name");
					if (it != args.end()) {
						elementName = it->second;
					}
					stack.annotationEnd(parser.getCommandName(), elementName);
					break;
				}
				case OsmlStreamParser::State::FIELD_START:
					stack.fieldStart(parser.inDefaultField());
					break;
				case OsmlStreamParser::State::FIELD_END:
					stack.fieldEnd();
					break;
				case OsmlStreamParser::State::DATA:
					stack.data(parser.getData());
					break;
				case OsmlStreamParser::State::END:
					return;
			}
		}
	}
};

void OsmlParser::doParse(CharReader &reader, ParserContext &ctx)
{
	OsmlParserImplementation parser(reader, ctx);
	parser.parse();
}
}
