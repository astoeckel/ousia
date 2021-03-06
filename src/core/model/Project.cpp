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

#include <core/common/RttiBuilder.hpp>

#include "Ontology.hpp"
#include "Document.hpp"
#include "Project.hpp"
#include "Typesystem.hpp"

namespace ousia {

Project::Project(Manager &mgr)
    : RootNode(mgr),
      systemTypesystem(acquire(new SystemTypesystem(mgr))),
      documents(this)
{
}

bool Project::doValidate(Logger &logger) const
{
	return continueValidation(documents, logger);
}

void Project::doResolve(ResolutionState &state){
	continueResolveComposita(documents, documents.getIndex(), state);
}

void Project::doReference(Handle<Node> node) {
	if (node->isa(&RttiTypes::Document)) {
		referenceDocument(node.cast<Document>());
	}
}

RttiSet Project::doGetReferenceTypes() const {
	return RttiSet{&RttiTypes::Document};
}

Rooted<SystemTypesystem> Project::getSystemTypesystem()
{
	return systemTypesystem;
}

Rooted<Typesystem> Project::createTypesystem(const std::string &name)
{
	return Rooted<Typesystem>{
	    new Typesystem{getManager(), systemTypesystem, name}};
}

Rooted<Document> Project::createDocument(const std::string &name)
{
	Rooted<Document> document{new Document(getManager(), name)};
	referenceDocument(document);
	return document;
}

Rooted<Ontology> Project::createOntology(const std::string &name)
{
	return Rooted<Ontology>{new Ontology(getManager(), systemTypesystem, name)};
}

void Project::referenceDocument(Handle<Document> document)
{
	invalidate();
	documents.push_back(document);
}

const NodeVector<Document> &Project::getDocuments() const { return documents; }

namespace RttiTypes {
const Rtti Project = RttiBuilder<ousia::Project>("Project")
                         .parent(&RootNode)
                         .composedOf(&Document)
                         .composedOf(&SystemTypesystem);
}
}

