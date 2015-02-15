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

#include "DocumentHandler.hpp"

#include <algorithm>

#include <core/common/RttiBuilder.hpp>
#include <core/common/Utils.hpp>
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Typesystem.hpp>
#include <core/parser/ParserScope.hpp>

namespace ousia {

/* DocumentHandler */

void DocumentHandler::start(Variant::mapType &args)
{
	Rooted<Document> document =
	    project()->createDocument(args["name"].asString());
	document->setLocation(location());
	scope().push(document);
	scope().setFlag(ParserFlag::POST_HEAD, false);
}

void DocumentHandler::end() { scope().pop(); }

/* DocumentChildHandler */

void DocumentChildHandler::preamble(Handle<Node> parentNode,
                                    std::string &fieldName,
                                    DocumentEntity *&parent, bool &inField)
{
	// check if the parent in the structure tree was an explicit field
	// reference.
	inField = parentNode->isa(&RttiTypes::DocumentField);
	if (inField) {
		fieldName = parentNode->getName();
		parentNode = scope().selectOrThrow(
		    {&RttiTypes::StructuredEntity, &RttiTypes::AnnotationEntity});
	} else {
		// if it wasn't an explicit reference, we use the default field.
		fieldName = DEFAULT_FIELD_NAME;
	}
	// reference the parent entity explicitly.
	parent = nullptr;
	if (parentNode->isa(&RttiTypes::StructuredEntity)) {
		parent = static_cast<DocumentEntity *>(
		    parentNode.cast<StructuredEntity>().get());
	} else if (parentNode->isa(&RttiTypes::AnnotationEntity)) {
		parent = static_cast<DocumentEntity *>(
		    parentNode.cast<AnnotationEntity>().get());
	}
}

static void createPath(const std::string &firstFieldName,
                       const NodeVector<Node> &path, DocumentEntity *&parent)
{
	// add the first element
	parent = static_cast<DocumentEntity *>(
	    parent->createChildStructuredEntity(path[0].cast<StructuredClass>(),
	                                        Variant::mapType{}, firstFieldName,
	                                        "").get());

	size_t S = path.size();
	for (size_t p = 2; p < S; p = p + 2) {
		parent = static_cast<DocumentEntity *>(
		    parent->createChildStructuredEntity(
		                path[p].cast<StructuredClass>(), Variant::mapType{},
		                path[p - 1]->getName(), "").get());
	}
}

static void createPath(const NodeVector<Node> &path, DocumentEntity *&parent)
{
	size_t S = path.size();
	for (size_t p = 1; p < S; p = p + 2) {
		parent = static_cast<DocumentEntity *>(
		    parent->createChildStructuredEntity(
		                path[p].cast<StructuredClass>(), Variant::mapType{},
		                path[p - 1]->getName(), "").get());
	}
}

void DocumentChildHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);
	Rooted<Node> parentNode = scope().selectOrThrow(
	    {&RttiTypes::Document, &RttiTypes::StructuredEntity,
	     &RttiTypes::AnnotationEntity, &RttiTypes::DocumentField});

	std::string fieldName;
	DocumentEntity *parent;
	bool inField;

	preamble(parentNode, fieldName, parent, inField);

	// try to find a FieldDescriptor for the given tag if we are not in a
	// field already. This does _not_ try to construct transparent paths
	// in between.
	if (!inField && parent != nullptr &&
	    parent->getDescriptor()->hasField(name())) {
		Rooted<DocumentField> field{
		    new DocumentField(parentNode->getManager(), name(), parentNode)};
		field->setLocation(location());
		scope().push(field);
		return;
	}

	// Otherwise create a new StructuredEntity
	// TODO: Consider Anchors and AnnotationEntities
	Rooted<StructuredClass> strct =
	    scope().resolve<StructuredClass>(Utils::split(name(), ':'), logger());
	if (strct == nullptr) {
		// if we could not resolve the name, throw an exception.
		throw LoggableException(
		    std::string("\"") + name() + "\" could not be resolved.",
		    location());
	}

	std::string name;
	auto it = args.find("name");
	if (it != args.end()) {
		name = it->second.asString();
		args.erase(it);
	}

	Rooted<StructuredEntity> entity;
	if (parentNode->isa(&RttiTypes::Document)) {
		entity = parentNode.cast<Document>()->createRootStructuredEntity(
		    strct, args, name);
	} else {
		// calculate a path if transparent entities are needed in between.
		std::string lastFieldName = fieldName;
		if (inField) {
			Rooted<FieldDescriptor> field =
			    parent->getDescriptor()->getFieldDescriptor(fieldName);
			auto pathRes =
			    field.cast<FieldDescriptor>()->pathTo(strct, logger());
			if (!pathRes.second) {
				throw LoggableException(
				    std::string("An instance of \"") + strct->getName() +
				        "\" is not allowed as child of field \"" + fieldName +
				        "\"",
				    location());
			}
			if (!pathRes.first.empty()) {
				createPath(fieldName, pathRes.first, parent);
				lastFieldName = DEFAULT_FIELD_NAME;
			}
		} else {
			auto path = parent->getDescriptor()->pathTo(strct, logger());
			if (path.empty()) {
				throw LoggableException(
				    std::string("An instance of \"") + strct->getName() +
				        "\" is not allowed as child of an instance of \"" +
				        parent->getDescriptor()->getName() + "\"",
				    location());
			}

			// create all transparent entities until the last field.
			createPath(path, parent);
			if (path.size() > 1) {
				lastFieldName = DEFAULT_FIELD_NAME;
			}
		}
		// create the entity for the new element at last.
		entity = parent->createChildStructuredEntity(strct, args, lastFieldName,
		                                             name);
	}
	entity->setLocation(location());
	scope().push(entity);
}

