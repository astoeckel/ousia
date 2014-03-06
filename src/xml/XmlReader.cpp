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

#include <functional>
#include <iostream>
#include <sstream>

#include "XmlAttributeHandler.hpp"
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
			/* This is Benjamins noob way of handling things: We just ignore them.
				case QXmlStreamReader::EndElement:
				// Expected tag was not found, instead we found a closing tag!
				// TODO: Use better logging mechanism!
				std::cout << "Expected one of the following tags: ("
						<<  XmlElementHandler::expectedElementsStr(handlers)
						<< "); but found end of element \""
						<< xml.name().toString().toStdString()
						<< "\" instead!" << std::endl;
				return false;*/
			default:
				continue;
		}
	}
	return false;
}

bool XmlReader::parseArguments(std::map<std::string, XmlAttributeHandler> &handlers)
{
	// Iterate the attributes of the current xml node
	for (auto &attr : xml.attributes()) {
		// Convert the name to a std string
		const std::string name = attr.name().toString().toStdString();
		const std::string value = attr.value().toString().toStdString();

		// Try to fetch a corresponding attribute in the handlers map
		auto it = handlers.find(name);
		if (it != handlers.end()) {
			XmlAttributeHandler &handler = (*it).second;
			if (handler.isValid(value)) {
				handler.executeSettter(value);
			} else {
				std::cout << "Invalid attribute value \"" << value
						<< "\" for attribute " << name << std::endl;
				return false;
			}
		} else {
			std::cout << "Unexpected attribute " << name << std::endl;
			return false;
		}
	}

	// Iterate over all handlers to check whether all required handlers have
	// been handled and in order to pass the default value to unhandled handlers
	for (auto &it : handlers) {
		// Fetch the name of the attribute and the handler
		const std::string &name = it.first;
		XmlAttributeHandler &handler = it.second;
		if (!handler.isHandled()) {
			if (handler.isRequired()) {
				std::cout << "Attribute " << name
						<< " is required but was not set!" << std::endl;
				return false;
			} else if (handler.getDefaultValue()) {
				handler.executeSettter(handler.getDefaultValue());
			}
		}
	}

	return true;
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
	std::shared_ptr<model::domain::Domain> res{new model::domain::Domain()};
	std::map<std::string, XmlAttributeHandler> handlers{
		std::make_pair("name", XmlAttributeHandler(
			true,
			[&](const std::string& v) -> bool {return true;}, 
			[&](const std::string& v) -> void {res->setName(v);}
		))
	};
	if (!parseArguments(handlers)) {
		std::cout << "Errors while parsing arguments for domain node!" << std::endl;
		return nullptr;
	}
	std::cout << res->getName() << std::endl;
	return res;
}

}
}

