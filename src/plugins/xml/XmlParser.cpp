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
#include <core/common/RttiBuilder.hpp>
#include <core/common/Utils.hpp>
#include <core/common/VariantReader.hpp>
#include <core/parser/ParserStack.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Project.hpp>
#include <core/model/RootNode.hpp>
#include <core/model/Typesystem.hpp>

#include "XmlParser.hpp"

namespace ousia {

/* HeadNode Helper class */

namespace {
class HeadNode : public Node {
public:
	using Node::Node;
};
}

namespace RttiTypes {
static Rtti HeadNode = RttiBuilder<ousia::HeadNode>("HeadNode");
}

/* Element Handler Classes */

class DocumentHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		Rooted<Document> document =
		    project()->createDocument(args["name"].asString());
		document->setLocation(location());
		scope().push(document);
		scope().setFlag(ParserFlag::POST_HEAD, false);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new DocumentHandler{handlerData};
	}
};

class TypesystemHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		// Create the typesystem instance
		Rooted<Typesystem> typesystem =
		    project()->createTypesystem(args["name"].asString());
		typesystem->setLocation(location());

		// Push the typesystem onto the scope, set the POST_HEAD flag to true
		scope().push(typesystem);
		scope().setFlag(ParserFlag::POST_HEAD, false);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemHandler{handlerData};
	}
};

class TypesystemEnumHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		scope().setFlag(ParserFlag::POST_HEAD, true);

		// Fetch the current typesystem and create the enum node
		Rooted<Typesystem> typesystem = scope().selectOrThrow<Typesystem>();
		Rooted<EnumType> enumType =
		    typesystem->createEnumType(args["name"].asString());
		enumType->setLocation(location());

		scope().push(enumType);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemEnumHandler{handlerData};
	}
};

class TypesystemEnumEntryHandler : public Handler {
public:
	using Handler::Handler;

	std::string entry;

	void start(Variant::mapType &args) override {}

	void end() override
	{
		Rooted<EnumType> enumType = scope().selectOrThrow<EnumType>();
		enumType->addEntry(entry, logger());
	}

	void data(const std::string &data, int field) override
	{
		if (field != 0) {
			// TODO: This should be stored in the HandlerData
			logger().error("Enum entry only has one field.");
			return;
		}
		entry.append(data);
	}

	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemEnumEntryHandler{handlerData};
	}
};

class TypesystemStructHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
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
			    [](Handle<Node> parent, Handle<Node> structType,
			       Logger &logger) {
				    if (parent != nullptr) {
					    structType.cast<StructType>()->setParentStructure(
					        parent.cast<StructType>(), logger);
				    }
				});
		}
		scope().push(structType);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemStructHandler{handlerData};
	}
};

class TypesystemStructFieldHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
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
			scope().resolveType(
			    type, attribute, logger(),
			    [](Handle<Node> type, Handle<Node> attribute, Logger &logger) {
				    if (type != nullptr) {
					    attribute.cast<Attribute>()->setType(type.cast<Type>(),
					                                         logger);
				    }
				});
		}
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemStructFieldHandler{handlerData};
	}
};

class TypesystemConstantHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
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
				    constant.cast<Constant>()->setType(type.cast<Type>(),
				                                       logger);
			    }
			});
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemConstantHandler{handlerData};
	}
};

/*
 * Domain Handlers
 */

class DomainHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		Rooted<Domain> domain =
		    project()->createDomain(args["name"].asString());
		domain->setLocation(location());

		scope().push(domain);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainHandler{handlerData};
	}
};

class DomainStructHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		scope().setFlag(ParserFlag::POST_HEAD, true);

		Rooted<Domain> domain = scope().selectOrThrow<Domain>();

		Rooted<StructuredClass> structuredClass = domain->createStructuredClass(
		    args["name"].asString(), args["cardinality"].asCardinality(),
		    nullptr, nullptr, args["transparent"].asBool(),
		    args["isRoot"].asBool());
		structuredClass->setLocation(location());

		const std::string &isa = args["isa"].asString();
		if (!isa.empty()) {
			scope().resolve<StructuredClass>(
			    isa, structuredClass, logger(),
			    [](Handle<Node> superclass, Handle<Node> structuredClass,
			       Logger &logger) {
				    if (superclass != nullptr) {
					    structuredClass.cast<StructuredClass>()->setSuperclass(
					        superclass.cast<StructuredClass>());
				    }
				});
		}

		scope().push(structuredClass);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainStructHandler{handlerData};
	}
};

class DomainAnnotationHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		scope().setFlag(ParserFlag::POST_HEAD, true);

		Rooted<Domain> domain = scope().selectOrThrow<Domain>();

		Rooted<AnnotationClass> annotationClass =
		    domain->createAnnotationClass(args["name"].asString(), nullptr);
		annotationClass->setLocation(location());

		scope().push(annotationClass);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainAnnotationHandler{handlerData};
	}
};

class DomainFieldHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		FieldDescriptor::FieldType type;
		if (args["isSubtree"].asBool()) {
			type = FieldDescriptor::FieldType::SUBTREE;
		} else {
			type = FieldDescriptor::FieldType::TREE;
		}

		Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

		Rooted<FieldDescriptor> field = parent->createFieldDescriptor(
		    type, args["name"].asString(), args["optional"].asBool());
		field->setLocation(location());

		scope().push(field);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainFieldHandler{handlerData};
	}
};

class DomainFieldRefHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

		const std::string &name = args["name"].asString();
		scope().resolve<FieldDescriptor>(
		    name, parent, logger(),
		    [](Handle<Node> field, Handle<Node> parent, Logger &logger) {
			    if (field != nullptr) {
				    parent.cast<StructuredClass>()->addFieldDescriptor(
				        field.cast<FieldDescriptor>());
			    }
			});
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainFieldRefHandler{handlerData};
	}
};

class DomainPrimitiveHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

		Rooted<FieldDescriptor> field = parent->createPrimitiveFieldDescriptor(
		    nullptr, args["name"].asString(), args["optional"].asBool());
		field->setLocation(location());

		const std::string &type = args["type"].asString();
		scope().resolve<Type>(
		    type, field, logger(),
		    [](Handle<Node> type, Handle<Node> field, Logger &logger) {
			    if (type != nullptr) {
				    field.cast<FieldDescriptor>()->setPrimitiveType(
				        type.cast<Type>());
			    }
			});

		scope().push(field);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainPrimitiveHandler{handlerData};
	}
};

class DomainChildHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		Rooted<FieldDescriptor> field =
		    scope().selectOrThrow<FieldDescriptor>();

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

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainChildHandler{handlerData};
	}
};

class DummyParentNode : public Node {
public:
	DummyParentNode(Manager &mgr, std::string name, Handle<Node> parent)
	    : Node(mgr, name, parent)
	{
	}
};

namespace RttiTypes {
const Rtti DummyParentNode =
    RttiBuilder<ousia::DummyParentNode>("DummyParentNode").parent(&Node);
}

class DomainParentHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		Rooted<StructuredClass> strct =
		    scope().selectOrThrow<StructuredClass>();

		// TODO: Is there a better way for this?
		Rooted<DummyParentNode> dummy{new DummyParentNode(
		    strct->getManager(), args["name"].asString(), strct)};
		dummy->setLocation(location());
		scope().push(dummy);
	}

	void end() override { scope().pop(); }

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentHandler{handlerData};
	}
};

class DomainParentFieldHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		Rooted<DummyParentNode> dummy =
		    scope().selectOrThrow<DummyParentNode>();
		FieldDescriptor::FieldType type;
		if (args["isSubtree"].asBool()) {
			type = FieldDescriptor::FieldType::SUBTREE;
		} else {
			type = FieldDescriptor::FieldType::TREE;
		}

		const std::string &name = args["name"].asString();
		const bool optional = args["optional"].asBool();
		Rooted<StructuredClass> strct =
		    dummy->getParent().cast<StructuredClass>();

		// resolve the parent, create the declared field and add the declared
		// StructuredClass as child to it.
		scope().resolve<Descriptor>(
		    dummy->getName(), strct, logger(),
		    [type, name, optional](Handle<Node> parent, Handle<Node> strct,
		                              Logger &logger) {
			    if (parent != nullptr) {
				    Rooted<FieldDescriptor> field =
				        parent.cast<Descriptor>()->createFieldDescriptor(
				            type, name, optional);
				    field->addChild(strct.cast<StructuredClass>());
			    }
			});
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentFieldHandler{handlerData};
	}
};

class DomainParentFieldRefHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		Rooted<DummyParentNode> dummy =
		    scope().selectOrThrow<DummyParentNode>();

		const std::string &name = args["name"].asString();
		Rooted<StructuredClass> strct =
		    dummy->getParent().cast<StructuredClass>();
		auto loc = location();

		// resolve the parent, get the referenced field and add the declared
		// StructuredClass as child to it.
		scope().resolve<Descriptor>(dummy->getName(), strct, logger(),
		                            [name, loc](Handle<Node> parent,
		                                          Handle<Node> strct,
		                                          Logger &logger) {
			if (parent != nullptr) {
				auto res = parent.cast<Descriptor>()->resolve(
				    RttiTypes::FieldDescriptor, name);
				if (res.size() != 1) {
					logger.error(
					    std::string("Could not find referenced field ") + name,
					    loc);
					return;
				}
				Rooted<FieldDescriptor> field =
				    res[0].node.cast<FieldDescriptor>();
				field->addChild(strct.cast<StructuredClass>());
			}
		});
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentFieldRefHandler{handlerData};
	}
};

/*
 * Import and Include Handler
 */

class ImportIncludeHandler : public Handler {
public:
	using Handler::Handler;

	bool srcInArgs = false;
	std::string rel;
	std::string type;
	std::string src;

	void start(Variant::mapType &args) override
	{
		rel = args["rel"].asString();
		type = args["type"].asString();
		src = args["src"].asString();
		srcInArgs = !src.empty();
	}

	void data(const std::string &data, int field) override
	{
		if (srcInArgs) {
			logger().error("\"src\" attribute has already been set");
			return;
		}
		if (field != 0) {
			logger().error("Command has only one field.");
			return;
		}
		src.append(data);
	}
};

class ImportHandler : public ImportIncludeHandler {
public:
	using ImportIncludeHandler::ImportIncludeHandler;

	void start(Variant::mapType &args) override
	{
		ImportIncludeHandler::start(args);

		// Make sure imports are still possible
		if (scope().getFlag(ParserFlag::POST_HEAD)) {
			logger().error("Imports must be listed before other commands.",
			               location());
			return;
		}
	}

	void end() override
	{
		// Fetch the last node and check whether an import is valid at this
		// position
		Rooted<Node> leaf = scope().getLeaf();
		if (leaf == nullptr || !leaf->isa(RttiTypes::RootNode)) {
			logger().error(
			    "Import not supported here, must be inside a document, domain "
			    "or typesystem command.",
			    location());
			return;
		}
		Rooted<RootNode> leafRootNode = leaf.cast<RootNode>();

		// Perform the actual import, register the imported node within the leaf
		// node
		Rooted<Node> imported =
		    context().import(src, type, rel, leafRootNode->getReferenceTypes());
		if (imported != nullptr) {
			leafRootNode->reference(imported);
		}
	}

	static Handler *create(const HandlerData &handlerData)
	{
		return new ImportHandler{handlerData};
	}
};

class IncludeHandler : public ImportIncludeHandler {
public:
	using ImportIncludeHandler::ImportIncludeHandler;

	void start(Variant::mapType &args) override
	{
		ImportIncludeHandler::start(args);
	}

	void end() override
	{
		context().include(src, type, rel, {&RttiTypes::Node});
	}

	static Handler *create(const HandlerData &handlerData)
	{
		return new IncludeHandler{handlerData};
	}
};

namespace ParserStates {
/* Document states */
static const ParserState Document =
    ParserStateBuilder()
        .parent(&None)
        .createdNodeType(&RttiTypes::Document)
        .elementHandler(DocumentHandler::create)
        .arguments({Argument::String("name", "")});

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
                    Argument::Cardinality("cardinality", AnyCardinality),
                    Argument::Bool("isRoot", false),
                    Argument::Bool("transparent", false),
                    Argument::String("isa", "")});
// TODO: What about attributes?

static const ParserState DomainAnnotation =
    ParserStateBuilder()
        .parent(&Domain)
        .createdNodeType(&RttiTypes::AnnotationClass)
        .elementHandler(DomainAnnotationHandler::create)
        .arguments({Argument::String("name")});
// TODO: What about attributes?

