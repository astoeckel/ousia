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

#include "TypesystemHandler.hpp"

#include <core/model/Typesystem.hpp>
#include <core/parser/ParserScope.hpp>

namespace ousia {

/* TypesystemHandler */

void TypesystemHandler::start(Variant::mapType &args)
{
	// Create the typesystem instance
	Rooted<Typesystem> typesystem =
	    project()->createTypesystem(args["name"].asString());
	typesystem->setLocation(location());

	// Push the typesystem onto the scope, set the POST_HEAD flag to true
	scope().push(typesystem);
	scope().setFlag(ParserFlag::POST_HEAD, false);
}

void TypesystemHandler::end() { scope().pop(); }

/* TypesystemEnumHandler */

void TypesystemEnumHandler::start(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	// Fetch the current typesystem and create the enum node
	Rooted<Typesystem> typesystem = scope().selectOrThrow<Typesystem>();
	Rooted<EnumType> enumType =
	    typesystem->createEnumType(args["name"].asString());
	enumType->setLocation(location());

	scope().push(enumType);
}

void TypesystemEnumHandler::end() { scope().pop(); }

/* TypesystemEnumEntryHandler */

void TypesystemEnumEntryHandler::start(Variant::mapType &args) {}

void TypesystemEnumEntryHandler::end()
{
	Rooted<EnumType> enumType = scope().selectOrThrow<EnumType>();
	enumType->addEntry(entry, logger());
}

void TypesystemEnumEntryHandler::data(const std::string &data, int field)
{
	if (field != 0) {
		// TODO: This should be stored in the HandlerData
		logger().error("Enum entry only has one field.");
		return;
	}
	entry.append(data);
}

/* TypesystemStructHandler */

void TypesystemStructHandler::start(Variant::mapType &args)
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
}

void TypesystemStructHandler::end() { scope().pop(); }

/* TypesystemStructFieldHandler */

void TypesystemStructFieldHandler::start(Variant::mapType &args)
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
}

void TypesystemStructFieldHandler::end() {}

/* TypesystemConstantHandler */

void TypesystemConstantHandler::start(Variant::mapType &args)
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
}

void TypesystemConstantHandler::end() {}
}
