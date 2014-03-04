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

#include "GridTree.hpp"

#include <iostream>

namespace uigrid {

Splitter getSplitter(Orientation orientation, const Rect &r)
{
	switch (orientation) {
		case Orientation::vert:
			return Splitter(this, Rect(r.x1, r.y2 - ss, r.x2, r.y2),
					orientation);
		case Orientation::horz:
			return Splitter(this, Rect(r.x1, r.y1, r.x2 - ss, r.y2 - ss),
					orientation);
		default:
			return Rect(0, 0, 0, 0);
	}
}

void GridTreeNode::gatherBoundingBoxes(std::vector<FrameArea> *areas,
			std::vector<Splitter> *splitters, const Rect &r,
			int splitterSize)
{
	const int ss = splitterSize;
	const int h = r.h();
	const int w = r.w();

	// If this node is a leaf, store the area of the frame and the splitter
	// positions in the given lists and abort.
	if (isLeaf()) {
		if (areas) {
			areas->push_back(FrameArea(this, r));
		}
		if (splitters) {
			splitters->push_back(getSplitter(Orientation::vert, r));
			splitters->push_back(getSplitter(Orientation::horz, r));
		}
		return;
	}

	// Recursively descend into the child nodes. Calculate the area the
	// child nodes occupy. The last child should always occupy all remaining
	// space in order to avoid gaps caused by rounding errors.
	unsigned int i = 0;
	switch (orientation) {
		case Orientation::vert: {
				int offsY = r.y1;
				for (auto it = children.begin(); it != children.end();
						it++, i++) {
					const int ch = (i == children.size() - 1)
						? r.y2 - offsY : h * (*it)->relativeSize;
					(*it)->gatherBoundingBoxes(areas, splitters,
						Rect::bounds(r.x1, offsY, w, ch), splitterSize);
					offsY += ch;
				}
			}
			break;
		case Orientation::horz: {
				int offsX = r.x1;
				for (auto it = children.begin(); it != children.end();
						it++, i++) {
					const int cw = (i == children.size() - 1)
						? r.x2 - offsX : w * (*it)->relativeSize;
					(*it)->gatherBoundingBoxes(areas, splitters,
						Rect::bounds(offsX, r.y1, cw, h), splitterSize);
					offsX += cw;
				}
			}
			break;
		case Orientation::none:
			break;
	}
}

}

