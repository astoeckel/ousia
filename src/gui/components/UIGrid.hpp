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

#ifndef _UI_GRID_H_
#define _UI_GRID_H_

#include <QWidget>

namespace uigrid {

class GridTreeNode;

class UIGrid : public QWidget
{
	Q_OBJECT

private:
	GridTreeNode *rootGridNode;

protected:
	virtual void paintEvent(QPaintEvent *event);

public:

	UIGrid(QWidget *parent);

	~UIGrid();

};

}

#endif /* _UI_GRID_H_ */

