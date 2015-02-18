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
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Project.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

#include "DocumentHandler.hpp"
#include "DomainHandler.hpp"
#include "State.hpp"
#include "TypesystemHandler.hpp"

namespace ousia {
namespace parser_stack {

/* DomainHandler */

bool DomainHandler::start(Variant::mapType &args)
{
	// Create the Domain node
	Rooted<Domain> domain =
	    context().getProject()->createDomain(args["name"].asString());
	domain->setLocation(location());

	// If the domain is defined inside a document, add the reference to the
	// document
	Rooted<Document> document = scope().select<Document>();
	if (document != nullptr) {
		document->reference(domain);
	}

	// Push the typesystem onto the scope, set the POST_HEAD flag to true
	scope().push(domain);
	scope().setFlag(ParserFlag::POST_HEAD, false);
	return true;
}

void DomainHandler::end() { scope().pop(logger()); }

/* DomainStructHandler */

bool DomainStructHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	Rooted<Domain> domain = scope().selectOrThrow<Domain>();

	Rooted<StructuredClass> structuredClass = domain->createStructuredClass(
	    args["name"].asString(), args["cardinality"].asCardinality(), nullptr,
	    args["transparent"].asBool(), args["isRoot"].asBool());
	structuredClass->setLocation(location());

	const std::string &isa = args["isa"].asString();
	if (!isa.empty()) {
		scope().resolve<StructuredClass>(
		    isa, structuredClass, logger(),
		    [](Handle<Node> superclass, Handle<Node> structuredClass,
		       Logger &logger) {
			    if (superclass != nullptr) {
				    structuredClass.cast<StructuredClass>()->setSuperclass(
				        superclass.cast<StructuredClass>(), logger);
			    }
			});
	}

	scope().push(structuredClass);
	return true;
}

void DomainStructHandler::end() { scope().pop(logger()); }

/* DomainAnnotationHandler */
bool DomainAnnotationHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	Rooted<Domain> domain = scope().selectOrThrow<Domain>();

	Rooted<AnnotationClass> annotationClass =
	    domain->createAnnotationClass(args["name"].asString());
	annotationClass->setLocation(location());

	scope().push(annotationClass);
	return true;
}

void DomainAnnotationHandler::end() { scope().pop(logger()); }

/* DomainAttributesHandler */

bool DomainAttributesHandler::start(Variant::mapType &args)
{
	// Fetch the current typesystem and create the struct node
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	Rooted<StructType> attrDesc = parent->getAttributesDescriptor();
	attrDesc->setLocation(location());

	scope().push(attrDesc);
	return true;
}

void DomainAttributesHandler::end() { scope().pop(logger()); }

/* DomainFieldHandler */

bool DomainFieldHandler::start(Variant::mapType &args)
{
	FieldDescriptor::FieldType type;
	if (args["isSubtree"].asBool()) {
		type = FieldDescriptor::FieldType::SUBTREE;
	} else {
		type = FieldDescriptor::FieldType::TREE;
	}

	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	auto res = parent->createFieldDescriptor(
	    logger(), type, args["name"].asString(), args["optional"].asBool());
	res.first->setLocation(location());
	if (res.second) {
		logger().warning(
		    std::string("Field \"") + res.first->getName() +
		        "\" was declared after main field. The order of fields "
		        "was changed to make the main field the last field.",
		    *res.first);
	}

	scope().push(res.first);
	return true;
}

void DomainFieldHandler::end() { scope().pop(logger()); }

/* DomainFieldRefHandler */

bool DomainFieldRefHandler::start(Variant::mapType &args)
{
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	const std::string &name = args["ref"].asString();

	auto loc = location();

	scope().resolveFieldDescriptor(name, parent, logger(),
	                               [loc](Handle<Node> field,
	                                     Handle<Node> parent, Logger &logger) {
		if (field != nullptr) {
			if (parent.cast<StructuredClass>()->addFieldDescriptor(
			        field.cast<FieldDescriptor>(), logger)) {
				logger.warning(
				    std::string("Field \"") + field->getName() +
				        "\" was referenced after main field was declared. The "
				        "order of fields was changed to make the main field "
				        "the last field.",
				    loc);
			}
		}
	});
	return true;
}

