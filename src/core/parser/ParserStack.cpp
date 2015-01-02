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

#include <core/common/Utils.hpp>
#include <core/common/Exceptions.hpp>

namespace ousia {
namespace parser {

/* Class Handler */

void Handler::data(const std::string &data, int field)
{
	for (auto &c : data) {
		if (!Utils::isWhitespace(c)) {
			throw LoggableException{"No data allowed here."};
		}
	}
}

void Handler::child(std::shared_ptr<Handler> handler)
{
	// Do nothing here
}

/* Class HandlerDescriptor */

HandlerInstance HandlerDescriptor::create(const ParserContext &ctx,
                                          std::string name, State parentState,
                                          bool isChild,
                                          const Variant &args) const
{
	Handler *h = ctor(ctx, name, targetState, parentState, isChild);

	// Canonicalize the arguments
	Variant arguments = args;
	if (argsType != nullptr) {
		argsType->build(arguments, ctx.logger);
	}

	h->start(arguments);
	return HandlerInstance(h, this);
}

/* Class ParserStack */

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
		    Utils::join(expected, "\", \"") + std::string{"\", but got \""} +
		    name + std::string{"\""}};
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

void ParserStack::start(std::string name, const Variant &args)
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
	stack.emplace(descr->create(ctx, name, curState, isChild, args));
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

void ParserStack::data(const std::string &data, int field)
{
	// Check whether there is any command the data can be sent to
	if (stack.empty()) {
		throw LoggableException{"No command to receive data."};
	}

	// Pass the data to the current Handler instance
	stack.top().handler->data(data, field);
}
}
}

