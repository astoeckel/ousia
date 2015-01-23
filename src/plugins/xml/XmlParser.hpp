/*
    Ousía
    Copyright (C) 2014, 2015  Benjamin Paaßen, Andreas Stöckel

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
 * @file XmlParser.hpp
 *
 * Contains the parser responsible for reading Ousía XML Documents (extension
 * oxd) and Ousía XML Modules (extension oxm).
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_XML_PARSER_HPP_
#define _OUSIA_XML_PARSER_HPP_

#include <core/parser/Parser.hpp>

namespace ousia {

/**
 * The XmlParser class implements parsing the various types of Ousía XML
 * documents using the expat stream XML parser.
 */
class XmlParser : public Parser {
protected:
	/**
	 * Parses the given input stream as XML file and returns the parsed
	 * top-level node.
	 *
	 * @param reader is the CharReader from which the input should be read.
	 * @param ctx is a reference to the ParserContext instance that should be
	 * used.
	 */
	Rooted<Node> doParse(CharReader &reader, ParserContext &ctx) override;
};

}

#endif /* _OUSIA_XML_PARSER_HPP_ */