void DomainFieldRefHandler::end() {}

/* DomainPrimitiveHandler */

bool DomainPrimitiveHandler::start(Variant::mapType &args)
{
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	FieldDescriptor::FieldType fieldType;
	if (args["isSubtree"].asBool()) {
		fieldType = FieldDescriptor::FieldType::SUBTREE;
	} else {
		fieldType = FieldDescriptor::FieldType::TREE;
	}

	auto res = parent->createPrimitiveFieldDescriptor(
	    new UnknownType(manager()), logger(), fieldType,
	    args["name"].asString(), args["optional"].asBool());
	res.first->setLocation(location());
	if (res.second) {
		logger().warning(
		    std::string("Field \"") + res.first->getName() +
		        "\" was declared after main field. The order of fields "
		        "was changed to make the main field the last field.",
		    *res.first);
	}

	const std::string &type = args["type"].asString();
	scope().resolve<Type>(type, res.first, logger(),
	                      [](Handle<Node> type, Handle<Node> field,
	                         Logger &logger) {
		if (type != nullptr) {
			field.cast<FieldDescriptor>()->setPrimitiveType(type.cast<Type>());
		}
	});

	scope().push(res.first);
	return true;
}

void DomainPrimitiveHandler::end() { scope().pop(logger()); }

/* DomainChildHandler */

bool DomainChildHandler::start(Variant::mapType &args)
{
	Rooted<FieldDescriptor> field = scope().selectOrThrow<FieldDescriptor>();

	const std::string &ref = args["ref"].asString();
	scope().resolve<StructuredClass>(
	    ref, field, logger(),
	    [](Handle<Node> child, Handle<Node> field, Logger &logger) {
		    if (child != nullptr) {
			    field.cast<FieldDescriptor>()->addChild(
			        child.cast<StructuredClass>());
		    }
		});
	return true;
}

/* DomainParentHandler */

bool DomainParentHandler::start(Variant::mapType &args)
{
	Rooted<StructuredClass> strct = scope().selectOrThrow<StructuredClass>();

	Rooted<DomainParent> parent{
	    new DomainParent(strct->getManager(), args["ref"].asString(), strct)};
	parent->setLocation(location());
	scope().push(parent);
	return true;
}

void DomainParentHandler::end() { scope().pop(logger()); }

/* DomainParentFieldHandler */

bool DomainParentFieldHandler::start(Variant::mapType &args)
{
	Rooted<DomainParent> parentNameNode = scope().selectOrThrow<DomainParent>();
	FieldDescriptor::FieldType type;
	if (args["isSubtree"].asBool()) {
		type = FieldDescriptor::FieldType::SUBTREE;
	} else {
		type = FieldDescriptor::FieldType::TREE;
	}

	const std::string &name = args["name"].asString();
	const bool optional = args["optional"].asBool();
	Rooted<StructuredClass> strct =
	    parentNameNode->getParent().cast<StructuredClass>();

	// resolve the parent, create the declared field and add the declared
	// StructuredClass as child to it.
	scope().resolve<Descriptor>(
	    parentNameNode->getName(), strct, logger(),
	    [type, name, optional](Handle<Node> parent, Handle<Node> strct,
	                           Logger &logger) {
		    if (parent != nullptr) {
			    Rooted<FieldDescriptor> field =
			        (parent.cast<Descriptor>()->createFieldDescriptor(
			             logger, type, name, optional)).first;
			    field->addChild(strct.cast<StructuredClass>());
		    }
		});
	return true;
}

/* DomainParentFieldRefHandler */

