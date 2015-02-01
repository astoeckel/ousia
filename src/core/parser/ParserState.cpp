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

#include "ParserState.hpp"

namespace ousia {

/* Class ParserState */

ParserState::ParserState() : elementHandler(nullptr), childHandler(nullptr) {}

ParserState::ParserState(ParserStateSet parents, Arguments arguments,
                         RttiSet supportedTypes,
                         HandlerConstructor elementHandler,
                         HandlerConstructor childHandler)
    : parents(parents),
      arguments(arguments),
      supportedTypes(supportedTypes),
      elementHandler(elementHandler),
      childHandler(childHandler)
{
}

ParserState::ParserState(const ParserStateBuilder &builder)
    : ParserState(builder.build())
{
}

/* Class ParserStateBuilder */

ParserStateBuilder &ParserStateBuilder::copy(const ParserState &state)
{
	this->state = state;
	return *this;
}

ParserStateBuilder &ParserStateBuilder::parent(const ParserState *parent)
{
	state.parents.insert(parent);
	return *this;
}

ParserStateBuilder &ParserStateBuilder::parents(const ParserStateSet &parents)
{
	state.parents.insert(parents.begin(), parents.end());
	return *this;
}

ParserStateBuilder &ParserStateBuilder::arguments(const Arguments &arguments)
{
	state.arguments = arguments;
	return *this;
}

ParserStateBuilder &ParserStateBuilder::supportedType(const Rtti *type)
{
	state.supportedTypes.insert(type);
	return *this;
}

ParserStateBuilder &ParserStateBuilder::supportedTypes(const RttiSet &types)
{
	state.supportedTypes.insert(types.begin(), types.end());
	return *this;
}

ParserStateBuilder &ParserStateBuilder::elementHandler(
    HandlerConstructor elementHandler)
{
	state.elementHandler = elementHandler;
	return *this;
}

ParserStateBuilder &ParserStateBuilder::childHandler(
    HandlerConstructor childHandler)
{
	state.childHandler = childHandler;
	return *this;
}

const ParserState &ParserStateBuilder::build() const { return state; }

/* Constant initializations */

namespace ParserStates {
const ParserState All;
const ParserState None;
}
}