static const ParserState DomainField =
    ParserStateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainFieldHandler::create)
        .arguments({Argument::String("name", DEFAULT_FIELD_NAME),
                    Argument::Bool("isSubtree", false),
                    Argument::Bool("optional", false)});

static const ParserState DomainFieldRef =
    ParserStateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainFieldRefHandler::create)
        .arguments({Argument::String("name", DEFAULT_FIELD_NAME)});

static const ParserState DomainStructPrimitive =
    ParserStateBuilder()
        .parents({&DomainStruct, &DomainAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainPrimitiveHandler::create)
        .arguments({Argument::String("name", DEFAULT_FIELD_NAME),
                    Argument::Bool("optional", false),
                    Argument::String("type")});

static const ParserState DomainStructChild =
    ParserStateBuilder()
        .parent(&DomainField)
        .elementHandler(DomainChildHandler::create)
        .arguments({Argument::String("ref")});

static const ParserState DomainStructParent =
    ParserStateBuilder()
        .parent(&DomainStruct)
        .createdNodeType(&RttiTypes::DummyParentNode)
        .elementHandler(DomainParentHandler::create)
        .arguments({Argument::String("name")});

static const ParserState DomainStructParentField =
    ParserStateBuilder()
        .parent(&DomainStructParent)
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainParentFieldHandler::create)
        .arguments({Argument::String("name", DEFAULT_FIELD_NAME),
                    Argument::Bool("isSubtree", false),
                    Argument::Bool("optional", false)});

static const ParserState DomainStructParentFieldRef =
    ParserStateBuilder()
        .parent(&DomainStructParent)
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(DomainParentFieldRefHandler::create)
        .arguments({Argument::String("name", DEFAULT_FIELD_NAME)});

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
    {"domain", &Domain},
    {"struct", &DomainStruct},
    {"annotation", &DomainAnnotation},
    {"field", &DomainField},
    {"fieldRef", &DomainFieldRef},
    {"primitive", &DomainStructPrimitive},
    {"child", &DomainStructChild},
    {"parent", &DomainStructParent},
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

/**
 * Structue containing the private data that is being passed to the
 * XML-Handlers.
 */
struct XMLUserData {
	/**
	 * Containing the depth of the current XML file
	 */
	size_t depth;

	/**
	 * Reference at the ParserStack instance.
	 */
	ParserStack *stack;

	/**
	 * Reference at the CharReader instance.
	 */
	CharReader *reader;

	/**
	 * Constructor of the XMLUserData struct.
	 *
	 * @param stack is a pointer at the ParserStack instance.
	 * @param reader is a pointer at the CharReader instance.
	 */
	XMLUserData(ParserStack *stack, CharReader *reader)
	    : depth(0), stack(stack), reader(reader)
	{
	}
};

/**
 * Wrapper class around the XML_Parser pointer which safely frees it whenever
 * the scope is left (e.g. because an exception was thrown).
 */
class ScopedExpatXmlParser {
private:
	/**
	 * Internal pointer to the XML_Parser instance.
	 */
	XML_Parser parser;

public:
	/**
	 * Constructor of the ScopedExpatXmlParser class. Calls XML_ParserCreateNS
	 * from the expat library. Throws a parser exception if the XML parser
	 * cannot be initialized.
	 *
	 * @param encoding is the protocol-defined encoding passed to expat (or
	 * nullptr if expat should determine the encoding by itself).
	 */
	ScopedExpatXmlParser(const XML_Char *encoding) : parser(nullptr)
	{
		parser = XML_ParserCreate(encoding);
		if (!parser) {
			throw LoggableException{
			    "Internal error: Could not create expat XML parser!"};
		}
	}

	/**
	 * Destuctor of the ScopedExpatXmlParser, frees the XML parser instance.
	 */
	~ScopedExpatXmlParser()
	{
		if (parser) {
			XML_ParserFree(parser);
			parser = nullptr;
		}
	}

	/**
	 * Returns the XML_Parser pointer.
	 */
	XML_Parser operator&() { return parser; }
};

/* Adapter Expat -> ParserStack */

