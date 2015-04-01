/*
    Ousía
    Copyright (C) 2015  Benjamin Paaßen, Andreas Stöckel

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

/**
 * @file TestXmlParser.hpp
 *
 * Contains a method to parse a XML file into a simple tree representation.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_TEST_XML_PARSER_HPP_
#define _OUSIA_TEST_XML_PARSER_HPP_

#include <memory>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "TestLogger.hpp"

namespace ousia {
namespace test {

/**
 * Class used to represent a XML file.
 */
struct XmlNode {
	std::weak_ptr<XmlNode> parent;
	std::vector<std::shared_ptr<XmlNode>> children;
	std::map<std::string, std::string> attributes;
	std::string name;
	std::string text;
	int line;
	int column;

	/**
	 * Default constructor, creates a root node.
	 */
	XmlNode() : line(0), column(0) {}

	/**
	 * Creates a node for the given parent with the given name.
	 *
	 * @param parent is the parent XML node.
	 */
	XmlNode(std::weak_ptr<XmlNode> parent, const std::string &name)
	    : parent(parent), name(name), line(0), column(0)
	{
	}

	/**
	 * Returns the path to this node.
	 */
	std::string path();

	/**
	 * Compares two XmlNode instances, logs differences to the given
	 * test::logger instance.
	 */
	bool compareTo(Logger &logger, std::shared_ptr<XmlNode> other,
	               std::set<int> &errExpected, std::set<int> &errActual);
};

std::pair<bool, std::shared_ptr<XmlNode>> parseXml(Logger &logger,
                                                       std::istream &is,
                                                       std::set<int> &errLines);

}
}

#endif /* _OUSIA_TEST_XML_PARSER_HPP_ */
