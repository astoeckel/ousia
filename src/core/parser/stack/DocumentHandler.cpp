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
#include <core/common/VariantReader.hpp>
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Project.hpp>
#include <core/model/Typesystem.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

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

void DocumentHandler::end() { scope().pop(); }

/* DocumentChildHandler */

void DocumentChildHandler::preamble(Handle<Node> parentNode,
                                    std::string &fieldName,
                                    DocumentEntity *&parent, bool &inField)
{
	// Check if the parent in the structure tree was an explicit field
	// reference.
	inField = parentNode->isa(&RttiTypes::DocumentField);
	if (inField) {
		fieldName = parentNode->getName();
		parentNode = scope().selectOrThrow(
		    {&RttiTypes::StructuredEntity, &RttiTypes::AnnotationEntity});
	} else {
		// If it wasn't an explicit reference, we use the default field.
		fieldName = DEFAULT_FIELD_NAME;
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
                                      DocumentEntity *&parent)
{
	// TODO (@benjamin): These should be pushed onto the scope and poped once
	// the scope is left. Otherwise stuff may not be correclty resolved.
	size_t S = path.size();
	for (size_t p = 1; p < S; p = p + 2) {
		parent = static_cast<DocumentEntity *>(
		    parent->createChildStructuredEntity(
		                path[p].cast<StructuredClass>(), Variant::mapType{},
		                path[p - 1]->getName(), "").get());
	}
}

bool DocumentChildHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);
	Rooted<Node> parentNode = scope().selectOrThrow(
	    {&RttiTypes::Document, &RttiTypes::StructuredEntity,
	     &RttiTypes::AnnotationEntity, &RttiTypes::DocumentField});

	std::string fieldName;
	DocumentEntity *parent;
	bool inField;

	preamble(parentNode, fieldName, parent, inField);

	// Try to find a FieldDescriptor for the given tag if we are not in a
	// field already. This does _not_ try to construct transparent paths
	// in between.
	if (!inField && parent != nullptr &&
	    parent->getDescriptor()->hasField(name())) {
		Rooted<DocumentField> field{
		    new DocumentField(parentNode->getManager(), fieldName, parentNode)};
		field->setLocation(location());
		scope().push(field);
		return true;
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
		entity =
		    parent->createChildStructuredEntity(strct, args, fieldName, name);
	}
	entity->setLocation(location());
	scope().push(entity);
	return true;
}

void DocumentChildHandler::end() { scope().pop(); }

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
	Rooted<Node> parentNode = scope().selectOrThrow(
	    {&RttiTypes::StructuredEntity, &RttiTypes::AnnotationEntity,
	     &RttiTypes::DocumentField});

	std::string fieldName;
	DocumentEntity *parent;
	bool inField;

	preamble(parentNode, fieldName, parent, inField);

	Rooted<Descriptor> desc = parent->getDescriptor();

	// We distinguish two cases here: One for fields that are given.
	if (fieldName != DEFAULT_FIELD_NAME) {
		// Retrieve the actual FieldDescriptor
		Rooted<FieldDescriptor> field = desc->getFieldDescriptor(fieldName);
		if (field == nullptr) {
			logger().error(
			    std::string("Can't handle data because no field with name \"") +
			        fieldName + "\" exists in descriptor\"" + desc->getName() +
			        "\".",
			    location());
			return false;
		}
		// If it is not primitive at all, we can't parse the content.
		if (!field->isPrimitive()) {
			logger().error(std::string("Can't handle data because field \"") +
			                   fieldName + "\" of descriptor \"" +
			                   desc->getName() + "\" is not primitive!",
			               location());
			return false;
		}

		// Try to convert the data variable to the correct format, abort if this
		// does not work
		if (!convertData(field, data, logger())) {
			return false;
		}

		// Add it as primitive content
		parent->createChildDocumentPrimitive(data, fieldName);
		return true;
	} else {
		// The second case is for primitive fields. Here we search through
		// all FieldDescriptors that allow primitive content at this point
		// and could be constructed via transparent intermediate entities.
		// We then try to parse the data using the type specified by the
		// respective field. If that does not work we proceed to the next
		// possible field.
		NodeVector<FieldDescriptor> fields = desc->getDefaultFields();
		std::vector<LoggerFork> forks;
		for (auto field : fields) {
			// Then try to parse the content using the type specification
			forks.emplace_back(logger().fork());

			// Try to convert the data variable to the correct format, abort if
			// this does not work
			if (!convertData(field, data, forks.back())) {
				return false;
			}

			// Show possible warnings that were emitted by this type conversion
			forks.back().commit();

			// If that worked, construct the necessary path
			auto pathRes = desc->pathTo(field, logger());
			assert(pathRes.second);
			NodeVector<Node> path = pathRes.first;
			createPath(path, parent);

			// Then create the primitive element
			parent->createChildDocumentPrimitive(data, fieldName);
			return true;
		}
		logger().error("Could not read data with any of the possible fields:");
		for (size_t f = 0; f < fields.size(); f++) {
			logger().note(Utils::join(fields[f]->path(), ".") + ":",
			              SourceLocation{}, MessageMode::NO_CONTEXT);
			forks[f].commit();
		}
		return false;
	}
	return true;
}
}

namespace RttiTypes {
const Rtti DocumentField = RttiBuilder<ousia::parser_stack::DocumentField>(
                               "DocumentField").parent(&Node);
}
}
