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

#include "DomainHandler.hpp"

#include <core/common/RttiBuilder.hpp>
#include <core/model/Domain.hpp>
#include <core/parser/ParserScope.hpp>

namespace ousia {

/* DomainHandler */

void DomainHandler::start(Variant::mapType &args)
{
	Rooted<Domain> domain = project()->createDomain(args["name"].asString());
	domain->setLocation(location());

	scope().push(domain);
}

void DomainHandler::end() { scope().pop(); }

/* DomainStructHandler */

void DomainStructHandler::start(Variant::mapType &args)
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
}

void DomainStructHandler::end() { scope().pop(); }

/* DomainAnnotationHandler */
void DomainAnnotationHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	Rooted<Domain> domain = scope().selectOrThrow<Domain>();

	Rooted<AnnotationClass> annotationClass =
	    domain->createAnnotationClass(args["name"].asString());
	annotationClass->setLocation(location());

	scope().push(annotationClass);
}

void DomainAnnotationHandler::end() { scope().pop(); }

/* DomainAttributesHandler */

void DomainAttributesHandler::start(Variant::mapType &args)
{
	// Fetch the current typesystem and create the struct node
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	Rooted<StructType> attrDesc = parent->getAttributesDescriptor();
	attrDesc->setLocation(location());

	scope().push(attrDesc);
}

void DomainAttributesHandler::end() { scope().pop(); }

/* DomainFieldHandler */

void DomainFieldHandler::start(Variant::mapType &args)
{
	FieldDescriptor::FieldType type;
	if (args["isSubtree"].asBool()) {
		type = FieldDescriptor::FieldType::SUBTREE;
	} else {
		type = FieldDescriptor::FieldType::TREE;
	}

	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	Rooted<FieldDescriptor> field = parent->createFieldDescriptor(
	    logger(), type, args["name"].asString(), args["optional"].asBool());
	field->setLocation(location());

	scope().push(field);
}

void DomainFieldHandler::end() { scope().pop(); }

/* DomainFieldRefHandler */

void DomainFieldRefHandler::start(Variant::mapType &args)
{
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	const std::string &name = args["ref"].asString();
	scope().resolveFieldDescriptor(
	    name, parent, logger(),
	    [](Handle<Node> field, Handle<Node> parent, Logger &logger) {
		    if (field != nullptr) {
			    parent.cast<StructuredClass>()->addFieldDescriptor(
			        field.cast<FieldDescriptor>(), logger);
		    }
		});
}

void DomainFieldRefHandler::end() {}

/* DomainPrimitiveHandler */

void DomainPrimitiveHandler::start(Variant::mapType &args)
{
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	FieldDescriptor::FieldType fieldType;
	if (args["isSubtree"].asBool()) {
		fieldType = FieldDescriptor::FieldType::SUBTREE;
	} else {
		fieldType = FieldDescriptor::FieldType::TREE;
	}

	Rooted<FieldDescriptor> field = parent->createPrimitiveFieldDescriptor(
	    new UnknownType(manager()), logger(), fieldType,
	    args["name"].asString(), args["optional"].asBool());
	field->setLocation(location());

	const std::string &type = args["type"].asString();
	scope().resolve<Type>(type, field, logger(),
	                      [](Handle<Node> type, Handle<Node> field,
	                         Logger &logger) {
		if (type != nullptr) {
			field.cast<FieldDescriptor>()->setPrimitiveType(type.cast<Type>());
		}
	});

	scope().push(field);
}

void DomainPrimitiveHandler::end() { scope().pop(); }

/* DomainChildHandler */

void DomainChildHandler::start(Variant::mapType &args)
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
}

void DomainChildHandler::end() {}

/* DomainParentHandler */

void DomainParentHandler::start(Variant::mapType &args)
{
	Rooted<StructuredClass> strct = scope().selectOrThrow<StructuredClass>();

	Rooted<DomainParent> parent{
	    new DomainParent(strct->getManager(), args["ref"].asString(), strct)};
	parent->setLocation(location());
	scope().push(parent);
}

void DomainParentHandler::end() { scope().pop(); }

/* DomainParentFieldHandler */
void DomainParentFieldHandler::start(Variant::mapType &args)
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
			        parent.cast<Descriptor>()->createFieldDescriptor(
			            logger, type, name, optional);
			    field->addChild(strct.cast<StructuredClass>());
		    }
		});
}

void DomainParentFieldHandler::end() {}

/* DomainParentFieldRefHandler */

void DomainParentFieldRefHandler::start(Variant::mapType &args)
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
}

void DomainParentFieldRefHandler::end() {}

namespace RttiTypes {
const Rtti DomainParent =
    RttiBuilder<ousia::DomainParent>("DomainParent").parent(&Node);
}
}
