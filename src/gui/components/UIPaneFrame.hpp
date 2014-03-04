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

#ifndef _UI_PANE_FRAME_HPP_
#define _UI_PANE_FRAME_HPP_

#include <QWidget>

class QBoxLayout;
class QVBoxLayout;
class QHBoxLayout;

class UIPaneFrame : public QWidget
{
	Q_OBJECT

private:
	QVBoxLayout *rootLayout;
	QHBoxLayout *topLayout;
	QVBoxLayout *centerLayout;
	QHBoxLayout *bottomLayout;
	QWidget *topContainer;
	QWidget *centerContainer;
	QWidget *bottomContainer;

	void removeLayoutSpacing(QBoxLayout *layout);

public:
	explicit UIPaneFrame(QWidget *parent = 0);
	 ~UIPaneFrame();

};

#endif /* _UI_PANE_FRAME_HPP_ */

