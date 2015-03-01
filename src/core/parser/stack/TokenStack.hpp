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
 * @file TokenStack.hpp
 *
 * Contains the TokenStack class used for collecting the currently enabled user
 * defined tokens on a per-field basis.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_TOKEN_STACK_HPP_
#define _OUSIA_PARSER_STACK_TOKEN_STACK_HPP_

#include <memory>
#include <vector>

#include <core/common/Token.hpp>

namespace ousia {
namespace parser_stack {

/**
 * The TokenStack class is used by the Stack class to collect all currently
 * enabled user defined tokens.
 */
class TokenStack {
private:
	/**
	 * Shared pointer at the parent TokenStack instance. May be nullptr, in
	 * which case no parent TokenStack instance exists.
	 */
	const TokenStack *parentStack;

	/**
	 * Stack containing vectors of TokenSyntaxDescriptor instances as given by
	 * the user.
	 */
	std::vector<std::vector<TokenSyntaxDescriptor>> stack;

	/**
	 * Constructor of the TokenStack class.
	 *
	 * @param parentStack is a pointer at the underlying parentStack instance
	 * to which calls should be forwarded if no data has been pushed onto this
	 * stack instance.
	 */
	TokenStack(const TokenStack *parentStack) : parentStack(parentStack) {}

public:
	/**
	 * Default constructor of the TokenStack class with no reference at a parent
	 * stack.
	 */
	TokenStack() : TokenStack(nullptr) {}

	/**
	 * Constructor of the TokenStack class with a reference at a parent
	 * TokenStack instance.
	 *
	 * @param parentStack is a reference at a parent TokenStack instance. If no
	 * data has yet been pushed onto this instance, calls will be forwarded to
	 * the parent stack.
	 */
	TokenStack(const TokenStack &parentStack) : TokenStack(&parentStack) {}

	/**
	 * Pushes a list of TokenSyntaxDescriptor instances onto the internal stack.
	 *
	 * @param tokens is a list of TokenSyntaxDescriptor instances that should be
	 * stored on the stack.
	 */
	void pushTokens(const std::vector<TokenSyntaxDescriptor> &tokens);

	/**
	 * Removes the previously pushed list of tokens from the stack.
	 */
	void popTokens();

	/**
	 * Returns a set containing all currently enabled tokens. The set of enabled
	 * tokens are those tokens that were pushed last onto the stack. This set
	 * has to be passed to the TokenizedData instance in order to gather all
	 * tokens that are currently possible.
	 *
	 * @return a set of tokens containing all the Tokens that are currently
	 * possible.
	 */
	TokenSet tokens() const;
};
}
}

#endif /* _OUSIA_PARSER_STACK_TOKEN_STACK_HPP_ */

