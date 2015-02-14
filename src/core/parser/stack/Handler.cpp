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

#include "Callbacks.hpp"
#include "Handler.hpp"
#include "State.hpp"

namespace ousia {
namespace parser_stack {

/* Class HandlerData */

HandlerData::HandlerData(ParserContext &ctx, Callbacks &callbacks,
                         std::string name, const State &state,
                         const SourceLocation &location)
    : ctx(ctx),
      callbacks(callbacks),
      name(std::move(name)),
      state(state),
      location(location)
{
}

/* Class Handler */

Handler::Handler(const HandlerData &internalData) : internalData(internalData)
{
}

Handler::~Handler() {}

ParserContext &Handler::context() { return internalData.ctx; }

const std::string &Handler::name() { return internalData.name; }

ParserScope &Handler::scope() { return internalData.ctx.getScope(); }

Manager &Handler::manager() { return internalData.ctx.getManager(); }

Logger &Handler::logger() { return internalData.ctx.getLogger(); }

const State &Handler::state() { return internalData.state; }

SourceLocation Handler::location() { return internalData.location; }

void Handler::setWhitespaceMode(WhitespaceMode whitespaceMode)
{
	internalData.callbacks.setWhitespaceMode(whitespaceMode);
}

void Handler::registerToken(const std::string &token)
{
	internalData.callbacks.registerToken(token);
}

void Handler::unregisterToken(const std::string &token)
{
	internalData.callbacks.unregisterToken(token);
}

/* Class DefaultHandler */

/*void DefaultHandler::start(Variant::mapType &args) {}

void DefaultHandler::end() {}

Handler *DefaultHandler::create(const data &data)
{
    return new DefaultHandler{data};
}*/
}
}

