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
 * @file Callbacks.hpp
 *
 * Contains an interface defining the callbacks that can be directed from a
 * StateHandler to the StateStack, and from the StateStack to
 * the actual parser.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_CALLBACKS_HPP_
#define _OUSIA_PARSER_STACK_CALLBACKS_HPP_

#include <string>
#include <vector>

#include <core/common/Whitespace.hpp>
#include <core/common/Token.hpp>

namespace ousia {

// Forward declarations
class Variant;

namespace parser_stack {

/**
 * Interface between the Stack class and the underlying parser used for
 * registering and unregistering tokens.
 */
class ParserCallbacks {
public:
	/**
	 * Virtual descructor.
	 */
	virtual ~ParserCallbacks();

	/**
	 * Registers the given token as token that should be reported to the handler
	 * using the "token" function.
	 *
	 * @param token is the token string that should be reported.
	 * @return the token id with which the token will be reported. Should return
	 * Tokens::Empty if the given token could not be registered.
	 */
	virtual TokenId registerToken(const std::string &token) = 0;

	/**
	 * Unregisters the given token, it will no longer be reported to the handler
	 * using the "token" function.
	 *
	 * @param id is the token id of the token that should be unregistered.
	 */
	virtual void unregisterToken(TokenId id) = 0;
};

/**
 * Interface defining a set of callback functions that act as a basis for the
 * StateStackCallbacks and the ParserCallbacks.
 */
class HandlerCallbacks: public ParserCallbacks {
public:
	/**
	 * Reads a string variant form the current input stream. This function must
	 * be called from the data() method.
	 *
	 * @return a string variant containing the current text data. The return
	 * value depends on the currently set whitespace mode and the tokens that
	 * were enabled using the enableTokens callback method.
	 */
	Variant readData();

	/**
	 * Pushes a list of TokenSyntaxDescriptor instances onto the internal stack.
	 * The tokens described in the token list are the tokens that are currently
	 * enabled.
	 *
	 * @param tokens is a list of TokenSyntaxDescriptor instances that should be
	 * stored on the stack.
	 */
	void pushTokens(const std::vector<TokenSyntaxDescriptor> &tokens);

	/**
	 * Removes the previously pushed list of tokens from the stack.
	 */
	void popTokens();
};

}
}

#endif /* _OUSIA_PARSER_STACK_CALLBACKS_HPP_ */

