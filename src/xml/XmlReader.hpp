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

#ifndef _OUSIA_XML_XML_READER_HPP_
#define _OUSIA_XML_XML_READER_HPP_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <model/GraphNode.hpp>
#include <model/domain/Domain.hpp>

class QXmlStreamReader;

namespace ousia {
namespace xml {

class XmlElementHandler;
class XmlAttributeHandler;

/**
 * The XmlReader class is responsible for parsing the ousia XML documents and
 * deserializing them into the internal object representation.
 */
class XmlReader {

private:
	/**
	 * Reference to the QXmlStreamReader used for accessing the XML data on a
	 * token basis.
	 */
	QXmlStreamReader &xml;

	/**
	 * Parses a domain definition from the XML file.
	 */
	std::shared_ptr<model::domain::Domain> readDomain();

	/**
	 * Used internally in order to conveniently expect one xml tag in a set of
	 * elements. Returns true if there was an error while waiting for the tag,
	 * false otherwise.
	 */
	bool expectOneOf(std::vector<XmlElementHandler> &handlers);

	/**
	 * Used internally to parse the current argument map.
	 */
	bool parseArguments(std::map<std::string, XmlAttributeHandler> &handlers);

public:

	/**
	 * Instanciates the XMLReader class for the given instance of the
	 * QXMLStreamReader class.
	 */
	XmlReader(QXmlStreamReader &xml);

	/**
	 * Starts processing the xml and returns the generated graph node.
	 */
	std::shared_ptr<model::GraphNode> process();

};

}
}

#endif /* _OUSIA_XML_XML_READER_HPP_ */