static SourceLocation syncLoggerPosition(XML_Parser p, size_t len = 0)
{
	// Fetch the parser stack and the associated user data
	XMLUserData *userData = static_cast<XMLUserData *>(XML_GetUserData(p));
	ParserStack *stack = userData->stack;

	// Fetch the current location in the XML file
	size_t offs = XML_GetCurrentByteIndex(p);

	// Build the source location and update the default location of the
	// current
	// logger instance
	SourceLocation loc{stack->getContext().getSourceId(), offs, offs + len};
	stack->getContext().getLogger().setDefaultLocation(loc);
	return loc;
}

enum class XMLAttributeState {
	IN_TAG_NAME,
	SEARCH_ATTR,
	IN_ATTR_NAME,
	HAS_ATTR_NAME,
	HAS_ATTR_EQUALS,
	IN_ATTR_DATA
};

static std::map<std::string, SourceLocation> reconstructXMLAttributeOffsets(
    CharReader &reader, SourceLocation location)
{
	std::map<std::string, SourceLocation> res;

	// Fork the reader, we don't want to mess up the XML parsing process, do we?
	CharReaderFork readerFork = reader.fork();

	// Move the read cursor to the start location, abort if this does not work
	size_t offs = location.getStart();
	if (!location.isValid() || offs != readerFork.seek(offs)) {
		return res;
	}

	// Now all we need to do is to implement one half of an XML parser. As this
	// is inherently complicated we'll totaly fail at it. Don't care. All we
	// want to get is those darn offsets for pretty error messages... (and we
	// can assume the XML is valid as it was already read by expat)
	XMLAttributeState state = XMLAttributeState::IN_TAG_NAME;
	char c;
	std::stringstream attrName;
	while (readerFork.read(c)) {
		// Abort at the end of the tag
		if (c == '>' && state != XMLAttributeState::IN_ATTR_DATA) {
			return res;
		}

		// One state machine to rule them all, one state machine to find them,
		// One state machine to bring them all and in the darkness bind them
		// (the byte offsets)
		switch (state) {
			case XMLAttributeState::IN_TAG_NAME:
				if (Utils::isWhitespace(c)) {
					state = XMLAttributeState::SEARCH_ATTR;
				}
				break;
			case XMLAttributeState::SEARCH_ATTR:
				if (!Utils::isWhitespace(c)) {
					state = XMLAttributeState::IN_ATTR_NAME;
					attrName << c;
				}
				break;
			case XMLAttributeState::IN_ATTR_NAME:
				if (Utils::isWhitespace(c)) {
					state = XMLAttributeState::HAS_ATTR_NAME;
				} else if (c == '=') {
					state = XMLAttributeState::HAS_ATTR_EQUALS;
				} else {
					attrName << c;
				}
				break;
			case XMLAttributeState::HAS_ATTR_NAME:
				if (!Utils::isWhitespace(c)) {
					if (c == '=') {
						state = XMLAttributeState::HAS_ATTR_EQUALS;
						break;
					}
					// Well, this is a strange XML file... We expected to
					// see a '=' here! Try to continue with the
					// "HAS_ATTR_EQUALS" state as this state will hopefully
					// inlcude some error recovery
				} else {
					// Skip whitespace here
					break;
				}
			// Fallthrough
			case XMLAttributeState::HAS_ATTR_EQUALS:
				if (!Utils::isWhitespace(c)) {
					if (c == '"') {
						// Here we are! We have found the beginning of an
						// attribute. Let's quickly lock the current offset away
						// in the result map
						res.emplace(attrName.str(),
						            SourceLocation{reader.getSourceId(),
						                           readerFork.getOffset()});
						attrName.str(std::string{});
						state = XMLAttributeState::IN_ATTR_DATA;
					} else {
						// No, this XML file is not well formed. Assume we're in
						// an attribute name once again
						attrName.str(std::string{&c, 1});
						state = XMLAttributeState::IN_ATTR_NAME;
					}
				}
				break;
			case XMLAttributeState::IN_ATTR_DATA:
				if (c == '"') {
					// We're at the end of the attribute data, start anew
					state = XMLAttributeState::SEARCH_ATTR;
				}
				break;
		}
	}
	return res;
}