void DocumentChildHandler::end() { scope().pop(); }

std::pair<bool, Variant> DocumentChildHandler::convertData(
    Handle<FieldDescriptor> field, Logger &logger, const std::string &data)
{
	// if the content is supposed to be of type string, we can finish
	// directly.
	auto vts = field->getPrimitiveType()->getVariantTypes();
	if (std::find(vts.begin(), vts.end(), VariantType::STRING) != vts.end()) {
		return std::make_pair(true, Variant::fromString(data));
	}

	// then try to parse the content using the type specification.
	auto res = field->getPrimitiveType()->read(
	    data, logger, location().getSourceId(), location().getStart());
	return res;
}

void DocumentChildHandler::data(const std::string &data, int fieldIdx)
{
	Rooted<Node> parentNode = scope().selectOrThrow(
	    {&RttiTypes::StructuredEntity, &RttiTypes::AnnotationEntity,
	     &RttiTypes::DocumentField});

	std::string fieldName;
	DocumentEntity *strctParent;
	bool inField;

	preamble(parentNode, fieldName, strctParent, inField);

	Rooted<Descriptor> desc = strctParent->getDescriptor();
	// The parent from which we need to connect to the primitive content.
	Rooted<Node> parentClass;
	/*
	 * We distinguish two cases here: One for fields that are given.
	 */
	if (inField) {
		// retrieve the actual FieldDescriptor
		Rooted<FieldDescriptor> field = desc->getFieldDescriptor(fieldName);
		if (field == nullptr) {
			logger().error(
			    std::string("Can't handle data because no field with name \"") +
			        fieldName + "\" exists in descriptor\"" + desc->getName() +
			        "\".",
			    location());
			return;
		}
		// if it is a primitive field directly, try to parse the content.
		if (field->isPrimitive()) {
			auto res = convertData(field, logger(), data);
			// add it as primitive content.
			if (res.first) {
				strctParent->createChildDocumentPrimitive(res.second,
				                                          fieldName);
			}
			return;
		}
		// if it is not primitive we need to connect via transparent elements
		// and default fields.
		parentClass = field;
	} else {
		// in case of default fields we need to construct via default fields
		// and maybe transparent elements.
		parentClass = desc;
	}
	/*
	 * Search through all permitted default fields of the parent class that
	 * allow primitive content at this point and could be constructed via
	 * transparent intermediate entities.
	 * We then try to parse the data using the type specified by the respective
	 * field. If that does not work we proceed to the next possible field.
	 */
	// retrieve all default fields at this point.
	NodeVector<FieldDescriptor> defaultFields;
	if (inField) {
		defaultFields = parentClass.cast<FieldDescriptor>()->getDefaultFields();
	} else {
		defaultFields = parentClass.cast<StructuredClass>()->getDefaultFields();
	}
	std::vector<LoggerFork> forks;
	for (auto field : defaultFields) {
		// then try to parse the content using the type specification.
		forks.emplace_back(logger().fork());
		auto res = convertData(field, forks.back(), data);
		if (res.first) {
			forks.back().commit();
			// if that worked, construct the necessary path.
			if (inField) {
				NodeVector<Node> path =
				    parentClass.cast<FieldDescriptor>()->pathTo(field,
				                                                logger());
				createPath(fieldName, path, strctParent);
			} else {
				auto pathRes = desc->pathTo(field, logger());
				assert(pathRes.second);
				createPath(pathRes.first, strctParent);
			}
			// then create the primitive element.
			strctParent->createChildDocumentPrimitive(res.second);
			return;
		}
	}
	logger().error("Could not read data with any of the possible fields:");
	size_t f = 0;
	for (auto field : defaultFields) {
		logger().note(Utils::join(field->path(), ".") + ":", SourceLocation{},
		              MessageMode::NO_CONTEXT);
		forks[f].commit();
		f++;
	}
}

namespace RttiTypes {
const Rtti DocumentField =
    RttiBuilder<ousia::DocumentField>("DocumentField").parent(&Node);
}
}