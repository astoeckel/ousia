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

#include "Domain.hpp"
#include "Document.hpp"
#include "Project.hpp"
#include "Typesystem.hpp"

namespace ousia {

namespace model {

Project::Project(Manager &mgr)
    : Node(mgr),
      systemTypesystem(acquire(new SystemTypesystem(mgr))),
      documents(this),
      domains(this),
      typesystems(this)
{
}

bool Project::doValidate(Logger &logger) const
{
	return continueValidation(documents, logger) &
	       continueValidation(domains, logger) &
	       continueValidation(typesystems, logger);
}

Rooted<SystemTypesystem> Project::getSystemTypesystem()
{
	return systemTypesystem;
}

Rooted<Typesystem> Project::createTypesystem(const std::string &name)
{
	Rooted<Typesystem> typesystem{
	    new Typesystem{getManager(), systemTypesystem, name}};
	addTypesystem(typesystem);
	return typesystem;
}

void Project::addTypesystem(Handle<Typesystem> typesystem)
{
	invalidate();
	typesystems.push_back(typesystem);
}

Rooted<Document> Project::createDocument(const std::string &name)
{
	Rooted<Document> document{new Document(getManager(), name)};
	addDocument(document);
	return document;
}

void Project::addDocument(Handle<Document> document)
{
	invalidate();
	documents.push_back(document);
}

Rooted<Domain> Project::createDomain(const std::string &name)
{
	Rooted<Domain> domain{new Domain(getManager(), systemTypesystem, name)};
	addDomain(domain);
	return domain;
}

void Project::addDomain(Handle<Domain> domain)
{
	invalidate();
	domains.push_back(domain);
}

const NodeVector<Document> &Project::getDocuments() { return documents; }

const NodeVector<Domain> &Project::getDomains() { return domains; }

const NodeVector<Typesystem> &Project::getTypesystems() { return typesystems; }
}

namespace RttiTypes {
const Rtti Project = RttiBuilder<model::Project>("Project")
                         .parent(&Node)
                         .composedOf(&Document)
                         .composedOf(&Typesystem)
                         .composedOf(&SystemTypesystem)
                         .composedOf(&Domain);
}
}

