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
#include <core/model/Syntax.hpp>

namespace ousia {
namespace parser_stack {

/**
 * Structure describing a token once it has been found in the text. Contains
 * lists pointing at the corresponding SyntaxDescriptors for the open, close and
 * shortForm token types.
 */
struct TokenDescriptor {
	/**
	 * Vector containing the SyntaxDescriptors in which the token occurs as
	 * "close" token. The Token is first tried to be processed as a "close"
	 * token.
	 */
	std::vector<SyntaxDescriptor> close;

	/**
	 * Vector containing the SyntaxDescriptors in which the token occurs as
	 * "shortForm" token. Creating a "shortForm" of the referenced ontology
	 * descriptor is tried next.
	 */
	std::vector<SyntaxDescriptor> shortForm;

	/**
	 * Vector containing the SyntaxDescriptors in which the token occurs as
	 * "open" token. The possibility of using the token in the "open" mode is
	 * tried in the last step.
	 */
	std::vector<SyntaxDescriptor> open;
};

/**
 * The TokenStack class is used by the Stack class to collect all currently
 * enabled user defined tokens. Additionally it allows the Stack class to
 * remember the end tokens for pending open tokens.
 */
class TokenStack {
private:
	/**
	 * Stack containing vectors of TokenSyntaxDescriptor instances as given by
	 * the user.
	 */
	std::vector<std::vector<SyntaxDescriptor>> stack;

public:
	/**
	 * Pushes a list of SyntaxDescriptor instances onto the internal stack.
	 *
	 * @param tokens is a list of SyntaxDescriptor instances that should be
	 * stored on the stack.
	 */
	void pushTokens(const std::vector<SyntaxDescriptor> &tokens);

	/**
	 * Removes the previously pushed list of tokens from the stack.
	 */
	void popTokens();

	/**
	 * Returns the TokenDescriptor for the given token.
	 *
	 * @param token is the token for which the different forms should be looked
	 * up.
	 */
	TokenDescriptor lookup(TokenId token) const;

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

