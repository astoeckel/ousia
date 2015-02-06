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

#include "DynamicTokenTree.hpp"

namespace ousia {

/* Class DynamicTokenTree::Node */

DynamicTokenTree::Node::Node() : descriptor(nullptr) {}

/* Class DynamicTokenTree */

bool DynamicTokenTree::registerToken(const std::string &token,
                                     const TokenDescriptor *descriptor) noexcept
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
			it = node->children.emplace(c, std::unique_ptr<Node>(new Node{})).first;
		}
		node = it->second.get();
	}

	// If the resulting node already has a descriptor set, we're screwed.
	if (node->descriptor != nullptr) {
		return false;
	}

	// Otherwise just set the descriptor to the given descriptor.
	node->descriptor = descriptor;
	return true;
}

bool DynamicTokenTree::unregisterToken(const std::string &token) noexcept
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
	for (size_t i = 0; i < token.size() - 1; i++) {
		// Go to the next node, abort if the tree ends unexpectedly
		auto it = node->children.find(token[i]);
		if (it == node->children.end()) {
			return false;
		}

		// Reset the subtree handler if this node has another descriptor
		Node *child = it->second.get();
		if ((child->descriptor != nullptr || child->children.size() > 1)) {
			subtreeRoot = child;
			subtreeKey = token[i + 1];
		}
		node = child;
	}

	// If the leaf node, we cannot delete the subtree. Set the
	// descriptor to nullptr instead
	if (!node->children.empty()) {
		node->descriptor = nullptr;
		return true;
	}

	// If we end up here, we can safely delete the complete subtree
	subtreeRoot->children.erase(subtreeKey);
	return true;
}

const TokenDescriptor *DynamicTokenTree::hasToken(
    const std::string &token) const noexcept
{
	Node const *node = &root;
	for (size_t i = 0; i < token.size(); i++) {
		const char c = token[i];
		auto it = node->children.find(c);
		if (it == node->children.end()) {
			return nullptr;
		}
		node = it->second.get();
	}
	return node->descriptor;
}
}

