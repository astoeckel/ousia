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
 * @file Parser.hpp
 *
 * Contains the abstract "Parser" class. Parsers are objects capable of reading
 * a certain file format and transforming it into a node.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_HPP_
#define _OUSIA_PARSER_HPP_

#include <istream>
#include <set>
#include <string>

#include "Exceptions.hpp"
#include "Node.hpp"
#include "Logger.hpp"

namespace ousia {

// TODO: Implement a proper Mimetype class

/**
 * Exception to be thrown whenever an error occurs inside a specific parser.
 */
class ParserException : public LoggableException {
public:
	using LoggableException::LoggableException;
};

/**
 * Abstract parser class. This class builds the basic interface that should be
 * used by any parser which reads data from an input stream and transforms it
 * into an Ousía node graph.
 */
class Parser {
public:

	Parser() {};
	Parser(const Parser&) = delete;

	/**
	 * Returns a set containing all mime types supported by the parser. The mime
	 * types are used to describe the type of the document that is read by the
	 * parser. The default implementation returns an empty set. This method
	 * should be overridden by derived classes.
	 *
	 * @return a set containing the string value of the supported mime types.
	 */
	virtual std::set<std::string> mimetypes()
	{
		return std::set<std::string>{};
	};

	/**
	 * Parses the given input stream and returns a corresponding node for
	 * inclusion in the document graph. This method should be overridden by
	 * derived classes.
	 *
	 * @param is is a reference to the input stream that should be parsed.
	 * @param context defines the context in which the input stream should be
	 * parsed. The context represents the scope from which element names should
	 * be looked up.
	 * @param logger is a reference to the Logger instance that should be used
	 * to log error messages and warnings that occur while parsing the document.
	 * @return a reference to the node representing the subgraph that has been
	 * created. The resulting node may point at not yet resolved entities, the
	 * calling code will try to resolve these. If no valid node can be produced,
	 * a corresponding LoggableException must be thrown by the parser.
	 */
	virtual Rooted<Node> parse(std::istream &is, Handle<Node> context,
	                           Logger &logger) = 0;

	/**
	 * Parses the given string and returns a corresponding node for
	 * inclusion in the document graph. This method should be overridden by
	 * derived classes.
	 *
	 * @param str is the string that should be parsed.
	 * @param context defines the context in which the input stream should be
	 * parsed. The context represents the scope from which element names should
	 * be looked up.
	 * @param logger is a reference to the Logger instance that should be used
	 * to log error messages and warnings that occur while parsing the document.
	 * @return a reference to the node representing the subgraph that has been
	 * created. The resulting node may point at not yet resolved entities, the
	 * calling code will try to resolve these. If no valid node can be produced,
	 * a corresponding ParserException must be thrown by the parser.
	 */
	Rooted<Node> parse(const std::string &str, Handle<Node> context,
	                   Logger &logger);
};
}

#endif /* _OUSIA_PARSER_HPP_ */

