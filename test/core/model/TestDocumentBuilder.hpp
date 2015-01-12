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

#ifndef _MODEL_TEST_DOCUMENT_BUILDER_HPP_
#define _MODEL_TEST_DOCUMENT_BUILDER_HPP_

#include <sstream>

#include <core/common/Logger.hpp>
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Typesystem.hpp>

namespace ousia {
namespace model {

typedef std::vector<std::string> Path;

/* Class StructuredEntity */

static std::string getPathString(const Path &path)
{
	std::stringstream ss;
	ss << path[0];
	for (size_t i = 1; i < path.size(); i++) {
		ss << '.';
		ss << path[i];
	}
	return std::move(ss.str());
}

static Rooted<Descriptor> resolveDescriptor(Handle<Document> doc,
                                            Logger &logger, const Path &path,
                                            const RttiType &type)
{
	// use the actual resolve method.
	std::vector<ResolutionResult> resolved = doc->resolve(path, type);
	// if we don't find anything, log an error
	if (resolved.size() == 0) {
		logger.error(std::string("Could not resolve ") + getPathString(path));
		return {nullptr};
	}
	// if we have more than one result, log an error.
	if (resolved.size() > 1) {
		logger.error(getPathString(path) + " was ambigous: ");
		for (auto &r : resolved) {
			logger.error(getPathString(r.node->path()));
		}
	}
	// Return the resulting node.
	return resolved[0].node.cast<Descriptor>();
}

/**
 * This builds the root StructuredEntity for the given document. It
 * automatically appends the newly build entity to the given document.
 *
 * @param document   is the document this entity shall be build for. The
 *                   resulting entity will automatically be appended to that
 *                   document. Also the manager of that document will be
 *                   used to register the new node.
 * @param logger     is the current logger.
 * @param path       is the name of the StructuredClass or a path specifying it
 *                   uniquely.
 * @param attributes are the attributes of the new node in terms of a Struct
 *                   variant (empty per default).
 * @param name       is the name of this StructuredEntity (empty per
 *                   default).
 * @return           the newly created StructuredEntity or a nullptr if some
 *                   input handle was empty or the given domains did not
 *                   contain a StructuredClass with the given name.
 */
Rooted<StructuredEntity> buildRootStructuredEntity(Handle<Document> document,
                                                   Logger &logger,
                                                   const Path &path,
                                                   Variant attributes = {},
                                                   std::string name = "")
{
	// If the parent is not set, we can not build the entity.
	if (document == nullptr) {
		logger.error("The input document handle was null!");
		return {nullptr};
	}
	// If we can not find the correct descriptor, we can not build the entity
	// either.
	Rooted<Descriptor> descriptor =
	    resolveDescriptor(document, logger, path, RttiTypes::StructuredClass);
	if (descriptor == nullptr) {
		return {nullptr};
	}
	// Then construct the StructuredEntity itself.
	Rooted<StructuredEntity> root{new StructuredEntity(
	    document->getManager(), document, descriptor.cast<StructuredClass>(),
	    attributes, std::move(name))};
	// append it to the document.
	document->setRoot(root);
	// and return it.
	return root;
}

/**
 * This builds a StructuredEntity as child of the given DocumentEntity. It
 * automatically appends the newly build entity to its parent.
 *
 * @param document   is the document this entity shall be build for. The domains
 *                   referenced here are the basis to resolve the given path.
 * @param logger     is the current logger.
 * @param parent     is the parent DocumentEntity. The newly constructed
 *                   StructuredEntity will automatically be appended to it.
 * @param path       is the name of the StructuredClass or a path specifying it
 *                   uniquely.
 * @param fieldName  is the name of the field where the newly constructed
 *                   StructuredEntity shall be appended.
 * @param attributes are the attributes of the new node in terms of a Struct
 *                   variant (empty per default).
 * @param name       is the name of this StructuredEntity (empty per
 *                   default).
 * @return           the newly created StructuredEntity or a nullptr if some
 *                   input handle was empty or the given domains did not
 *                   contain a StructuredClass with the given name.
 */
Rooted<StructuredEntity> buildStructuredEntity(
    Handle<Document> document, Logger &logger, Handle<StructuredEntity> parent,
    Path path, const std::string &fieldName = "", Variant attributes = {},
    std::string name = "")
{
	// If the input handles are not set, we can not build the entity.
	if (parent == nullptr) {
		logger.error("The input parent handle was null!");
		return {nullptr};
	}
	if (document == nullptr) {
		logger.error("The input document handle was null!");
		return {nullptr};
	}
	// If we can not find the correct descriptor, we can not build the entity
	// either.
	Rooted<Descriptor> descriptor =
	    resolveDescriptor(document, logger, path, RttiTypes::StructuredClass);
	if (descriptor == nullptr) {
		return {nullptr};
	}
	if(!descriptor->isa(RttiTypes::StructuredClass)){
		return {nullptr};
		logger.error("The descriptor was not an AnnotationClass!");
	}
	// Then construct the StructuredEntity itself.
	Rooted<StructuredEntity> entity{new StructuredEntity(
	    parent->getManager(), parent, descriptor.cast<StructuredClass>(),
	    attributes, std::move(name))};
	// if the field does not exist, return null handle as well.
	if (!parent->hasField(fieldName)) {
		logger.error(std::string("The parent has no field of the name ") +
		             fieldName + "!");
		return {nullptr};
	}
	parent->addStructuredEntity(entity, fieldName);

	// and return it.
	return entity;
}

/**
 * This builds a DocumentPrimitive as child of the given DocumentEntity. It
 * automatically appends the newly build entity to its parent.
 *
 * @param logger     is the current logger.
 * @param parent     is the parent DocumentEntity. The newly constructed
 *                   DocumentPrimitive will automatically be appended to it.
 * @param content    is the primitive content of the new node in terms of a
 *                   Struct variant.
 * @param fieldName  is the name of the field where the newly constructed
 *                   StructuredEntity shall be appended.
 *
 * @return           the newly created DocumentPrimitive or a nullptr if some
 *                   input handle was empty.
 */
Rooted<DocumentPrimitive> buildPrimitiveEntity(
    Logger &logger, Handle<StructuredEntity> parent, Variant content = {},
    const std::string &fieldName = "")
{
	// If the input handles are not set, we can not build the entity.
	if (parent == nullptr) {
		logger.error("The input parent handle was null!");
		return {nullptr};
	}
	// Then construct the StructuredEntity itself.
	Rooted<DocumentPrimitive> entity{
	    new DocumentPrimitive(parent->getManager(), parent, content)};
	// if the field does not exist, return null handle as well.
	if (!parent->hasField(fieldName)) {
		logger.error(std::string("The parent has no field of the name ") +
		             fieldName + "!");
		return {nullptr};
	}
	parent->addStructuredEntity(entity, fieldName);
	// and return it.
	return entity;
}

/**
 * This builds an Anchor as child of the given DocumentEntity. It
 * automatically appends the newly build Anchor to its parent.
 *
 * @param logger     is the current logger.
 * @param parent     is the parent DocumentEntity. The newly constructed
 *                   Anchor will automatically be appended to it.
 * @param id         is the id of this Anchor.
 * @param fieldName  is the name of the field where the newly constructed
 *                   Anchor shall be appended.
 *
 * @return           the newly created Anchor or a nullptr if some
 *                   input handle was empty.
 */
Rooted<AnnotationEntity::Anchor> buildAnchor(Logger &logger,
                                             Handle<StructuredEntity> parent,
                                             std::string id,
                                             const std::string &fieldName = "")
{
	// If the parent is not set, we can not build the anchor.
	if (parent == nullptr) {
		logger.error("The input parent handle was null!");
		return {nullptr};
	}
	// Then construct the Anchor itself
	Rooted<AnnotationEntity::Anchor> anchor{
	    new AnnotationEntity::Anchor(parent->getManager(), id, parent)};
	// append the new entity to the right field.
	if (!parent->hasField(fieldName)) {
		logger.error(std::string("The parent has no field of the name ") +
		             fieldName + "!");
		return {nullptr};
	}
	parent->addStructuredEntity(anchor, fieldName);
	// and return it.
	return anchor;
}
/**
 * This builds an AnnotationEntity as child of the given Document. It
 * automatically appends the newly build entity to its parent.
 *
 * @param document   is the document this entity shall be build for. The domains
 *                   referenced here are the basis to resolve the given path.
 * @param logger     is the current logger.
 * @param path       is the name of the AnnotationClass or a path specifying it
 *                   uniquely.
 * @param start      is the start Anchor for this AnnotationEntity.
 * @param end        is the end Anchor for this AnnotationEntity.
 * @param attributes are the attributes of the new node in terms of a Struct
 *                   variant (empty per default).
 * @param name       is the name of this AnnotationEntity (empty per
 *                   default).
 * @return           the newly created AnnotationEntity or a nullptr if some
 *                   input handle was empty or the given domains did not
 *                   contain a AnnotationClass with the given name.
 */
Rooted<AnnotationEntity> buildAnnotationEntity(
    Handle<Document> document, Logger &logger, const Path &path,
    Handle<AnnotationEntity::Anchor> start,
    Handle<AnnotationEntity::Anchor> end, Variant attributes = {},
    std::string name = "")
{
	// If the input handles are not set, we can not build the entity.
	if (document == nullptr) {
		logger.error("The input document handle was null!");
		return {nullptr};
	}
	// If we can not find the correct descriptor, we can not build the
	// entity either.
	Rooted<Descriptor> descriptor =
	    resolveDescriptor(document, logger, path, RttiTypes::AnnotationClass);
	if (descriptor == nullptr) {
		return {nullptr};
	}
	if(!descriptor->isa(RttiTypes::AnnotationClass)){
		return {nullptr};
		logger.error("The descriptor was not an AnnotationClass!");
	}
	// Then construct the AnnotationEntity itself
	Rooted<AnnotationEntity> anno{new AnnotationEntity(
	    document->getManager(), document, descriptor.cast<AnnotationClass>(),
	    start, end, attributes, name)};
	// append the new entity to the document
	document->addAnnotation(anno);
	// and return it.
	return anno;
}
}
}
#endif
