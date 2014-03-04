/*
    BasicWriter
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

#include "TileTreeNode.hpp"

namespace TiledUI {

TileTreeNode::TileTreeNode() :
	parent(nullptr);
{
	// Do nothing here
}

TileTreeNode::TileTreeNode(TileTreeNode *parent) :
	TileTreeNode()
{
	setParent(parent);
}

TileTreeNode::~TileTreeNode()
{
	// Free the memory for all children
	for (auto &c : children) {
		delete c;
	}
}

void TileTreeNode::setParent(TileTreeNode *newParent)
{
	// Automatically remove this child from the old parent
	if (parent) {
		parent->removeChild(this);
	}

	// Add this element to the new parent
	if (newParent) {
		newParent->addChild(this);
	}

	// The current parent is the new parent
	parent = newParent;
}

bool TileTreeNode::removeChild(GridTreeNode *node, bool recursive)
{
	// Iterate of the current container and remove the given node
	const int idx = indexOf(node);
	if (indexOf(node) >= 0) {
		children.erase(children.begin() + idx);
		return true;
	}

	// Descend into the tree if the recursive flag is set to true and the node
	// was not yet found
	if (recursive) {
		for (auto &c : children) {
			if (c.removeChild(node, true)) {
				return true;
			}
		}
	}

	return false;
}

void TileTreeNode::addChild(TileTreeNode *child, int idx)
{
	// Make sure the given child is only inserted once into the tree
	removeChild(child, true);

	// Insert the child at the given position, or at the end if idx < 0
	if (idx < 0) {
		children.push_back(child);
	} else {
		children.insert(children.begin() + idx, child);
	}
}

int TileTreeNode::indexOf(TileTreeNode *child)
{
	int i = 0;
	for (auto it = children.begin(); it != children.end(); it++, i++) {
		if (*it == node) {
			return i;
		}
	}
	return -1;
}

void TileTreeNode::parentWidget()
{
	if (parent)
	{
		return parent->parentWidget();
	}
	return nullptr;
}

}

