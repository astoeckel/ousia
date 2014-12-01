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

#include "XmlStates.hpp"

namespace ousia {
namespace parser {
namespace xml {

std::set<std::string> StateStack::expectedCommands(State state)
{
	std::set<std::string> res;
	for (const auto &v: handlers) {
		if (v.second.parentStates.count(state)) {
			res.insert(v.first);
		}
	}
	return res;
}

void StateStack::start(std::string tagName, char **attrs) {
	// Fetch the current handler and the current state
	const Handler *h = stack.empty() ? nullptr : stack.top();
	const State currentState = h ? State::NONE : h->state;

	// Fetch all handlers for the given tagName
	auto range = handlers.equal_range(tagName);
	if (range->first == handlers.end()) {
		// There are no handlers registered for this tag name -- check whether
		// the current handler supports arbitrary children
		if (h && h->arbitraryChildren)
	}
}

}
}
}