bool DomainParentFieldRefHandler::start(Variant::mapType &args)
{
	Rooted<DomainParent> parentNameNode = scope().selectOrThrow<DomainParent>();

	const std::string &name = args["ref"].asString();
	Rooted<StructuredClass> strct =
	    parentNameNode->getParent().cast<StructuredClass>();
	auto loc = location();

	// resolve the parent, get the referenced field and add the declared
	// StructuredClass as child to it.
	scope().resolve<Descriptor>(
	    parentNameNode->getName(), strct, logger(),
	    [name, loc](Handle<Node> parent, Handle<Node> strct, Logger &logger) {
		    if (parent != nullptr) {
			    Rooted<FieldDescriptor> field =
			        parent.cast<Descriptor>()->getFieldDescriptor(name);
			    if (field == nullptr) {
				    logger.error(
				        std::string("Could not find referenced field ") + name,
				        loc);
				    return;
			    }
			    field->addChild(strct.cast<StructuredClass>());
		    }
		});
	return true;
}

namespace States {
const State Domain = StateBuilder()
                         .parents({&None, &Document})
                         .createdNodeType(&RttiTypes::Domain)
                         .elementHandler(DomainHandler::create)
                         .arguments({Argument::String("name")});

const State DomainStruct =
    StateBuilder()
        .parent(&Domain)
        .createdNodeType(&RttiTypes::StructuredClass)
        .elementHandler(DomainStructHandler::create)
        .arguments({Argument::String("name"),
                    Argument::Cardinality("cardinality", Cardinality::any()),
                    Argument::Bool("isRoot", false),
                    Argument::Bool("transparent", false),
                    Argument::String("isa", "")});

const State DomainAnnotation =
    StateBuilder()
        .parent(&Domain)
        .createdNodeType(&RttiTypes::AnnotationClass)
        .elementHandler(DomainAnnotationHandler::create)
        .arguments({Argument::String("name")});

const State DomainAttributes =
    StateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::StructType)
        .elementHandler(DomainAttributesHandler::create)
        .arguments({});

const State DomainAttribute =
    StateBuilder()
        .parent(&DomainAttributes)
        .elementHandler(TypesystemStructFieldHandler::create)
        .arguments({Argument::String("name"), Argument::String("type"),
                    Argument::Any("default", Variant::fromObject(nullptr))});

const State DomainField = StateBuilder()
                              .parents({&DomainStruct, &DomainAnnotation})
                              .createdNodeType(&RttiTypes::FieldDescriptor)
                              .elementHandler(DomainFieldHandler::create)
                              .arguments({Argument::String("name", ""),
                                          Argument::Bool("isSubtree", false),
                                          Argument::Bool("optional", false)});

const State DomainFieldRef =
    StateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainFieldRefHandler::create)
        .arguments({Argument::String("ref", DEFAULT_FIELD_NAME)});

const State DomainStructPrimitive =
    StateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainPrimitiveHandler::create)
        .arguments(
            {Argument::String("name", ""), Argument::Bool("isSubtree", false),
             Argument::Bool("optional", false), Argument::String("type")});

const State DomainStructChild = StateBuilder()
                                    .parent(&DomainField)
                                    .elementHandler(DomainChildHandler::create)
                                    .arguments({Argument::String("ref")});

const State DomainStructParent =
    StateBuilder()
        .parent(&DomainStruct)
        .createdNodeType(&RttiTypes::DomainParent)
        .elementHandler(DomainParentHandler::create)
        .arguments({Argument::String("ref")});

const State DomainStructParentField =
    StateBuilder()
        .parent(&DomainStructParent)
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainParentFieldHandler::create)
        .arguments({Argument::String("name", ""),
                    Argument::Bool("isSubtree", false),
                    Argument::Bool("optional", false)});

const State DomainStructParentFieldRef =
    StateBuilder()
        .parent(&DomainStructParent)
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainParentFieldRefHandler::create)
        .arguments({Argument::String("ref", DEFAULT_FIELD_NAME)});
}
}

namespace RttiTypes {
const Rtti DomainParent = RttiBuilder<ousia::parser_stack::DomainParent>(
                              "DomainParent").parent(&Node);
}
}
