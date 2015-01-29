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
                                            const Rtti &type)
{
	// use the actual resolve method.
	std::vector<ResolutionResult> resolved = doc->resolve(type, path);
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
	    attributes, fieldName, std::move(name))};

	// and return it.
	return entity;
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
    Handle<Anchor> start,
    Handle<Anchor> end, Variant attributes = {},
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
	// and return it.
	return anno;
}
}
#endif
