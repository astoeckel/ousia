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
 * @file TokenTrie.hpp
 *
 * Class representing a token trie that can be updated dynamically.
 *
 * @author Benjamin Paaßen (astoecke@techfak.uni-bielefeld.de)
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_TOKEN_TRIE_HPP_
#define _OUSIA_TOKEN_TRIE_HPP_

#include <cstdint>
#include <memory>
#include <limits>
#include <unordered_map>

#include <core/common/Token.hpp>

namespace ousia {

/**
 * The Tokenizer internally uses a TokenTrie to be efficiently able to identify
 * the longest consecutive token in the text. This is equivalent to a prefix
 * trie.
 *
 * A token trie is a construct that structures all special tokens a Tokenizer
 * recognizes. Consider the tokens "aab", "a" and "bac" numbered as one, two and
 * three. Then the token tree would look like this:
 *
 * \code{*.txt}
 *        ~ (0)
 *       /     \
 *      a (2)  b (0)
 *      |      |
 *      a (0)  a (0)
 *      |      |
 *      b (1)  c (0)
 * \endcode
 *
 * Where the number indicates the corresponding token descriptor identifier.
 */
class TokenTrie {
public:
	/**
	 * Structure used to build the node tree.
	 */
	struct Node {
		/**
		 * Type used for the child map.
		 */
		using ChildMap = std::unordered_map<char, std::shared_ptr<Node>>;

		/**
		 * Map from single characters at the corresponding child nodes.
		 */
		ChildMap children;

		/**
		 * Id of the token represented by this node.
		 */
		TokenId id;

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
	 * @param id is the descriptor that should be set for this token.
	 * @return true if the operation is successful, false otherwise.
	 */
	bool registerToken(const std::string &token, TokenId id) noexcept;

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
	TokenId hasToken(const std::string &token) const noexcept;

	/**
	 * Returns a reference at the root node to be used for traversing the token
	 * tree.
	 *
	 * @return a reference at the root node.
	 */
	const Node *getRoot() const noexcept { return &root; }
};
}

#endif /* _OUSIA_TOKEN_TRIE_HPP_ */

