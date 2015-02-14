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

#include <core/common/Utils.hpp>
#include <core/common/Exceptions.hpp>
#include <core/model/Project.hpp>

#include "ParserScope.hpp"
#include "ParserStateStack.hpp"

namespace ousia {

/* Class ParserStateStack */

/**
 * Returns an Exception that should be thrown when a currently invalid command
 * is thrown.
 */
static LoggableException InvalidCommand(const std::string &name,
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

ParserStateStack::ParserStateStack(
    ParserContext &ctx,
    const std::multimap<std::string, const ParserState *> &states)
    : ctx(ctx), states(states)
{
}

bool ParserStateStack::deduceState()
{
	// Assemble all states
	std::vector<const ParserState *> states;
	for (const auto &e : this->states) {
		states.push_back(e.second);
	}

	// Fetch the type signature of the scope and derive all possible states,
	// abort if no unique parser state was found
	std::vector<const ParserState *> possibleStates =
	    ParserStateDeductor(ctx.getScope().getStackTypeSignature(), states)
	        .deduce();
	if (possibleStates.size() != 1) {
		ctx.getLogger().error(
		    "Error while including file: Cannot deduce parser state.");
		return false;
	}

	// Switch to this state by creating a dummy handler
	const ParserState *state = possibleStates[0];
	Handler *handler =
	    DefaultHandler::create({ctx, "", *state, *state, SourceLocation{}});
	stack.emplace(handler);
	return true;
}

std::set<std::string> ParserStateStack::expectedCommands()
{
	const ParserState *currentState = &(this->currentState());
	std::set<std::string> res;
	for (const auto &v : states) {
		if (v.second->parents.count(currentState)) {
			res.insert(v.first);
		}
	}
	return res;
}

const ParserState &ParserStateStack::currentState()
{
	return stack.empty() ? ParserStates::None : stack.top()->state();
}

std::string ParserStateStack::currentCommandName()
{
	return stack.empty() ? std::string{} : stack.top()->name();
}

const ParserState *ParserStateStack::findTargetState(const std::string &name)
{
	const ParserState *currentState = &(this->currentState());
	auto range = states.equal_range(name);
	for (auto it = range.first; it != range.second; it++) {
		const ParserStateSet &parents = it->second->parents;
		if (parents.count(currentState) || parents.count(&ParserStates::All)) {
			return it->second;
		}
	}

	return nullptr;
}

void ParserStateStack::start(const std::string &name, Variant::mapType &args,
                        const SourceLocation &location)
{
	ParserState const *targetState = findTargetState(name);
// TODO: Andreas, please improve this.
//	if (!Utils::isIdentifier(name)) {
//		throw LoggableException(std::string("Invalid identifier \"") + name +
//		                        std::string("\""));
//	}

	if (targetState == nullptr) {
		targetState = findTargetState("*");
	}
	if (targetState == nullptr) {
		throw InvalidCommand(name, expectedCommands());
	}

	// Fetch the associated constructor
	HandlerConstructor ctor = targetState->elementHandler
	                              ? targetState->elementHandler
	                              : DefaultHandler::create;

	// Canonicalize the arguments, allow additional arguments
	targetState->arguments.validateMap(args, ctx.getLogger(), true);

	// Instantiate the handler and call its start function
	Handler *handler = ctor({ctx, name, *targetState, currentState(), location});
	handler->start(args);
	stack.emplace(handler);
}

void ParserStateStack::start(std::string name, const Variant::mapType &args,
                        const SourceLocation &location)
{
	Variant::mapType argsCopy(args);
	start(name, argsCopy);
}

void ParserStateStack::end()
{
	// Check whether the current command could be ended
	if (stack.empty()) {
		throw LoggableException{"No command to end."};
	}

	// Remove the current HandlerInstance from the stack
	std::shared_ptr<Handler> inst{stack.top()};
	stack.pop();

	// Call the end function of the last Handler
	inst->end();
}

void ParserStateStack::data(const std::string &data, int field)
{
	// Check whether there is any command the data can be sent to
	if (stack.empty()) {
		throw LoggableException{"No command to receive data."};
	}

	// Pass the data to the current Handler instance
	stack.top()->data(data, field);
}
}

