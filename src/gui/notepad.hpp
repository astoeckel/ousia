#ifndef _NOTEPAD_H_
#define _NOTEPAD_H_

#include <QMainWindow>

class Notepad : public QMainWindow
{
	Q_OBJECT

public:
	explicit Notepad(QWidget *parent = 0);
	 ~Notepad();

};

#endif /* _NOTEPAD_H_ */

