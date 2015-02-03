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
                         RttiSet createdNodeTypes,
                         HandlerConstructor elementHandler,
                         HandlerConstructor childHandler)
    : parents(parents),
      arguments(arguments),
      createdNodeTypes(createdNodeTypes),
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
	state.parents = ParserStateSet{parent};
	return *this;
}

ParserStateBuilder &ParserStateBuilder::parents(const ParserStateSet &parents)
{
	state.parents = parents;
	return *this;
}

ParserStateBuilder &ParserStateBuilder::arguments(const Arguments &arguments)
{
	state.arguments = arguments;
	return *this;
}

ParserStateBuilder &ParserStateBuilder::createdNodeType(const Rtti *type)
{
	state.createdNodeTypes = RttiSet{type};
	return *this;
}

ParserStateBuilder &ParserStateBuilder::createdNodeTypes(const RttiSet &types)
{
	state.createdNodeTypes = types;
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

/* Class ParserStateDeductor */

ParserStateDeductor::ParserStateDeductor(
    std::vector<const Rtti *> signature,
    std::vector<const ParserState *> states)
    : tbl(signature.size()),
      signature(std::move(signature)),
      states(std::move(states))
{
}

bool ParserStateDeductor::isActive(size_t d, const ParserState *s)
{
	// Lookup the "active" state of (d, s), if it was not already set
	// (e.second is true) we'll have to calculate it
	auto e = tbl[d].emplace(s, false);
	bool &res = e.first->second;
	if (!e.second) {
		return res;
	}

	// Check whether this node is generative (may have produced the Node
	// described by the current Signature element)
	bool isGenerative = signature[d]->isOneOf(s->createdNodeTypes);

	if (isGenerative && d == 0) {
		// End of recursion -- the last signature element is reached and the
		// node was generative
		res = true;
	} else {
		// Try repetition of this node
		if (isGenerative && isActive(d - 1, s)) {
			res = true;
		} else {
			// Check whether any of the parent nodes were active -- either for
			// the previous element (if this one is generative) or for the
			// current element (assuming this node was not generative)
			for (const ParserState *parent : s->parents) {
				if ((isGenerative && isActive(d - 1, parent)) ||
					isActive(d, parent)) {
					res = true;
					break;
				}
			}
		}
	}

	return res;
}

std::vector<const ParserState *> ParserStateDeductor::deduce()
{
	std::vector<const ParserState *> res;
	if (!signature.empty()) {
		const size_t D = signature.size();
		for (auto s : states) {
			if (signature[D - 1]->isOneOf(s->createdNodeTypes) &&
			    isActive(D - 1, s)) {
				res.push_back(s);
			}
		}
	}
	return res;
}

/* Constant initializations */

namespace ParserStates {
const ParserState All;
const ParserState None;
}
}

