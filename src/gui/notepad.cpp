#include <gui/notepad.hpp>

#include <gui/components/UIPaneFrame.hpp>
//#include <gui/components/UIGrid.hpp>

Notepad::Notepad(QWidget *parent) :
	QMainWindow(parent)
{
	UIPaneFrame *frame = new UIPaneFrame(this);
	this->setCentralWidget(frame);
//	resize(1024, 768);
//	uigrid::UIGrid *grid = new uigrid::UIGrid(this);
//	setCentralWidget(grid);
}

Notepad::~Notepad()
{

}
