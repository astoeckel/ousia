#include <QApplication>
#include <QFile>
#include <QIODevice>
#include <QXmlStreamReader>

#include <vector>
#include <iostream>

#include <model/GraphNode.hpp>

using namespace ousia;

int main(int argc, char *argv[])
{
	std::shared_ptr<GraphNode> nd1{new GraphNode("node1")};
	std::shared_ptr<GraphNode> nd2{new GraphNode("node2", nd1)};

	std::cout << nd2->getParent()->getName() << std::endl;

	return 0;

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
}

