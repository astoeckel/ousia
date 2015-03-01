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

#include <algorithm>

#include <core/common/RttiBuilder.hpp>
#include <core/common/Utils.hpp>
#include <core/common/VariantReader.hpp>
#include <core/model/Document.hpp>
#include <core/model/Ontology.hpp>
#include <core/model/Project.hpp>
#include <core/model/Typesystem.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

#include "DocumentHandler.hpp"
#include "State.hpp"

namespace ousia {
namespace parser_stack {

/* DocumentHandler */

bool DocumentHandler::start(Variant::mapType &args)
{
	Rooted<Document> document =
	    context().getProject()->createDocument(args["name"].asString());
	document->setLocation(location());
	scope().push(document);
	scope().setFlag(ParserFlag::POST_HEAD, false);

	return true;
}

void DocumentHandler::end() { scope().pop(logger()); }

/* DocumentChildHandler */

void DocumentChildHandler::preamble(Rooted<Node> &parentNode, size_t &fieldIdx,
                                    DocumentEntity *&parent)
{
	// Check if the parent in the structure tree was an explicit field
	// reference.
	if (parentNode->isa(&RttiTypes::DocumentField)) {
		fieldIdx = parentNode.cast<DocumentField>()->fieldIdx;
		parentNode = scope().selectOrThrow(
		    {&RttiTypes::StructuredEntity, &RttiTypes::AnnotationEntity});
	}

	// Reference the parent entity explicitly.
	parent = nullptr;
	if (parentNode->isa(&RttiTypes::StructuredEntity)) {
		parent = static_cast<DocumentEntity *>(
		    parentNode.cast<StructuredEntity>().get());
	} else if (parentNode->isa(&RttiTypes::AnnotationEntity)) {
		parent = static_cast<DocumentEntity *>(
		    parentNode.cast<AnnotationEntity>().get());
	}
}

void DocumentChildHandler::createPath(const NodeVector<Node> &path,
                                      DocumentEntity *&parent, size_t p0)
{
	size_t S = path.size();
	for (size_t p = p0; p < S; p = p + 2) {
		// add the field.
		Rooted<DocumentField> field{new DocumentField(
		    manager(), scope().getLeaf(),
		    parent->getDescriptor()->getFieldDescriptorIndex(), true)};
		scope().push(field);
		// add the transparent/implicit structure element.
		Rooted<StructuredEntity> transparent =
		    parent->createChildStructuredEntity(path[p].cast<StructuredClass>(),
		                                        Variant::mapType{},
		                                        path[p - 1]->getName(), "");
		transparent->setLocation(location());
		transparent->setTransparent(true);
		scope().push(transparent);
		parent = static_cast<DocumentEntity *>(transparent.get());
	}
	// add the last field.
	Rooted<DocumentField> field{new DocumentField(
	    manager(), scope().getLeaf(),
	    parent->getDescriptor()->getFieldDescriptorIndex(), true)};
	scope().push(field);

	// Generally allow explicit fields in the new field
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, false);
}

void DocumentChildHandler::createPath(const size_t &firstFieldIdx,
                                      const NodeVector<Node> &path,
                                      DocumentEntity *&parent)
{
	// Add the first element
	Rooted<StructuredEntity> transparent = parent->createChildStructuredEntity(
	    path[0].cast<StructuredClass>(), firstFieldIdx);
	transparent->setLocation(location());
	transparent->setTransparent(true);
	scope().push(transparent);
	parent = static_cast<DocumentEntity *>(transparent.get());

	createPath(path, parent, 2);

	// Generally allow explicit fields in the new field
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, false);
}

