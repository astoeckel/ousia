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

#include "Document.hpp"

#include <core/common/Exceptions.hpp>
#include <core/common/Rtti.hpp>

namespace ousia {
namespace model {

int DocumentEntity::getFieldDescriptorIndex(const std::string &fieldName)
{
	const NodeVector<FieldDescriptor> &fds = descriptor->getFieldDescriptors();
	unsigned int f = 0;

	// look if we have an empty name.
	if (fieldName == "") {
		// in that case we look for a default field.
		// First: Do we only have one field?
		if (fds.size() == 1) {
			// if so we return that one.
			return f;
		}
		// Second: Do we have a TREE field?
		for (auto &fd : fds) {
			if (fd->getFieldType() == FieldDescriptor::FieldType::TREE) {
				return f;
			}
			f++;
		}
	} else {
		// otherwise we return the FieldDescriptor with the correct name (if
		// such a descriptor exists).
		for (auto &fd : fds) {
			if (fd->getName() == fieldName) {
				return f;
			}
			f++;
		}
	}
	return -1;
}

NodeVector<StructuredEntity> &DocumentEntity::getField(
    const std::string &fieldName)
{
	int f = getFieldDescriptorIndex(fieldName);
	if (f < 0) {
		throw OusiaException("No field for the given name exists!");
	}
	return fields[f];
}

NodeVector<StructuredEntity> &DocumentEntity::getField(
    Handle<FieldDescriptor> fieldDescriptor)
{
	if (fieldDescriptor.isNull()) {
		throw OusiaException("The given FieldDescriptor handle is null!");
	}
	const NodeVector<FieldDescriptor> &fds = descriptor->getFieldDescriptors();
	int f = 0;
	for (auto &fd : fds) {
		if (fd->getName() == fieldDescriptor->getName() &&
		    fd->getFieldType() == fieldDescriptor->getFieldType()) {
			return fields[f];
		}
		f++;
	}
	throw OusiaException(
	    "The given FieldDescriptor is not specified in the Descriptor of this "
	    "node.");
}

static Rooted<StructuredClass> resolveDescriptor(
    std::vector<Handle<Domain>> domains, const std::string &className)
{
	// iterate over all domains.
	for (auto &d : domains) {
		// use the actual resolve method.
		std::vector<ResolutionResult> resolved = d->resolve(className, typeOf<StructuredClass>());
		// if we don't find anything, continue.
		if (resolved.size() == 0) {
			continue;
		}
		// Otherwise take the first valid result.
		for (auto &r : resolved) {
			return r.node.cast<StructuredClass>();
		}
	}
	return {nullptr};
}

Rooted<StructuredEntity> StructuredEntity::buildRootEntity(
    Handle<Document> document, std::vector<Handle<Domain>> domains,
    const std::string &className, Variant attributes, std::string name)
{
	// If the parent is not set, we can not build the entity.
	if (document == nullptr) {
		return {nullptr};
	}
	// If we can not find the correct descriptor, we can not build the entity
	// either.
	Rooted<StructuredClass> descriptor = resolveDescriptor(domains, className);
	if (descriptor == nullptr) {
		return {nullptr};
	}
	// Then construct the StructuredEntity itself.
	Rooted<StructuredEntity> root{
	    new StructuredEntity(document->getManager(), document, descriptor,
	                         attributes, std::move(name))};
	// append it to the document.
	document->setRoot(root);
	// and return it.
	return root;
}

Rooted<StructuredEntity> StructuredEntity::buildEntity(
    Handle<DocumentEntity> parent, std::vector<Handle<Domain>> domains,
    const std::string &className, const std::string &fieldName,
    Variant attributes, std::string name)
{
	// If the parent is not set, we can not build the entity.
	if (parent == nullptr) {
		return {nullptr};
	}
	// If we can not find the correct descriptor, we can not build the entity
	// either.
	Rooted<StructuredClass> descriptor = resolveDescriptor(domains, className);
	if (descriptor == nullptr) {
		return {nullptr};
	}
	// Then construct the StructuredEntity itself.
	Rooted<StructuredEntity> entity{new StructuredEntity(
	    parent->getManager(), parent, descriptor, attributes, std::move(name))};
	// if the field does not exist, return null handle as well.
	if (!parent->hasField(fieldName)) {
		return {nullptr};
	}
	// append the new entity to the right field.
	NodeVector<StructuredEntity> &field = parent->getField(fieldName);
	field.push_back(entity);

	// and return it.
	return entity;
}

Rooted<DocumentPrimitive> DocumentPrimitive::buildEntity(
    Handle<DocumentEntity> parent, Variant content,
    const std::string &fieldName)
{
	// If the parent is not set, we can not build the entity.
	if (parent == nullptr) {
		return {nullptr};
	}
	// Then construct the StructuredEntity itself.
	Rooted<DocumentPrimitive> entity{
	    new DocumentPrimitive(parent->getManager(), parent, content)};
	// if the field does not exist, return null handle as well.
	if (!parent->hasField(fieldName)) {
		return {nullptr};
	}
	// append the new entity to the right field.
	NodeVector<StructuredEntity> &field = parent->getField(fieldName);
	field.push_back(entity);

	// and return it.
	return entity;
}

/* Type registrations */
}

namespace RttiTypes {
const Rtti<model::DocumentEntity> DocumentEntity =
    RttiBuilder("DocumentEntity").parent(&Node);
const Rtti<model::Document> Document =
    RttiBuilder("Document").parent(&Node).composedOf(
        {&AnnotationEntity, &StructuredEntity});
const Rtti<model::AnnotationEntity> AnnotationEntity =
    RttiBuilder("AnnotationEntity").parent(&DocumentEntity).composedOf(
        &StructuredEntity);
const Rtti<model::StructuredEntity> StructuredEntity =
    RttiBuilder("StructuredEntity").parent(&DocumentEntity).composedOf(
        {&StructuredEntity, &Anchor, &DocumentPrimitive});
const Rtti<model::DocumentPrimitive> DocumentPrimitive =
    RttiBuilder("DocumentPrimitive").parent(&StructuredEntity);
const Rtti<model::AnnotationEntity::Anchor> Anchor =
    RttiBuilder("Anchor").parent(&StructuredEntity);
}
}

