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
 * Contains the abstract Parser class. Parsers are objects capable of reading
 * a certain file format and transforming it into a node.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_HPP_
#define _OUSIA_PARSER_HPP_

#include <istream>
#include <set>
#include <string>

#include <core/managed/Managed.hpp>
#include <core/model/Node.hpp>

namespace ousia {

// Forward declarations
class CharReader;
class ParserContext;

/**
 * Abstract parser class. This class builds the basic interface that should be
 * used by any parser which reads data from an input stream and transforms it
 * into an Ousía node graph.
 */
class Parser {
protected:
	/**
	 * Parses the given input stream and returns a corresponding node for
	 * inclusion in the document graph. This method should be overridden by
	 * derived classes.
	 *
	 * @param reader is a reference to the CharReader that should be used.
	 * @param ctx is a reference to the context that should be used while
	 * parsing the document.
	 * @return a reference to the node representing the subgraph that has been
	 * created. The resulting node may point at not yet resolved entities, the
	 * calling code will try to resolve these. If no valid node can be produced,
	 * a corresponding LoggableException must be thrown by the parser.
	 */
	virtual Rooted<Node> doParse(CharReader &reader, ParserContext &ctx) = 0;

public:
	/**
	 * Default constructor.
	 */
	Parser() {}

	/**
	 * No copy construction.
	 */
	Parser(const Parser &) = delete;

	/**
	 * Virtual destructor.
	 */
	virtual ~Parser(){};

	/**
	 * Parses the given input stream and returns a corresponding node for
	 * inclusion in the document graph. This method should be overridden by
	 * derived classes.
	 *
	 * @param reader is a reference to the CharReader that should be used.
	 * @param ctx is a reference to the context that should be used while
	 * parsing the document.
	 * @return a reference to the node representing the subgraph that has been
	 * created. The resulting node may point at not yet resolved entities, the
	 * calling code will try to resolve these. If no valid node can be produced,
	 * a corresponding ParserException must be thrown by the parser.
	 */
	Rooted<Node> parse(CharReader &reader, ParserContext &ctx);

	/**
	 * Parses the given string and returns a corresponding node for
	 * inclusion in the document graph. This method should be overridden by
	 * derived classes.
	 *
	 * @param str is the string that should be parsed.
	 * @param ctx is a reference to the context that should be used while
	 * parsing the document.
	 * @return a reference to the node representing the subgraph that has been
	 * created. The resulting node may point at not yet resolved entities, the
	 * calling code will try to resolve these. If no valid node can be produced,
	 * a corresponding ParserException must be thrown by the parser.
	 */
	Rooted<Node> parse(const std::string &str, ParserContext &ctx);
};
}

#endif /* _OUSIA_PARSER_HPP_ */