bool DocumentChildHandler::start(Variant::mapType &args)
{
	// extract the special "name" attribute from the input arguments.
	// the remaining attributes will be forwarded to the newly constructed
	// element.
	std::string nameAttr;
	{
		auto it = args.find("name");
		if (it != args.end()) {
			nameAttr = it->second.asString();
			args.erase(it);
		}
	}

	scope().setFlag(ParserFlag::POST_HEAD, true);
	while (true) {
		Rooted<Node> parentNode = scope().getLeaf();

		Rooted<StructuredEntity> entity;
		// handle the root note specifically.
		if (parentNode->isa(&RttiTypes::Document)) {
			// if we already have a root node, stop.
			if (parentNode.cast<Document>()->getRoot() != nullptr) {
				logger().warning(
				    "This document already has a root node. The additional "
				    "node is ignored.",
				    location());
				return false;
			}
			Rooted<StructuredClass> strct = scope().resolve<StructuredClass>(
			    Utils::split(name(), ':'), logger());
			if (strct == nullptr) {
				// if we could not resolve the name, throw an exception.
				throw LoggableException(
				    std::string("\"") + name() + "\" could not be resolved.",
				    location());
			}
			entity = parentNode.cast<Document>()->createRootStructuredEntity(
			    strct, args, nameAttr);
		} else {
			assert(parentNode->isa(&RttiTypes::DocumentField));

			size_t fieldIdx;
			DocumentEntity *parent;

			preamble(parentNode, fieldIdx, parent);

			// TODO: REMOVE
			std::string thisName = name();
			std::string parentClassName;
			if (parent != nullptr) {
				parentClassName = parent->getDescriptor()->getName();
			}

			/*
			 * Try to find a FieldDescriptor for the given tag if we are not in
			 * a field already. This does _not_ try to construct transparent
			 * paths in between.
			 */
			{
				ssize_t newFieldIdx =
				    parent->getDescriptor()->getFieldDescriptorIndex(name());
				if (newFieldIdx != -1) {
					// Check whether explicit fields are allowed here, if not
					if (scope().getFlag(ParserFlag::POST_EXPLICIT_FIELDS)) {
						logger().note(
						    std::string(
						        "Data or structure commands have already been "
						        "given, command \"") +
						        name() + std::string(
						                     "\" is not interpreted explicit "
						                     "field. Move explicit field "
						                     "references to the beginning."),
						    location());
					} else {
						Rooted<DocumentField> field{new DocumentField(
						    manager(), parentNode, newFieldIdx, false)};
						field->setLocation(location());
						scope().push(field);
						isExplicitField = true;
						return true;
					}
				}
			}

			// Otherwise create a new StructuredEntity
			// TODO: Consider Anchors and AnnotationEntities
			Rooted<StructuredClass> strct = scope().resolve<StructuredClass>(
			    Utils::split(name(), ':'), logger());
			if (strct == nullptr) {
				// if we could not resolve the name, throw an exception.
				throw LoggableException(
				    std::string("\"") + name() + "\" could not be resolved.",
				    location());
			}

			// calculate a path if transparent entities are needed in between.
			Rooted<FieldDescriptor> field =
			    parent->getDescriptor()->getFieldDescriptors()[fieldIdx];
			size_t lastFieldIdx = fieldIdx;
			auto pathRes = field->pathTo(strct, logger());
			if (!pathRes.second) {
				if (scope().getLeaf().cast<DocumentField>()->transparent) {
					// if we have transparent elements above us in the structure
					// tree we try to unwind them before we give up.
					// pop the implicit field.
					scope().pop(logger());
					// pop the implicit element.
					scope().pop(logger());
					continue;
				}
				throw LoggableException(
				    std::string("An instance of \"") + strct->getName() +
				        "\" is not allowed as child of field \"" +
				        field->getName() + "\" of descriptor \"" +
				        parent->getDescriptor()->getName() + "\"",
				    location());
			}
			if (!pathRes.first.empty()) {
				createPath(lastFieldIdx, pathRes.first, parent);
				lastFieldIdx =
				    parent->getDescriptor()->getFieldDescriptorIndex();
			}
			// create the entity for the new element at last.
			// TODO: REMOVE
			strct_name = strct->getName();
			entity = parent->createChildStructuredEntity(strct, lastFieldIdx,
			                                             args, nameAttr);
		}

		// We're past the region in which explicit fields can be defined in the
		// parent structure element
		scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, true);

		// Bush the entity onto the stack
		entity->setLocation(location());
		scope().push(entity);
		return true;
	}
}

void DocumentChildHandler::end()
{
	// in case of explicit fields we do not want to pop something from the
	// stack.
	if (isExplicitField) {
		return;
	}
	// pop the "main" element.
	scope().pop(logger());
}

bool DocumentChildHandler::fieldStart(bool &isDefault, size_t fieldIdx)
{
	if (isExplicitField) {
		// In case of explicit fields we do not want to create another field.
		isDefault = true;
		return fieldIdx == 0;
	}
	Rooted<Node> parentNode = scope().getLeaf();
	assert(parentNode->isa(&RttiTypes::StructuredEntity) ||
	       parentNode->isa(&RttiTypes::AnnotationEntity));
	size_t dummy;
	DocumentEntity *parent;

	preamble(parentNode, dummy, parent);

	NodeVector<FieldDescriptor> fields =
	    parent->getDescriptor()->getFieldDescriptors();

	if (isDefault) {
		if(fields.empty()){
			return false;
		}
		fieldIdx = fields.size() - 1;
	} else {
		if (fieldIdx >= fields.size()) {
			return false;
		}
		isDefault = fieldIdx == fields.size() - 1;
	}
	// push the field on the stack.
	Rooted<DocumentField> field{
	    new DocumentField(manager(), parentNode, fieldIdx, false)};
	field->setLocation(location());
	scope().push(field);

	// Generally allow explicit fields in the new field
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, false);

	return true;
}

