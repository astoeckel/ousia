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

#include <QApplication>
#include <QColor>
#include <QPainter>
#include <QPalette>

#include "GridTree.hpp"

#include "UIGrid.hpp"

#include <iostream>

namespace uigrid {

UIGrid::UIGrid(QWidget *parent) :
	QWidget(parent)
{
	// Enable automatic ereasing of the background
	setAutoFillBackground(true);

	// Create the root grid node.
	rootGridNode = new GridTreeNode(Orientation::horz, 1.0);

	// Insert two new nodes into the tree
	GridTreeNode *nd1 = new GridTreeNode(Orientation::vert, 0.25, rootGridNode);
	GridTreeNode *nd2 = new GridTreeNode(Orientation::horz, 0.75, rootGridNode);

	// Add three nodes as children of the first node
	new GridTreeNode(Orientation::horz, 0.33, nd1);
	new GridTreeNode(Orientation::horz, 0.33, nd1);
	new GridTreeNode(Orientation::horz, 0.33, nd1);

	new GridTreeNode(Orientation::vert, 0.75, nd2);
	new GridTreeNode(Orientation::horz, 0.25, nd2);
}

UIGrid::~UIGrid()
{
	delete rootGridNode;
}

void UIGrid::paintEvent(QPaintEvent *event)
{
	const QPalette &palette = QApplication::palette();
	// Gather all splitter and frame area regions
	std::vector<Splitter> splitters;
	std::vector<FrameArea> areas;
	rootGridNode->gatherBoundingBoxes(&areas, &splitters,
			Rect(0, 0, width(), height()), 5);

	QPainter painter(this);

	// Draw the splitters (first the background, then the dividing line)
	painter.setPen(palette.mid().color());
	for (auto &c : splitters) {
		QRect r(c.r.x1, c.r.y1, c.r.w(), c.r.h());
		painter.fillRect(r, palette.light());
	}

}


}