static void xmlStartElementHandler(void *p, const XML_Char *name,
                                   const XML_Char **attrs)
{
	XML_Parser parser = static_cast<XML_Parser>(p);
	XMLUserData *userData = static_cast<XMLUserData *>(XML_GetUserData(p));
	ParserStack *stack = userData->stack;

	SourceLocation loc = syncLoggerPosition(parser);

	// Read the argument locations -- this is only a stupid and slow hack,
	// but it is necessary, as expat doesn't give use the byte offset of the
	// arguments.
	std::map<std::string, SourceLocation> offs =
	    reconstructXMLAttributeOffsets(*userData->reader, loc);

	// Assemble the arguments
	Variant::mapType args;
	const XML_Char **attr = attrs;
	while (*attr) {
		// Convert the C string to a std::string
		const std::string key{*(attr++)};

		// Search the location of the key
		SourceLocation keyLoc;
		auto it = offs.find(key);
		if (it != offs.end()) {
			keyLoc = it->second;
		}

		// Parse the string, pass the location of the key
		std::pair<bool, Variant> value = VariantReader::parseGenericString(
		    *(attr++), stack->getContext().getLogger(), keyLoc.getSourceId(),
		    keyLoc.getStart());
		args.emplace(key, value.second);
	}

	// Call the start function
	std::string nameStr(name);
	if (nameStr != "ousia" || userData->depth > 0) {
		stack->start(std::string(name), args, loc);
	}

	// Increment the current depth
	userData->depth++;
}

static void xmlEndElementHandler(void *p, const XML_Char *name)
{
	XML_Parser parser = static_cast<XML_Parser>(p);
	XMLUserData *userData = static_cast<XMLUserData *>(XML_GetUserData(p));
	ParserStack *stack = userData->stack;

	syncLoggerPosition(parser);

	// Decrement the current depth
	userData->depth--;

	// Call the end function
	std::string nameStr(name);
	if (nameStr != "ousia" || userData->depth > 0) {
		stack->end();
	}
}

static void xmlCharacterDataHandler(void *p, const XML_Char *s, int len)
{
	XML_Parser parser = static_cast<XML_Parser>(p);
	XMLUserData *userData = static_cast<XMLUserData *>(XML_GetUserData(p));
	ParserStack *stack = userData->stack;

	size_t ulen = len > 0 ? static_cast<size_t>(len) : 0;
	syncLoggerPosition(parser, ulen);
	const std::string data = Utils::trim(std::string{s, ulen});
	if (!data.empty()) {
		stack->data(data);
	}
}

/* Class XmlParser */

void XmlParser::doParse(CharReader &reader, ParserContext &ctx)
{
	// Create the parser object
	ScopedExpatXmlParser p{"UTF-8"};

	// Create the parser stack instance, if we're starting on a non-empty scope,
	// try to deduce the parser state
	ParserStack stack(ctx, ParserStates::XmlStates);
	if (!ctx.getScope().isEmpty()) {
		if (!stack.deduceState()) {
			return;
		}
	}

	// Pass the reference to the ParserStack to the XML handler
	XMLUserData data(&stack, &reader);
	XML_SetUserData(&p, &data);
	XML_UseParserAsHandlerArg(&p);

	// Set the callback functions
	XML_SetStartElementHandler(&p, xmlStartElementHandler);
	XML_SetEndElementHandler(&p, xmlEndElementHandler);
	XML_SetCharacterDataHandler(&p, xmlCharacterDataHandler);

	// Feed data into expat while there is data to process
	constexpr size_t BUFFER_SIZE = 64 * 1024;
	while (true) {
		// Fetch a buffer from expat for the input data
		char *buf = static_cast<char *>(XML_GetBuffer(&p, BUFFER_SIZE));
		if (!buf) {
			throw LoggableException{
			    "Internal error: XML parser out of memory!"};
		}

		// Read into the buffer
		size_t bytesRead = reader.readRaw(buf, BUFFER_SIZE);

		// Parse the data and handle any XML error
		if (!XML_ParseBuffer(&p, bytesRead, bytesRead == 0)) {
			// Fetch the xml parser byte offset
			size_t offs = XML_GetCurrentByteIndex(&p);

			// Throw a corresponding exception
			XML_Error code = XML_GetErrorCode(&p);
			std::string msg = std::string{XML_ErrorString(code)};
			throw LoggableException{"XML: " + msg,
			                        SourceLocation{ctx.getSourceId(), offs}};
		}

		// Abort once there are no more bytes in the stream
		if (bytesRead == 0) {
			break;
		}
	}
}
}

