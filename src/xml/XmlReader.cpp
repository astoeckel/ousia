/*
    Ousía
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

#include <QXmlStreamReader>

#include <sstream>
#include <iostream>

#include "XmlElementHandler.hpp"
#include "XmlReader.hpp"

namespace ousia {
namespace xml {

XmlReader::XmlReader(QXmlStreamReader &xml) :
	xml(xml)
{
	// Do nothing here
}

bool XmlReader::expectOneOf(std::vector<XmlElementHandler> &handlers)
{
	// Skip all tokens except for "start element" and "end element"
	while (!xml.atEnd()) {
		// TODO: Implement mechanism for using the current state of the
		// XmlStreamReader instead of always reading the next token?
		const auto tokenType = xml.readNext();
		switch (tokenType) {
			case QXmlStreamReader::StartElement:
				for (auto &h : handlers) {
					if (h.matches(xml.name())) {
						return h.execute();
					}
				}
				// Expected tag was not found, display error message
				// TODO: Use better logging mechanism!
				std::cout << "Expected one of the following tags: ("
						<<  XmlElementHandler::expectedElementsStr(handlers)
						<< "); but found element \""
						<< xml.name().toString().toStdString()
						<< "\" instead!" << std::endl;
				return false;
			case QXmlStreamReader::EndElement:
				// Expected tag was not found, instead we found a closing tag!
				// TODO: Use better logging mechanism!
				std::cout << "Expected one of the following tags: ("
						<<  XmlElementHandler::expectedElementsStr(handlers)
						<< "); but found end of element \""
						<< xml.name().toString().toStdString()
						<< "\" instead!" << std::endl;
				return false;
			default:
				continue;
		}
	}
	return false;
}

std::shared_ptr<model::GraphNode> XmlReader::process()
{
	std::shared_ptr<model::GraphNode> res{nullptr};
	std::vector<XmlElementHandler> handlers{
		{"domain", [&](){return (res = this->readDomain()) != nullptr;}}
	};
	if (!expectOneOf(handlers)) {
		std::cout << "Errors occured while parsing XML file!" << std::endl;
		return nullptr;
	}
	return res;
}

std::shared_ptr<model::domain::Domain> XmlReader::readDomain()
{
	if (!xml.attributes().hasAttribute("name")) {
		std::cout << "Expected name attribute!" << std::endl;
		return nullptr;
	}
	std::cout << "domain name: " << xml.attributes().value("name").toString().toStdString() << std::endl;
	return std::shared_ptr<model::domain::Domain>(new model::domain::Domain());
}

}
}

