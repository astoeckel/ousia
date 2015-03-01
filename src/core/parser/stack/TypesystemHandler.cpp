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

#include <core/model/Document.hpp>
#include <core/model/Ontology.hpp>
#include <core/model/Typesystem.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

#include "DocumentHandler.hpp"
#include "OntologyHandler.hpp"
#include "State.hpp"
#include "TypesystemHandler.hpp"

namespace ousia {
namespace parser_stack {

/* TypesystemHandler */

bool TypesystemHandler::start(Variant::mapType &args)
{
	// Create the typesystem instance
	Rooted<Typesystem> typesystem =
	    context().getProject()->createTypesystem(args["name"].asString());
	typesystem->setLocation(location());

	// If the typesystem is defined inside a ontology, add a reference to the
	// typesystem to the ontology -- do the same with a document, if no ontology
	// is found
	Rooted<Ontology> ontology = scope().select<Ontology>();
	if (ontology != nullptr) {
		ontology->reference(typesystem);
	} else {
		Rooted<Document> document = scope().select<Document>();
		if (document != nullptr) {
			document->reference(typesystem);
		}
	}

	// Push the typesystem onto the scope, set the POST_HEAD flag to true
	scope().push(typesystem);
	scope().setFlag(ParserFlag::POST_HEAD, false);

	return true;
}

void TypesystemHandler::end() { scope().pop(logger()); }

/* TypesystemEnumHandler */

bool TypesystemEnumHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	// Fetch the current typesystem and create the enum node
	Rooted<Typesystem> typesystem = scope().selectOrThrow<Typesystem>();
	Rooted<EnumType> enumType =
	    typesystem->createEnumType(args["name"].asString());
	enumType->setLocation(location());

	scope().push(enumType);

	return true;
}

void TypesystemEnumHandler::end() { scope().pop(logger()); }

/* TypesystemEnumEntryHandler */

void TypesystemEnumEntryHandler::doHandle(const Variant &fieldData,
                                          Variant::mapType &args)
{
	Rooted<EnumType> enumType = scope().selectOrThrow<EnumType>();
	enumType->addEntry(fieldData.asString(), logger());
}

/* TypesystemStructHandler */

bool TypesystemStructHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	// Fetch the arguments used for creating this type
	const std::string &name = args["name"].asString();
	const std::string &parent = args["parent"].asString();

	// Fetch the current typesystem and create the struct node
	Rooted<Typesystem> typesystem = scope().selectOrThrow<Typesystem>();
	Rooted<StructType> structType = typesystem->createStructType(name);
	structType->setLocation(location());

	// Try to resolve the parent type and set it as parent structure
	if (!parent.empty()) {
		scope().resolve<StructType>(
		    parent, structType, logger(),
		    [](Handle<Node> parent, Handle<Node> structType, Logger &logger) {
			    if (parent != nullptr) {
				    structType.cast<StructType>()->setParentStructure(
				        parent.cast<StructType>(), logger);
			    }
			});
	}
	scope().push(structType);

	return true;
}

void TypesystemStructHandler::end() { scope().pop(logger()); }

/* TypesystemStructFieldHandler */

bool TypesystemStructFieldHandler::start(Variant::mapType &args)
{
	// Read the argument values
	const std::string &name = args["name"].asString();
	const std::string &type = args["type"].asString();
	const Variant &defaultValue = args["default"];
	const bool optional =
	    !(defaultValue.isObject() && defaultValue.asObject() == nullptr);

	Rooted<StructType> structType = scope().selectOrThrow<StructType>();
	Rooted<Attribute> attribute =
	    structType->createAttribute(name, defaultValue, optional, logger());
	attribute->setLocation(location());

	// Try to resolve the type and default value
	if (optional) {
		scope().resolveTypeWithValue(
		    type, attribute, attribute->getDefaultValue(), logger(),
		    [](Handle<Node> type, Handle<Node> attribute, Logger &logger) {
			    if (type != nullptr) {
				    attribute.cast<Attribute>()->setType(type.cast<Type>(),
				                                         logger);
			    }
			});
	} else {
		scope().resolveType(type, attribute, logger(),
		                    [](Handle<Node> type, Handle<Node> attribute,
		                       Logger &logger) {
			if (type != nullptr) {
				attribute.cast<Attribute>()->setType(type.cast<Type>(), logger);
			}
		});
	}

	return true;
}

/* TypesystemConstantHandler */

bool TypesystemConstantHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	// Read the argument values
	const std::string &name = args["name"].asString();
	const std::string &type = args["type"].asString();
	const Variant &value = args["value"];

	Rooted<Typesystem> typesystem = scope().selectOrThrow<Typesystem>();
	Rooted<Constant> constant = typesystem->createConstant(name, value);
	constant->setLocation(location());

	// Try to resolve the type
	scope().resolveTypeWithValue(
	    type, constant, constant->getValue(), logger(),
	    [](Handle<Node> type, Handle<Node> constant, Logger &logger) {
		    if (type != nullptr) {
			    constant.cast<Constant>()->setType(type.cast<Type>(), logger);
		    }
		});

	return true;
}

namespace States {
const State Typesystem = StateBuilder()
                             .parents({&None, &Ontology, &Document})
                             .createdNodeType(&RttiTypes::Typesystem)
                             .elementHandler(TypesystemHandler::create)
                             .arguments({Argument::String("name", "")});

const State TypesystemEnum = StateBuilder()
                                 .parent(&Typesystem)
                                 .createdNodeType(&RttiTypes::EnumType)
                                 .elementHandler(TypesystemEnumHandler::create)
                                 .arguments({Argument::String("name")});

const State TypesystemEnumEntry =
    StateBuilder()
        .parent(&TypesystemEnum)
        .elementHandler(TypesystemEnumEntryHandler::create)
        .arguments({});

const State TypesystemStruct =
    StateBuilder()
        .parent(&Typesystem)
        .createdNodeType(&RttiTypes::StructType)
        .elementHandler(TypesystemStructHandler::create)
        .arguments({Argument::String("name"), Argument::String("parent", "")});

const State TypesystemStructField =
    StateBuilder()
        .parent(&TypesystemStruct)
        .elementHandler(TypesystemStructFieldHandler::create)
        .arguments({Argument::String("name"), Argument::String("type"),
                    Argument::Any("default", Variant::fromObject(nullptr))});

const State TypesystemConstant =
    StateBuilder()
        .parent(&Typesystem)
        .createdNodeType(&RttiTypes::Constant)
        .elementHandler(TypesystemConstantHandler::create)
        .arguments({Argument::String("name"), Argument::String("type"),
                    Argument::Any("value")});
}
}
}

