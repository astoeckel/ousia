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

#include <sstream>

#include "ParserStack.hpp"

#include <core/Exceptions.hpp>

namespace ousia {
namespace parser {

/* Class HandlerDescriptor */

HandlerInstance HandlerDescriptor::create(const ParserContext &ctx,
                                          std::string name, State parentState,
                                          bool isChild, char **attrs) const
{
	Handler *h = ctor(ctx, name, targetState, parentState, isChild);
	h->start(attrs);
	return HandlerInstance(h, this);
}

/* Class ParserStack */

/**
 * Function used internally to turn the elements of a collection into a string
 * separated by the given delimiter.
 */
template <class T>
static std::string join(T es, const std::string &delim)
{
	std::stringstream res;
	bool first = true;
	for (auto &e : es) {
		if (!first) {
			res << delim;
		}
		res << e;
		first = false;
	}
	return res.str();
}

/**
 * Returns an Exception that should be thrown when a currently invalid command
 * is thrown.
 */
static LoggableException invalidCommand(const std::string &name,
                                        const std::set<std::string> &expected)
{
	if (expected.empty()) {
		return LoggableException{
		    std::string{"No nested elements allowed, but got \""} + name +
		    std::string{"\""}};
	} else {
		return LoggableException{
		    std::string{"Expected "} +
		    (expected.size() == 1 ? std::string{"\""}
		                          : std::string{"one of \""}) +
		    join(expected, "\", \"") + std::string{"\", but got \""} + name +
		    std::string{"\""}};
	}
}

std::set<std::string> ParserStack::expectedCommands(State state)
{
	std::set<std::string> res;
	for (const auto &v : handlers) {
		if (v.second.parentStates.count(state)) {
			res.insert(v.first);
		}
	}
	return res;
}

void ParserStack::start(std::string name, char **attrs)
{
	// Fetch the current handler and the current state
	const HandlerInstance *h = stack.empty() ? nullptr : &stack.top();
	const State curState = currentState();
	bool isChild = false;

	// Fetch the correct Handler descriptor for this
	const HandlerDescriptor *descr = nullptr;
	auto range = handlers.equal_range(name);
	for (auto it = range.first; it != range.second; it++) {
		const std::set<State> &parentStates = it->second.parentStates;
		if (parentStates.count(curState) || parentStates.count(STATE_ALL)) {
			descr = &(it->second);
			break;
		}
	}
	if (!descr && currentArbitraryChildren()) {
		isChild = true;
		descr = h->descr;
	}

	// No descriptor found, throw an exception.
	if (!descr) {
		throw invalidCommand(name, expectedCommands(curState));
	}

	// Instantiate the handler and call its start function
	stack.emplace(descr->create(ctx, name, curState, isChild, attrs));
}

void ParserStack::end()
{
	// Check whether the current command could be ended
	if (stack.empty()) {
		throw LoggableException{"No command to end."};
	}

	// Remove the current HandlerInstance from the stack
	HandlerInstance inst{stack.top()};
	stack.pop();

	// Call the end function of the last Handler
	inst.handler->end();

	// Call the "child" function of the parent Handler in the stack
	// (if one exists).
	if (!stack.empty()) {
		stack.top().handler->child(inst.handler);
	}
}

void ParserStack::data(const char *data, int len)
{
	// Check whether there is any command the data can be sent to
	if (stack.empty()) {
		throw LoggableException{"No command to receive data."};
	}

	// Pass the data to the current Handler instance
	stack.top().handler->data(data, len);
}
}
}

