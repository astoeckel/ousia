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

#include <core/parser/ParserContext.hpp>

#include "ParserStateHandler.hpp"

namespace ousia {

/* Class ParserStatedata */

ParserStatedata::ParserStatedata(ParserContext &ctx,
                                 ParserStateCallbacks &callbacks,
                                 std::string name, const ParserState &state,
                                 const ParserState &parentState,
                                 const SourceLocation location)
    : ctx(ctx),
      callbacks(callbacks),
      name(std::move(name)),
      state(state),
      parentState(parentState),
      location(location){};

/* Class ParserStateHandler */

ParserStateHandler::ParserStateHandler(const ParserStatedata &data) : data(data)
{
}

ParserContext &ParserStateHandler::context() { return data.ctx; }

const std::string &ParserStateHandler::name() { return data.name; }

ParserScope &ParserStateHandler::scope() { return data.ctx.getScope(); }

Manager &ParserStateHandler::manager() { return data.ctx.getManager(); }

Logger &ParserStateHandler::logger() { return data.ctx.getLogger(); }

Rooted<Project> ParserStateHandler::project() { return data.ctx.getProject(); }

const ParserState &ParserStateHandler::state() { return data.state; }

SourceLocation ParserStateHandler::location() { return data.location; }

void ParserStateHandler::setWhitespaceMode(WhitespaceMode whitespaceMode)
{
	data.callbacks.setWhitespaceMode(whitespaceMode);
}

void ParserStateHandler::setDataType(VariantType type)
{
	data.callbacks.setDataType(type);
}

bool ParserStateHandler::supportsToken(const std::string &token)
{
	return data.callbacks.supportsToken(token);
}

void ParserStateHandler::registerToken(const std::string &token)
{
	data.callbacks.registerToken(token);
}

void ParserStateHandler::unregisterToken(const std::string &token)
{
	data.callbacks.unregisterToken(token);
}

void ParserStateHandler::data(const std::string &data, int field)
{
	if (Utils::hasNonWhitepaceChar(data)) {
		logger().error("Expected command but found character data.");
	}
}

/* Class DefaultParserStateHandler */

void DefaultParserStateHandler::start(Variant::mapType &args) {}

void DefaultParserStateHandler::end() {}

ParserStateHandler *DefaultParserStateHandler::create(const data &data)
{
	return new DefaultHandler{data};
}
}

