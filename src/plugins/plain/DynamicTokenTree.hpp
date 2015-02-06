/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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
 * @file DynamicTokenTree.hpp
 *
 * Class representing a token tree that can be updated dynamically.
 *
 * @author Benjamin Paaßen (astoecke@techfak.uni-bielefeld.de)
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_DYNAMIC_TOKEN_TREE_HPP_
#define _OUSIA_DYNAMIC_TOKEN_TREE_HPP_

#include <memory>
#include <unordered_map>

namespace ousia {

class TokenDescriptor;

/**
 * The Tokenizer internally uses a DynamicTokenTree to be efficiently able to
 * identify the longest consecutive token in the text. This is equivalent to a
 * prefix trie.
 *
 * A token tree is a construct that structures all special tokens a
 * Tokenizer recognizes. Consider the tokens "aab", "a" and "aac". Then
 * the token tree would look like this:
 *
 * \code{*.txt}
 * a
 * | \
 * a $
 * | \
 * b c
 * | |
 * $ $
 * \endcode
 *
 * Every node in the token tree is a valid end state that has a $ attached to
 * it. During the search algorithm the Tokenizer goes through the tree and
 * stores the last valid position. If a character follows that does not lead to
 * a new node in the TokenTree the search ends (and starts again at this
 * character). The token corresponding to the last valid position is returned.
 *
 * This allows us to uniquely identify the matching token given a certain
 * input text. Note that this is a greedy matching approach that does not
 * work if you're using truly ambiguous tokens (that have the same text).
 */
class DynamicTokenTree {
public:
	/**
	 * Structure used to build the node tree.
	 */
	struct Node {
		/**
		 * Type used for the child map.
		 */
		using ChildMap = std::unordered_map<char, std::unique_ptr<Node>>;

		/**
		 * Map from single characters at the corresponding child nodes.
		 */
		ChildMap children;

		/**
		 * Reference at the corresponding token descriptor. Set to nullptr if
		 * no token is attached to this node.
		 */
		TokenDescriptor const *descriptor;

		/**
		 * Default constructor, initializes the descriptor with nullptr.
		 */
		Node();
	};

private:
	/**
	 * Root node of the internal token tree.
	 */
	Node root;

public:
	/**
	 * Registers a token containing the given string. Returns false if the
	 * token already exists, true otherwise.
	 *
	 * @param token is the character sequence that should be registered as
	 * token.
	 * @param descriptor is the descriptor that should be set for this token.
	 * @return true if the operation is successful, false otherwise.
	 */
	bool registerToken(const std::string &token,
	                   const TokenDescriptor *descriptor) noexcept;

	/**
	 * Unregisters the token from the token tree. Returns true if the token was
	 * unregistered successfully, false otherwise.
	 *
	 * @param token is the character sequence that should be unregistered.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool unregisterToken(const std::string &token) noexcept;

	/**
	 * Returns true, if the given token exists within the TokenTree. This
	 * function is mostly thought for debugging and unit testing.
	 *
	 * @param token is the character sequence that should be searched.
	 * @return the attached token descriptor or nullptr if the given token is
	 * not found.
	 */
	const TokenDescriptor* hasToken(const std::string &token) const noexcept;
};
}

#endif /* _OUSIA_DYNAMIC_TOKEN_TREE_HPP_ */

