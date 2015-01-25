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
 * @file Project.hpp
 *
 * Contains the concept of the "Project" class which represents the entity into
 * which domains, documents, typesystems and other resources are embedded.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PROJECT_HPP_
#define _OUSIA_PROJECT_HPP_

#include <core/resource/ResourceManager.hpp>

#include "Node.hpp"

namespace ousia {

// Forward declarations
class Logger;
class Rtti;
class Registry;
class ParserContext;
class SystemTypesystem;
class Typesystem;
class Document;
class Domain;

/**
 * The Project class constitutes the top-level node in which a collection of
 * documents are stored. It also contains an instance of the SystemTypesystem
 * and allows for simple creation of new Typesystem and Domain instances.
 */
class Project : public Node {
private:
	/**
	 * Reference at the internally used Registry instance.
	 */
	Registry &registry;

	/**
	 * Private instance of the system typesystem which is distributed as a
	 * reference to all child typesystems.
	 */
	Owned<SystemTypesystem> systemTypesystem;

	/**
	 * List containing all loaded documents.
	 */
	NodeVector<Document> documents;

	/**
	 * ResourceManager used to manage all resources used by the project.
	 */
	ResourceManager resourceManager;

protected:
	/**
	 * Validates the project and all parts it consists of.
	 *
	 * @param logger is the logger instance to which errors will be logged.
	 */
	bool doValidate(Logger &loger) const override;

public:
	/**
	 * Constructor of the Project class.
	 *
	 * @param mgr is the manager instance used for managing this Node.
	 * @param registry is the registry instance that should be used for locating
	 * files and finding parsers for these files.
	 */
	Project(Manager &mgr, Registry &registry);

	/**
	 * Parses a file with the given Logger in an empty ParserScope. This
	 * function is meant to be called by the top-level (e.g. a main function)
	 * and not by other parsers. These should use the link and include methods
	 * instead.
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
	 * @param logger is the logger that should be used
	 * @return the parsed node or nullptr if something goes wrong.
	 */
	Rooted<Node> parse(const std::string &path, const std::string mimetype,
	                   const std::string rel, const RttiSet &supportedTypes,
	                   Logger &logger);

	/**
	 * Parses a file with ParserContext and an empty ParserScope. The parsed
	 * object graph of files that are parsed using the "link" function is
	 * cached (in contrast to the "include" function).
	 *
	 * @param ctx is the ParserContext that should be passed to the underlying
	 * parser. The scope in the ParserContext will be exchanged.
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
	Rooted<Node> link(ParserContext &ctx, const std::string &path,
	                  const std::string mimetype, const std::string rel,
	                  const RttiSet &supportedTypes);

	/**
	 * Parses a file with ParserContext and the current ParserScope. In contrast
	 * to the "link" function, include() does not cache the parsed node (as it
	 * depends on the current ParserScope).
	 *
	 * @param ctx is the ParserContext that should be passed to the underlying
	 * parser. The scope in the ParserContext will be exchanged.
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
	Rooted<Node> include(ParserContext &ctx, const std::string &path,
	                     const std::string mimetype, const std::string rel,
	                     const RttiSet &supportedTypes);

	/**
	 * Returns a SourceContextCallback that can be passed to a logger instance.
	 * Remeber to reset the SourceContextCallback after the Project instance has
	 * been freed.
	 *
	 * @return a SourceContextCallback that is coupled to this Project instance.
	 */
	SourceContextCallback getSourceContextCallback();

	/**
	 * Returns a reference to the internal system typesystem.
	 *
	 * @return a reference to the system typesystem.
	 */
	Rooted<SystemTypesystem> getSystemTypesystem();

	/**
	 * Returns a new typesystem with the given name adds it to the list of
	 * typesystems. Provides a reference of the system typesystem to the
	 * typesystem.
	 *
	 * @param name is the name of the typesystem that should be created.
	 */
	Rooted<Typesystem> createTypesystem(const std::string &name);

	/**
	 * Returns a new document with the given name and adds it to the list of
	 * documents.
	 *
	 * @param name is the name of the document that should be created.
	 */
	Rooted<Document> createDocument(const std::string &name);

	/**
	 * Returns a new domain with the given name and adds it to the list of
	 * domains. Provides a reference of the system typesystem to the domain.
	 *
	 * @param name is the name of the domain that should be created.
	 */
	Rooted<Domain> createDomain(const std::string &name);

	/**
	 * Adds the given document to the list of documents in the project.
	 *
	 * @param document is the document that should be added to the project.
	 */
	void addDocument(Handle<Document> document);

	/**
	 * Returns all documents of this project.
	 *
	 * @return a reference pointing at the document list.
	 */
	const NodeVector<Document> &getDocuments() const;
};

namespace RttiTypes {
/**
 * Type information of the Project class.
 */
extern const Rtti Project;
}
}

#endif /* _OUSIA_PROJECT_HPP_ */

