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

#include "TokenTrie.hpp"

namespace ousia {

/* Class DynamicTokenTree::Node */

TokenTrie::Node::Node() : type(EmptyToken) {}

/* Class DynamicTokenTree */

bool TokenTrie::registerToken(const std::string &token,
                              TokenTypeId type) noexcept
{
	// Abort if the token is empty -- this would taint the root node
	if (token.empty()) {
		return false;
	}

	// Iterate over each character in the given string and insert them as
	// (new) nodes
	Node *node = &root;
	for (size_t i = 0; i < token.size(); i++) {
		// Insert a new node if this one does not exist
		const char c = token[i];
		auto it = node->children.find(c);
		if (it == node->children.end()) {
			it = node->children.emplace(c, std::make_shared<Node>()).first;
		}
		node = it->second.get();
	}

	// If the resulting node already has a type set, we're screwed.
	if (node->type != EmptyToken) {
		return false;
	}

	// Otherwise just set the type to the given type.
	node->type = type;
	return true;
}

bool TokenTrie::unregisterToken(const std::string &token) noexcept
{
	// We cannot remove empty tokens as we need to access the fist character
	// upfront
	if (token.empty()) {
		return false;
	}

	// First pass -- search the node in the path that can be deleted
	Node *subtreeRoot = &root;
	char subtreeKey = token[0];
	Node *node = &root;
	for (size_t i = 0; i < token.size(); i++) {
		// Go to the next node, abort if the tree ends unexpectedly
		auto it = node->children.find(token[i]);
		if (it == node->children.end()) {
			return false;
		}

		// Reset the subtree handler if this node has another type
		node = it->second.get();
		if ((node->type != EmptyToken || node->children.size() > 1) &&
		    (i + 1 != token.size())) {
			subtreeRoot = node;
			subtreeKey = token[i + 1];
		}
	}

	// If the node type is already EmptyToken, we cannot do anything here
	if (node->type == EmptyToken) {
		return false;
	}

	// If the target node has children, we cannot delete the subtree. Set the
	// type to EmptyToken instead
	if (!node->children.empty()) {
		node->type = EmptyToken;
		return true;
	}

	// If we end up here, we can safely delete the complete subtree
	subtreeRoot->children.erase(subtreeKey);
	return true;
}

TokenTypeId TokenTrie::hasToken(const std::string &token) const noexcept
{
	Node const *node = &root;
	for (size_t i = 0; i < token.size(); i++) {
		const char c = token[i];
		auto it = node->children.find(c);
		if (it == node->children.end()) {
			return EmptyToken;
		}
		node = it->second.get();
	}
	return node->type;
}
}

