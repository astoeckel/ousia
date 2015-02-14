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

/**
 * @file ParserStateStack.hpp
 *
 * Helper classes for document or description parsers. Contains the
 * ParserStateStack class, which is an pushdown automaton responsible for
 * accepting commands in the correct order and calling specified handlers.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STATE_STACK_HPP_
#define _OUSIA_PARSER_STATE_STACK_HPP_

#include <cstdint>

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <vector>

#include <core/common/Variant.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Argument.hpp>

#include "Parser.hpp"
#include "ParserContext.hpp"
#include "ParserState.hpp"

namespace ousia {

/**
 * The ParserStateStack class is a pushdown automaton responsible for turning a
 * command stream into a tree of Node instances.
 */
class ParserStateStack {
private:
	/**
	 * Reference at the parser context.
	 */
	ParserContext &ctx;

	/**
	 * Map containing all registered command names and the corresponding
	 * state descriptors.
	 */
	const std::multimap<std::string, const ParserState *> &states;

	/**
	 * Internal stack used for managing the currently active Handler instances.
	 */
	std::stack<std::shared_ptr<Handler>> stack;

	/**
	 * Used internally to get all expected command names for the current state.
	 * This function is used to build error messages.
	 *
	 * @return a set of strings containing the names of the expected commands.
	 */
	std::set<std::string> expectedCommands();

	/**
	 * Returns the targetState for a command with the given name that can be
	 * reached from for the current state.
	 *
	 * @param name is the name of the requested command.
	 * @return nullptr if no target state was found, a pointer at the target
	 *state
	 * otherwise.
	 */
	const ParserState *findTargetState(const std::string &name);

public:
	/**
	 * Creates a new instance of the ParserStateStack class.
	 *
	 * @param ctx is the parser context the parser stack is working on.
	 * @param states is a map containing the command names and pointers at the
	 * corresponding ParserState instances.
	 */
	ParserStateStack(
	    ParserContext &ctx,
	    const std::multimap<std::string, const ParserState *> &states);

	/**
	 * Tries to reconstruct the parser state from the Scope instance of the
	 * ParserContext given in the constructor. This functionality is needed for
	 * including files,as the Parser of the included file needs to be brought to
	 + an equivalent state as the one in the including file.
	 *
	 * @param scope is the ParserScope instance from which the ParserState
	 * should be reconstructed.
	 * @param logger is the logger instance to which error messages should be
	 * written.
	 * @return true if the operation was sucessful, false otherwise.
	 */
	bool deduceState();

	/**
	 * Returns the state the ParserStateStack instance currently is in.
	 *
	 * @return the state of the currently active Handler instance or STATE_NONE
	 * if no handler is on the stack.
	 */
	const ParserState &currentState();

	/**
	 * Returns the command name that is currently being handled.
	 *
	 * @return the name of the command currently being handled by the active
	 * Handler instance or an empty string if no handler is currently active.
	 */
	std::string currentCommandName();

	/**
	 * Function that should be called whenever a new command is reached.
	 *
	 * @param name is the name of the command (including the namespace
	 * separator ':') and its corresponding location. Must be a string variant.
	 * @param args is a map variant containing the arguments that were passed to
	 * the command.
	 */
	void command(Variant name, Variant args);

	/**
	 * Function that should be called whenever a new field starts. Fields of the
	 * same command may not be separated by calls to 
	 */
	void fieldStart();

	/**
	 * Function that should be called whenever a field ends.
	 */
	void fieldEnd();

	/**
	 * Function that shuold be called whenever character data is found in the
	 * input stream.
	 *
	 * @param data is a variant of any type containing the data that was parsed
	 * as data.
	 */
	void data(Variant data);

	/**
	 * Function that should be called whenever an annotation starts.
	 *
	 * @param name is the name of the annotation class.
	 * @param args is a map variant containing the arguments that were passed
	 * to the annotation.
	 */
	void annotationStart(Variant name, Variant args);

	/**
	 * Function that should be called whenever an annotation ends.
	 *
	 * @param name is the name of the annotation class that was ended.
	 * @param annotationName is the name of the annotation that was ended.
	 */
	void annotationEnd(Variant name, Variant annotationName);

	/**
	 * Function that should be called whenever a previously registered token
	 * is found in the input stream.
	 *
	 * @param token is string variant containing the token that was encountered.
	 */
	void token(Variant token);
};
}

#endif /* _OUSIA_PARSER_STATE_STACK_HPP_ */

