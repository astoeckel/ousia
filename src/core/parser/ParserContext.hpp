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

#include <core/common/Location.hpp>
#include <core/managed/Managed.hpp>
#include <core/model/Project.hpp>

#include "ParserScope.hpp"

namespace ousia {

// Forward declaration
class Logger;

/**
 * Class containing the objects that are passed to a parser instance.
 */
class ParserContext {
private:
	/**
	 * Project instance into which the new content should be parsed.
	 */
	Rooted<Project> project;

	/**
	 * Reference to the ParserScope instance that should be used within the
	 * parser.
	 */
	ParserScope &scope;

	/**
	 * SourceId is the ID of the resource that is currently being processed.
	 */
	SourceId sourceId;

	/**
	 * Reference to the Logger the parser should log any messages to.
	 */
	Logger &logger;

public:
	/**
	 * Constructor of the ParserContext class.
	 *
	 * @param project is the project into which the content should be parsed.
	 * @param scope is a reference to the ParserScope instance that should be
	 * used to lookup names.
	 * @param sourceId is a SourceId instance specifying the source the parser
	 * is reading from.
	 * @param logger is a reference to the Logger instance that should be used
	 * to log error messages and warnings that occur while parsing the document.
	 */
	ParserContext(Handle<Project> project, ParserScope &scope,
	              SourceId sourceId, Logger &logger);

	/**
	 * Constructor of the ParserContext class with the sourceId being set
	 * to the InvalidSourceId value.
	 *
	 * @param project is the project into which the content should be parsed.
	 * @param scope is a reference to the ParserScope instance that should be
	 * used to lookup names.
	 * @param logger is a reference to the Logger instance that should be used
	 * to log error messages and warnings that occur while parsing the document.
	 */
	ParserContext(Handle<Project> project, ParserScope &scope,
	              Logger &logger);

	/**
	 * Clones the ParserContext instance but exchanges the ParserScope instance
	 * and the source id.
	 *
	 * @param scope is a reference at the new ParserScope instance.
	 * @param sourceId is the source id the parser is reading from.
	 * @return a copy of this ParserContext with exchanged scope and source id.
	 */
	ParserContext clone(ParserScope &scope, SourceId sourceId) const;

	/**
	 * Clones the ParserContext instance but exchanges the source id.
	 *
	 * @param sourceId is the source id the parser is reading from.
	 * @return a copy of this ParserContext with exchanged source id.
	 */
	ParserContext clone(SourceId sourceId) const;

	/**
	 * Returns a handle pointing at the Project node.
	 *
	 * @return a project node handle.
	 */
	Rooted<Project> getProject() const { return project; }

	/**
	 * Returns a reference pointing at the current ParserScope instance.
	 *
	 * @return a reference at the parser scope object that should be used during
	 * the parsing process.
	 */
	ParserScope &getScope() const { return scope; }

	/**
	 * Returns a reference pointing at the current LoggerInstance.
	 *
	 * @return a reference at LoggerInstance to which all error messages should
	 * be logged.
	 */
	Logger &getLogger() const { return logger; }

	/**
	 * Returns a reference pointing at the manager instance that should be used
	 * when creating new Managed objects.
	 *
	 * @return a reference pointing at the underlying Manager instance.
	 */
	Manager &getManager() const;

	/**
	 * Returns the SourceId instance which specifies the source file the parser
	 * is currently reading from.
	 *
	 * @return the current source id.
	 */
	SourceId getSourceId() const { return sourceId; }
};
}

#endif /* _OUSIA_PARSER_CONTEXT_HPP_ */

