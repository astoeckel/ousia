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

#ifndef _TILE_TREE_NODE_HPP_
#define _TILE_TREE_NODE_HPP_

#include <QSize>
#include <QRect>

#include <list>

namespace TiledUI {

class TileTreeNode {

private:
	TileTreeNode *parent;
	std::list<TileTreeNode*> children;

protected:

	void setParent(TileTreeNode *newParent);

	bool removeChild(TileTreeNode *child, bool recursive = false);

	void addChild(TileTreeNode *child, int idx = -1);

	int indexOf(TileTreeNode *child);

	virtual ();

public:

	TileTreeNode();

	TileTreeNode(TileTreeNode *parent);

	~TileTreeNode();

	virtual QSize minimumSize() const = 0;

	virtual QSize maximumSize() const = 0;

	virtual QRect geometry() const = 0;

	void resize(QSize size);

	void resize(int width, int height);

	virtual QWidget* parentWidget();

	virtual void 

};

}

#endif /* _TILE_TREE_NODE_HPP_ */

