#include <QApplication>
#include <QFile>
#include <QIODevice>
#include <QXmlStreamReader>

#include <vector>
#include <iostream>

#include <xml/XmlReader.hpp>

using namespace ousia::xml;

int main(int argc, char *argv[])
{
	// Open the file given as first argument
	if (argc < 2) {
		std::cout << "No filename specified!" << std::endl;
		return 1;
	}

	QFile file(argv[1]);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cout << "Error while opening file " << argv[1] << std::endl;
		return 1;
	}

	// Create the QXmlStreamReader instance
	QXmlStreamReader xml(&file);

	// Pass it to the XmlReader
	XmlReader xmlReader(xml);
	xmlReader.process();

	return 0;
}