void DocumentChildHandler::fieldEnd()
{
	assert(scope().getLeaf()->isa(&RttiTypes::DocumentField));

	// pop the field from the stack.
	scope().pop(logger());

	// pop all remaining transparent elements.
	while (scope().getLeaf()->isa(&RttiTypes::StructuredEntity) &&
	       scope().getLeaf().cast<StructuredEntity>()->isTransparent()) {
		// pop the transparent element.
		scope().pop(logger());
		// pop the transparent field.
		scope().pop(logger());
	}
}

bool DocumentChildHandler::annotationStart(const Variant &className,
                                           Variant::mapType &args)
{
	// TODO: Implement
	return false;
}

bool DocumentChildHandler::annotationEnd(const Variant &className,
                                         const Variant &elementName)
{
	// TODO: Implement
	return false;
}

bool DocumentChildHandler::convertData(Handle<FieldDescriptor> field,
                                       Variant &data, Logger &logger)
{
	bool valid = true;
	Rooted<Type> type = field->getPrimitiveType();

	// If the content is supposed to be of type string, we only need to check
	// for "magic" values -- otherwise just call the "parseGenericString"
	// function on the string data
	if (type->isa(&RttiTypes::StringType)) {
		const std::string &str = data.asString();
		// TODO: Referencing constants with "." separator should also work
		if (Utils::isIdentifier(str)) {
			data.markAsMagic();
		}
	} else {
		// Parse the string as generic string, assign the result
		auto res = VariantReader::parseGenericString(
		    data.asString(), logger, data.getLocation().getSourceId(),
		    data.getLocation().getStart());
		data = res.second;
	}

	// Now try to resolve the value for the primitive type
	return valid && scope().resolveValue(data, type, logger);
}

bool DocumentChildHandler::data(Variant &data)
{
	// We're past the region in which explicit fields can be defined in the
	// parent structure element
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, true);

	Rooted<Node> parentField = scope().getLeaf();
	assert(parentField->isa(&RttiTypes::DocumentField));

	size_t fieldIdx;
	DocumentEntity *parent;

	preamble(parentField, fieldIdx, parent);

	Rooted<Descriptor> desc = parent->getDescriptor();

	// Retrieve the actual FieldDescriptor
	Rooted<FieldDescriptor> field = desc->getFieldDescriptors()[fieldIdx];
	// If it is a primitive field directly, try to parse the content.
	if (field->isPrimitive()) {
		// Add it as primitive content.
		if (!convertData(field, data, logger())) {
			return false;
		}

		parent->createChildDocumentPrimitive(data, fieldIdx);
		return true;
	}

	// Search through all permitted default fields of the parent class that
	// allow primitive content at this point and could be constructed via
	// transparent intermediate entities.
	NodeVector<FieldDescriptor> defaultFields = field->getDefaultFields();
	// Try to parse the data using the type specified by the respective field.
	// If that does not work we proceed to the next possible field.
	std::vector<LoggerFork> forks;
	for (auto primitiveField : defaultFields) {
		// Then try to parse the content using the type specification.
		forks.emplace_back(logger().fork());
		if (!convertData(primitiveField, data, forks.back())) {
			continue;
		}

		// The conversion worked, commit any possible warnings
		forks.back().commit();

		// Construct the necessary path
		NodeVector<Node> path = field->pathTo(primitiveField, logger());
		// TODO: Create methods with indices instead of names.
		createPath(fieldIdx, path, parent);

		// Then create the primitive element
		parent->createChildDocumentPrimitive(data);
		return true;
	}

	// No field was found that might take the data -- dump the error messages
	// from the loggers -- or, if there were no primitive fields, clearly state
	// this fact
	if (defaultFields.empty()) {
		logger().error("Got data, but structure \"" + name() +
		                   "\" does not have any primitive field",
		               data);
	} else {
		logger().error("Could not read data with any of the possible fields:",
		               data);
		size_t f = 0;
		for (auto field : defaultFields) {
			logger().note(std::string("Field ") +
			                  Utils::join(field->path(), ".") +
			                  std::string(":"),
			              SourceLocation{}, MessageMode::NO_CONTEXT);
			forks[f].commit();
			f++;
		}
	}
	return false;
}

namespace States {
const State Document = StateBuilder()
                           .parent(&None)
                           .createdNodeType(&RttiTypes::Document)
                           .elementHandler(DocumentHandler::create)
                           .arguments({Argument::String("name", "")});

const State DocumentChild = StateBuilder()
                                .parents({&Document, &DocumentChild})
                                .createdNodeTypes({&RttiTypes::StructureNode,
                                                   &RttiTypes::AnnotationEntity,
                                                   &RttiTypes::DocumentField})
                                .elementHandler(DocumentChildHandler::create);
}
}

namespace RttiTypes {
const Rtti DocumentField = RttiBuilder<ousia::parser_stack::DocumentField>(
                               "DocumentField").parent(&Node);
}
}