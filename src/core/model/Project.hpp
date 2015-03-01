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
 * which ontologies, documents, typesystems and other resources are embedded.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PROJECT_HPP_
#define _OUSIA_PROJECT_HPP_

#include <core/common/Rtti.hpp>

#include "RootNode.hpp"

namespace ousia {

// Forward declarations
class Logger;
class Registry;
class SystemTypesystem;
class Typesystem;
class Document;
class Ontology;

/**
 * The Project class constitutes the top-level node in which a collection of
 * documents are stored. It also contains an instance of the SystemTypesystem
 * and allows for simple creation of new Typesystem and Ontology instances.
 */
class Project : public RootNode {
private:
	/**
	 * Private instance of the system typesystem which is distributed as a
	 * reference to all child typesystems.
	 */
	Owned<SystemTypesystem> systemTypesystem;

	/**
	 * List containing all loaded documents.
	 */
	NodeVector<Document> documents;

protected:
	bool doValidate(Logger &loger) const override;
	void doResolve(ResolutionState &state) override;
	void doReference(Handle<Node> node) override;
	RttiSet doGetReferenceTypes() const override;

public:
	/**
	 * Constructor of the Project class.
	 *
	 * @param mgr is the manager instance used for managing this Node.
	 */
	Project(Manager &mgr);

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
	 * Returns a new ontology with the given name and adds it to the list of
	 * ontologies. Provides a reference of the system typesystem to the ontology.
	 *
	 * @param name is the name of the ontology that should be created.
	 */
	Rooted<Ontology> createOntology(const std::string &name);

	/**
	 * Adds the given document to the list of documents in the project.
	 *
	 * @param document is the document that should be added to the project.
	 */
	void referenceDocument(Handle<Document> document);

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

