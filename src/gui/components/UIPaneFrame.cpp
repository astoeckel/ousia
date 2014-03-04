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

#include <QBoxLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QColor>
#include <QToolButton>
#include <QSizePolicy>
#include <QLabel>
#include <QTreeView>
#include <QDir>
#include <QFileSystemModel>

#include <gui/components/UIPaneFrame.hpp>

static void setWidgetColor(QWidget *w, const QColor c)
{
	QPalette p(w->palette());
	p.setColor(QPalette::Background, c);
	w->setAutoFillBackground(true);
	w->setPalette(p);
}

UIPaneFrame::UIPaneFrame(QWidget *parent) :
	QWidget(parent)
{
	// Create the layout components
	rootLayout = new QVBoxLayout;
	topLayout = new QHBoxLayout;
	centerLayout = new QVBoxLayout;
	bottomLayout = new QHBoxLayout;

	// Assemble the top bar
//	QToolButton *btn = new QToolButton;
//	btn->setIcon(QIcon::fromTheme("user-home"));

	QLabel *lbl = new QLabel("Dies ist nur ein Test");
	lbl->setContentsMargins(10, 0, 10, 0);

	QToolButton *btn2 = new QToolButton;
	btn2->setIcon(QIcon::fromTheme("edit-find"));

	QToolButton *btn3 = new QToolButton;
	btn3->setIcon(QIcon::fromTheme("window-new"));

//	topLayout->addWidget(btn);
	topLayout->addWidget(lbl);
	topLayout->addWidget(btn2);
	topLayout->addWidget(btn3);

	QTreeView *tree = new QTreeView;
	QFileSystemModel *model = new QFileSystemModel;
	model->setRootPath(QDir::currentPath());
	tree->setModel(model);
	centerLayout->addWidget(tree);

	// Remove the spacing of the layout components
	removeLayoutSpacing(rootLayout);
	removeLayoutSpacing(topLayout);
	removeLayoutSpacing(centerLayout);
	removeLayoutSpacing(bottomLayout);

	// Create the widget containers
	topContainer = new QWidget;
	centerContainer = new QWidget;
	bottomContainer = new QWidget;

	// Assign the layouts to the top and bottom part, add the components to the
	// root layout
	topContainer->setLayout(topLayout);
	topContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

	centerContainer->setLayout(centerLayout);

	bottomContainer->setLayout(bottomLayout);
	topContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

	rootLayout->addWidget(topContainer, 0);
	rootLayout->addWidget(centerContainer, 1);
	rootLayout->addWidget(bottomContainer, 0);

	this->setLayout(rootLayout);
}

UIPaneFrame::~UIPaneFrame()
{

}

void UIPaneFrame::removeLayoutSpacing(QBoxLayout *layout)
{
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
}

