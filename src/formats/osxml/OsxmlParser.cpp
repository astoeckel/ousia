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

#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include <expat.h>

#include <core/common/CharReader.hpp>
#include <core/common/Utils.hpp>
#include <core/common/VariantReader.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserStack.hpp>
#include <core/parser/stack/DocumentHandler.hpp>
#include <core/parser/stack/DomainHandler.hpp>
#include <core/parser/stack/ImportIncludeHandler.hpp>
#include <core/parser/stack/TypesystemHandler.hpp>
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Typesystem.hpp>

#include "XmlParser.hpp"

namespace ousia {

namespace ParserStates {
/* Document states */
static const ParserState Document =
    ParserStateBuilder()
        .parent(&None)
        .createdNodeType(&RttiTypes::Document)
        .elementHandler(DocumentHandler::create)
        .arguments({Argument::String("name", "")});

static const ParserState DocumentChild =
    ParserStateBuilder()
        .parents({&Document, &DocumentChild})
        .createdNodeTypes({&RttiTypes::StructureNode,
                           &RttiTypes::AnnotationEntity,
                           &RttiTypes::DocumentField})
        .elementHandler(DocumentChildHandler::create);

/* Domain states */
static const ParserState Domain = ParserStateBuilder()
                                      .parents({&None, &Document})
                                      .createdNodeType(&RttiTypes::Domain)
                                      .elementHandler(DomainHandler::create)
                                      .arguments({Argument::String("name")});

static const ParserState DomainStruct =
    ParserStateBuilder()
        .parent(&Domain)
        .createdNodeType(&RttiTypes::StructuredClass)
        .elementHandler(DomainStructHandler::create)
        .arguments({Argument::String("name"),
                    Argument::Cardinality("cardinality", Cardinality::any()),
                    Argument::Bool("isRoot", false),
                    Argument::Bool("transparent", false),
                    Argument::String("isa", "")});

static const ParserState DomainAnnotation =
    ParserStateBuilder()
        .parent(&Domain)
        .createdNodeType(&RttiTypes::AnnotationClass)
        .elementHandler(DomainAnnotationHandler::create)
        .arguments({Argument::String("name")});

static const ParserState DomainAttributes =
    ParserStateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::StructType)
        .elementHandler(DomainAttributesHandler::create)
        .arguments({});

static const ParserState DomainAttribute =
    ParserStateBuilder()
        .parent(&DomainAttributes)
        .elementHandler(TypesystemStructFieldHandler::create)
        .arguments({Argument::String("name"), Argument::String("type"),
                    Argument::Any("default", Variant::fromObject(nullptr))});

static const ParserState DomainField =
    ParserStateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainFieldHandler::create)
        .arguments({Argument::String("name", ""),
                    Argument::Bool("isSubtree", false),
                    Argument::Bool("optional", false)});

static const ParserState DomainFieldRef =
    ParserStateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainFieldRefHandler::create)
        .arguments({Argument::String("ref", DEFAULT_FIELD_NAME)});

static const ParserState DomainStructPrimitive =
    ParserStateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainPrimitiveHandler::create)
        .arguments(
            {Argument::String("name", ""), Argument::Bool("isSubtree", false),
             Argument::Bool("optional", false), Argument::String("type")});

static const ParserState DomainStructChild =
    ParserStateBuilder()
        .parent(&DomainField)
        .elementHandler(DomainChildHandler::create)
        .arguments({Argument::String("ref")});

static const ParserState DomainStructParent =
    ParserStateBuilder()
        .parent(&DomainStruct)
        .createdNodeType(&RttiTypes::DomainParent)
        .elementHandler(DomainParentHandler::create)
        .arguments({Argument::String("ref")});

static const ParserState DomainStructParentField =
    ParserStateBuilder()
        .parent(&DomainStructParent)
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainParentFieldHandler::create)
        .arguments({Argument::String("name", ""),
                    Argument::Bool("isSubtree", false),
                    Argument::Bool("optional", false)});

static const ParserState DomainStructParentFieldRef =
    ParserStateBuilder()
        .parent(&DomainStructParent)
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainParentFieldRefHandler::create)
        .arguments({Argument::String("ref", DEFAULT_FIELD_NAME)});

/* Typesystem states */
static const ParserState Typesystem =
    ParserStateBuilder()
        .parents({&None, &Domain})
        .createdNodeType(&RttiTypes::Typesystem)
        .elementHandler(TypesystemHandler::create)
        .arguments({Argument::String("name", "")});

static const ParserState TypesystemEnum =
    ParserStateBuilder()
        .parent(&Typesystem)
        .createdNodeType(&RttiTypes::EnumType)
        .elementHandler(TypesystemEnumHandler::create)
        .arguments({Argument::String("name")});

static const ParserState TypesystemEnumEntry =
    ParserStateBuilder()
        .parent(&TypesystemEnum)
        .elementHandler(TypesystemEnumEntryHandler::create)
        .arguments({});

static const ParserState TypesystemStruct =
    ParserStateBuilder()
        .parent(&Typesystem)
        .createdNodeType(&RttiTypes::StructType)
        .elementHandler(TypesystemStructHandler::create)
        .arguments({Argument::String("name"), Argument::String("parent", "")});

static const ParserState TypesystemStructField =
    ParserStateBuilder()
        .parent(&TypesystemStruct)
        .elementHandler(TypesystemStructFieldHandler::create)
        .arguments({Argument::String("name"), Argument::String("type"),
                    Argument::Any("default", Variant::fromObject(nullptr))});

static const ParserState TypesystemConstant =
    ParserStateBuilder()
        .parent(&Typesystem)
        .createdNodeType(&RttiTypes::Constant)
        .elementHandler(TypesystemConstantHandler::create)
        .arguments({Argument::String("name"), Argument::String("type"),
                    Argument::Any("value")});

/* Special states for import and include */
static const ParserState Import =
    ParserStateBuilder()
        .parents({&Document, &Typesystem, &Domain})
        .elementHandler(ImportHandler::create)
        .arguments({Argument::String("rel", ""), Argument::String("type", ""),
                    Argument::String("src", "")});

static const ParserState Include =
    ParserStateBuilder()
        .parent(&All)
        .elementHandler(IncludeHandler::create)
        .arguments({Argument::String("rel", ""), Argument::String("type", ""),
                    Argument::String("src", "")});

static const std::multimap<std::string, const ParserState *> XmlStates{
    {"document", &Document},
    {"*", &DocumentChild},
    {"domain", &Domain},
    {"struct", &DomainStruct},
    {"annotation", &DomainAnnotation},
    {"attributes", &DomainAttributes},
    {"attribute", &DomainAttribute},
    {"field", &DomainField},
    {"fieldRef", &DomainFieldRef},
    {"primitive", &DomainStructPrimitive},
    {"childRef", &DomainStructChild},
    {"parentRef", &DomainStructParent},
    {"field", &DomainStructParentField},
    {"fieldRef", &DomainStructParentFieldRef},
    {"typesystem", &Typesystem},
    {"enum", &TypesystemEnum},
    {"entry", &TypesystemEnumEntry},
    {"struct", &TypesystemStruct},
    {"field", &TypesystemStructField},
    {"constant", &TypesystemConstant},
    {"import", &Import},
    {"include", &Include}};
}


}

