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
#include <core/common/Rtti.hpp>
#include <core/model/Node.hpp>
#include <core/model/Project.hpp>

namespace ousia {

// Forward declaration
class Logger;
class ParserScope;
class Registry;
class ResourceManager;

/**
 * Class containing the objects that are passed to a parser instance.
 */
class ParserContext {
private:
	/**
	 * Reference at the internally used Registry instance.
	 */
	Registry &registry;

	/**
	 * ResourceManager used to manage all resources used by the project.
	 */
	ResourceManager &resourceManager;

	/**
	 * Reference to the ParserScope instance that should be used within the
	 * parser.
	 */
	ParserScope &scope;

	/**
	 * Project instance into which the new content should be parsed.
	 */
	Rooted<Project> project;

	/**
	 * Reference to the Logger the parser should log any messages to.
	 */
	Logger &logger;

	/**
	 * SourceId is the ID of the resource that is currently being processed.
	 */
	SourceId sourceId;

public:
	/**
	 * Constructor of the ParserContext class.
	 *
	 * @param registry is the registry instance that should be used for locating
	 * files and finding parsers for these files.
	 * @param resourceManager is a reference at the ResourceManager which
	 * manages the inclusion of source files.
	 * @param scope is a reference to the ParserScope instance that should be
	 * used to lookup names.
	 * @param project is the project into which the content should be parsed.
	 * @param logger is a reference to the Logger instance that should be used
	 * to log error messages and warnings that occur while parsing the document.
	 * @param sourceId is a SourceId instance specifying the source the parser
	 * is reading from.
	 */
	ParserContext(Registry &registry, ResourceManager &resourceManager,
	              ParserScope &scope, Handle<Project> project, Logger &logger,
	              SourceId sourceId = InvalidSourceId);

	/**
	 * Parses a file with ParserContext and an empty ParserScope. The parsed
	 * object graph of files that are parsed using the "import" function is
	 * cached (in contrast to the "include" function). A copy of this parser
	 * context will be passed to the called parser, with the ParserScope
	 * reference stored in the "scope" variable exchanged by an empty scope.
	 *
	 * @param path is the path of the file that should be parsed.
	 * @param mimetype is the mimetype of the resource that should be parsed
	 * (may be empty, in which case the mimetype is deduced from the file
	 * extension).
	 * @param rel is a "relation string" supplied by the user which specifies
	 * the relationship of the specified resource. May be empty, in which case
	 * the relation is deduced from the supported types and the types of the
	 * parser for the given mimetype.
	 * @param supportedTypes contains the types of the returned Node the caller
	 * can deal with. Note that only the types the parser claims to return are
	 * checked, not the actual result.
	 * @return the parsed node or nullptr if something goes wrong.
	 */
	Rooted<Node> import(const std::string &path, const std::string mimetype,
	                    const std::string rel, const RttiSet &supportedTypes);

	/**
	 * Parses a file with ParserContext and the current ParserScope. In contrast
	 * to the "import" function, include() does not cache the parsed node (as it
	 * depends on the current ParserScope).
	 *
	 * @param path is the path of the file that should be parsed.
	 * @param mimetype is the mimetype of the resource that should be parsed
	 * (may be empty, in which case the mimetype is deduced from the file
	 * extension).
	 * @param rel is a "relation string" supplied by the user which specifies
	 * the relationship of the specified resource. May be empty, in which case
	 * the relation is deduced from the supported types and the types of the
	 * parser for the given mimetype.
	 * @param supportedTypes contains the types of the returned Node the caller
	 * can deal with. Note that only the types the parser claims to return are
	 * checked, not the actual result.
	 * @return the parsed nodes or an empty list if something goes wrong (or
	 * there were indeed no objects to be parsed).
	 */
	NodeVector<Node> include(const std::string &path,
	                         const std::string mimetype, const std::string rel,
	                         const RttiSet &supportedTypes);

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
	 * Returns a reference pointing at the Registry used within this parser
	 * context.
	 *
	 * @return a reference at the registry instance.
	 */
	Registry &getRegistry() const { return registry; }

	/**
	 * Returns a reference pointing at the ResourceManager used within this
	 * parser context.
	 *
	 * @return a reference at the ResourceManager instance.
	 */
	ResourceManager &getResourceManager() const { return resourceManager; }

	/**
	 * Returns a reference pointing at the current ParserScope instance.
	 *
	 * @return a reference at the parser scope object that should be used during
	 * the parsing process.
	 */
	ParserScope &getScope() const { return scope; }

	/**
	 * Returns a handle pointing at the Project node.
	 *
	 * @return a project node handle.
	 */
	Rooted<Project> getProject() const { return project; }

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

