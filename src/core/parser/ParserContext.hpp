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
 * @file ParserContext.hpp
 *
 * Contains the ParserContext, a struct containing references to all the
 * important structures a Parser needs to access while parsing an input stream.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_CONTEXT_HPP_
#define _OUSIA_PARSER_CONTEXT_HPP_

#include <core/managed/Managed.hpp>
#include <core/model/Node.hpp>
#include <core/model/Project.hpp>

namespace ousia {

// Forward declaration
class Logger;
class ParserScope;
class Registry;

/**
 * Struct containing the objects that are passed to a parser instance.
 */
struct ParserContext {
	/**
	 * Reference to the ParserScope instance that should be used within the parser.
	 */
	ParserScope &scope;

	/**
	 * Reference to the Registry instance that should be used within the parser.
	 */
	Registry &registry;

	/**
	 * Reference to the Logger the parser should log any messages to.
	 */
	Logger &logger;

	/**
	 * Reference to the Manager the parser should append nodes to.
	 */
	Manager &manager;

	/**
	 * Project instance into which the new content should be parsed.
	 */
	Rooted<model::Project> project;

	/**
	 * Constructor of the ParserContext class.
	 *
	 * @param scope is a reference to the ParserScope instance that should be used to
	 * lookup names.
	 * @param registry is a reference at the Registry class, which allows to
	 * obtain references at parsers for other formats or script engine
	 * implementations.
	 * @param logger is a reference to the Logger instance that should be used
	 * to log error messages and warnings that occur while parsing the document.
	 * @param manager is a Reference to the Manager the parser should append
	 * nodes to.
	 * @param project is the project into which the content should be parsed.
	 */
	ParserContext(ParserScope &scope, Registry &registry, Logger &logger,
	              Manager &manager, Handle<model::Project> project);
};

}

#endif /* _OUSIA_PARSER_CONTEXT_HPP_ */

