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

#ifndef _GRID_TREE_H_
#define _GRID_TREE_H_

#include <vector>
#include <list>

namespace uigrid {

class GridTreeNode;

enum class Orientation
{
	none, horz, vert
};

struct Rect
{
	int x1, y1, x2, y2;

	Rect(int x1, int y1, int x2, int y2) :
		x1(x1), y1(y1), x2(x2), y2(y2)
	{
		// Do nothing here
	}

	static Rect bounds(int x, int y, int w, int h)
	{
		return Rect(x, y, x + w, y + h);
	}

	int w() const
	{
		return x2 - x1;
	}

	int h() const
	{
		return y2 - y1;
	}

};

struct FrameArea
{
	GridTreeNode *node;
	Rect r;

	FrameArea(GridTreeNode *node, const Rect &r) :
		node(node), r(r)
	{
		// Do nothing here
	}
};

struct Splitter
{
	GridTreeNode *node;
	Rect r;
	Orientation orientation;

	Splitter(GridTreeNode *node, const Rect &r, Orientation orientation) :
		node(node), r(r), orientation(orientation)
	{
		// Do nothing here
	}
};

class GridTreeNode
{

private:
	Orientation orientation;
	float relativeSize;
	GridTreeNode *parent;
	void *data;

	std::list<GridTreeNode*> children;

	void setParent(GridTreeNode *parent)
	{
		if (this->parent) {
			this->parent->removeChild(this);
		}
		if (parent) {
			parent->addChild(this);
		}
		this->parent = parent;
	}

	void removeChild(GridTreeNode *node)
	{
		for (auto it = children.begin(); it != children.end();) {
			if (*it == node) {
				it = children.erase(it);
			} else {
				it++;
			}
		}
	}

	void addChild(GridTreeNode *node)
	{
		children.push_back(node);
	}

public:

	/**
	 * Constructor of the GridTreeNode class.
	 *
	 * @param orientation describes the orientation of the children of
	 * this grid tree node.
	 * @param relativeSize contains the size of this node relative to the size
	 * of its parent node. The sum of the relativeSizes of all siblings has to
	 * be one.
	 * @param data is the data that should be attached to the node.
	 */
	GridTreeNode(Orientation orientation, float relativeSize = 1.0f,
			GridTreeNode *parent = nullptr, void *data = nullptr) :
		orientation(orientation), relativeSize(relativeSize), parent(nullptr),
		data(data)
	{
		setParent(parent);
	}

	~GridTreeNode()
	{
		// Delete all children
		for (auto c : children) {
			delete c;
		}
	}

	/**
	 * Returns true if this element of the grid tree is a "leaf" (e.g. this
	 * element has no children).
	 *
	 * @return true if the grid element is a leaf node, false otherwise.
	 */
	bool isLeaf() const
	{
		return children.empty();
	}

	/**
	 * Returns true if this element is the root node (has no parent).
	 *
	 * @return true if this element is the root node, false otherwise.
	 */
	bool isRoot() const
	{
		return parent == nullptr;
	}

	/**
	 * Sets the relative size of the node.
	 *
	 * @param relativeSize is the new relative size of the frame. Should be in
	 * an interval of [0, 1].
	 */
	void setRelativeSize(float relativeSize)
	{
		this->relativeSize = relativeSize;
	}

	/**
	 * Returns the current relative size of the node.
	 *
	 * @return the current relative size of the node.
	 */
	float getRelativeSize()
	{
		return relativeSize;
	}

	/**
	 * Returns the data that was attached to this grid tree node.
	 *
	 * @return the data that was attached to this grid tree node.
	 */
	void* getData()
	{
		return data;
	}

	/**
	 * Gathers the frame areas and the areas for which splitters should be
	 * drawn.
	 *
	 * @param areas is the list into which the frame area descriptors should be
	 * inserted.
	 * @param splitters is the list into which the splitter descriptors should
	 * be inserted. If nullptr is given, the list is not filled
	 * @param w is the width of the region for which the splitters should be
	 * gathered.
	 * @param h is the height of the region for which the splitters should be
	 * gathered.
	 */
	void gatherBoundingBoxes(std::vector<FrameArea> *areas,
			std::vector<Splitter> *splitters, const Rect &r,
			int splitterSize);

	/**
	 * Returns the position of the splitter with the given orientation for this
	 * element.
	 */
	Rect getSplitterPosition(Orientation orientation, const Rect &r);

};

}

#endif /* _GRID_TREE_H_ */

