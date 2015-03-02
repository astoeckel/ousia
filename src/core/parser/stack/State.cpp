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

#include "State.hpp"

namespace ousia {
namespace parser_stack {

/* Class State */

State::State() : elementHandler(nullptr), supportsAnnotations(false), supportsTokens(false) {}

State::State(StateSet parents, Arguments arguments,
                         RttiSet createdNodeTypes,
                         HandlerConstructor elementHandler,
                         bool supportsAnnotations,
                         bool supportsTokens)
    : parents(parents),
      arguments(arguments),
      createdNodeTypes(createdNodeTypes),
      elementHandler(elementHandler),
      supportsAnnotations(supportsAnnotations),
      supportsTokens(supportsTokens)
{
}

State::State(const StateBuilder &builder)
    : State(builder.build())
{
}

/* Class StateBuilder */

StateBuilder &StateBuilder::copy(const State &state)
{
	this->state = state;
	return *this;
}

StateBuilder &StateBuilder::parent(const State *parent)
{
	state.parents = StateSet{parent};
	return *this;
}

StateBuilder &StateBuilder::parents(const StateSet &parents)
{
	state.parents = parents;
	return *this;
}

StateBuilder &StateBuilder::arguments(const Arguments &arguments)
{
	state.arguments = arguments;
	return *this;
}

StateBuilder &StateBuilder::createdNodeType(const Rtti *type)
{
	state.createdNodeTypes = RttiSet{type};
	return *this;
}

StateBuilder &StateBuilder::createdNodeTypes(const RttiSet &types)
{
	state.createdNodeTypes = types;
	return *this;
}

StateBuilder &StateBuilder::elementHandler(
    HandlerConstructor elementHandler)
{
	state.elementHandler = elementHandler;
	return *this;
}

StateBuilder &StateBuilder::supportsAnnotations(bool supportsAnnotations)
{
	state.supportsAnnotations = supportsAnnotations;
	return *this;
}

StateBuilder &StateBuilder::supportsTokens(bool supportsTokens)
{
	state.supportsTokens = supportsTokens;
	return *this;
}


const State &StateBuilder::build() const { return state; }

/* Class StateDeductor */

StateDeductor::StateDeductor(
    std::vector<const Rtti *> signature,
    std::vector<const State *> states)
    : tbl(signature.size()),
      signature(std::move(signature)),
      states(std::move(states))
{
}

bool StateDeductor::isActive(size_t d, const State *s)
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
			for (const State *parent : s->parents) {
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

std::vector<const State *> StateDeductor::deduce()
{
	std::vector<const State *> res;
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

namespace States {
const State All;
const State None;
}
}
}

