#include <gui/notepad.hpp>
#include <QApplication>
#include <QFile>
#include <QIODevice>
#include <QXmlStreamReader>

#include <vector>
#include <iostream>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Notepad w;
	w.show();

	// Open the file given as first argument
/*	QFile file(argv[1]);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cout << "Error while opening file " << argv[1] << std::endl;
		return 1;
	}

	// Read all tags using the xml stream reader
	QXmlStreamReader xml(&file);

	while (!xml.atEnd()) {
		xml.readNext();
	}
	if (xml.hasError()) {
		std::cout << "Error while parsing XML: " << xml.errorString().toStdString() << " at line " << xml.lineNumber() << std::endl;
		return 1;
	}
	return 0;*/

	return a.exec();
}
