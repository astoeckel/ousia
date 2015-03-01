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
 * @file Stack.hpp
 *
 * Helper classes for document or description parsers. Contains the
 * Stack class, which is an pushdown automaton responsible for
 * accepting commands in the correct order and calling specified handlers.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_STACK_HPP_
#define _OUSIA_PARSER_STACK_STACK_HPP_

#include <map>
#include <memory>

namespace ousia {

// Forward declarations
class ParserContext;
class TokenizedData;
class Variant;

namespace parser_stack {

// Forward declarations
class StackImpl;
class State;

/**
 * The Stack class is a pushdown automaton responsible for turning a command
 * stream into a tree of Node instances. It does so by following a state
 * transition graph and creating a set of Handler instances, which are placed
 * on the stack. Additionally it is responsible for the normalization of
 * Annotations and for handling tokens.
 */
class Stack {
private:
	/**
	 * Pointer at the internal implementation
	 */
	std::unique_ptr<StackImpl> impl;

public:
	/**
	 * Creates a new instance of the Stack class.
	 *
	 * @param ctx is the parser context the parser stack is working on.
	 * @param states is a map containing the command names and pointers at the
	 * corresponding State instances.
	 */
	Stack(ParserContext &ctx,
	      const std::multimap<std::string, const State *> &states);

	/**
	 * Destructor of the Stack class.
	 */
	~Stack();

	/**
	 * Returns the state the Stack instance currently is in.
	 *
	 * @return the state of the currently active Handler instance or
	 * States::None if no handler is on the stack.
	 */
	const State &currentState();

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
	 * @param args is a map containing the arguments that were passed to the
	 * command.
	 * @param range if true, the started command has an explicit range.
	 */
	void commandStart(const Variant &name, const Variant::mapType &args,
	                  bool range);

	/**
	 * Function that should be called whenever an annotation starts.
	 *
	 * @param name is the name of the annotation class.
	 * @param args is a map variant containing the arguments that were passed
	 * to the annotation.
	 * @param range if true, the annotation fields have an explicit range.
	 */
	void annotationStart(const Variant &className, const Variant &args,
	                     bool range);

	/**
	 * Function that should be called whenever an annotation ends.
	 *
	 * @param name is the name of the annotation class that was ended.
	 * @param annotationName is the name of the annotation that was ended.
	 */
	void annotationEnd(const Variant &className, const Variant &elementName);

	/**
	 * Function the should be called whenever a ranged command or annotation
	 * ends. Must be called if the range parameter range was set to true when
	 * annotationStart() or commandStart() were called.
	 */
	void rangeEnd();

	/**
	 * Function that should be called whenever a new field starts. Fields of the
	 * same command may not be separated by calls to data or annotations. Doing
	 * so will result in a LoggableException.
	 *
	 * @param isDefault should be set to true if the started field explicitly
	 * is the default field.
	 */
	void fieldStart(bool isDefault);

	/**
	 * Function that should be called whenever a field ends. Calling this
	 * function if there is no field to end will result in a LoggableException.
	 */
	void fieldEnd();

	/**
	 * Function that should be called whenever character data is found in the
	 * input stream. May only be called if the currently is a command on the
	 * stack.
	 *
	 * @param data is a TokenizedData instance containing the pre-segmented data
	 * that should be read.
	 */
	void data(const TokenizedData &data);

	/**
	 * Function that shuold be called whenever character data is found in the
	 * input stream. The given string variant is converted into a TokenizedData
	 * instance internally.
	 *
	 * @param stringData is a string variant containing the data that has been
	 * found.
	 */
	void data(const Variant &stringData);
};
}
}

#endif /* _OUSIA_STACK_HPP_ */

