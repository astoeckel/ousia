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

		// Check whether this typesystem is a direct child of a domain
		Handle<Node> parent = scope().select({&RttiTypes::Domain});
		if (parent != nullptr) {
			parent.cast<Domain>()->referenceTypesystem(typesystem);
		}

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
		Rooted<Typesystem> typesystem = scope().select<Typesystem>();
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

		// Descend into the struct type
		scope().push(structType);
	}

	void end() override
	{
		// Descend from the struct type
		scope().pop();
	}

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

		Rooted<StructType> structType = scope().select<StructType>();
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

		Rooted<Typesystem> typesystem = scope().select<Typesystem>();
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

		const std::string &isa = args["isa"].asString();

		Rooted<Domain> domain = scope().select<Domain>();
		Rooted<StructuredClass> structuredClass = domain->createStructuredClass(
		    args["name"].asString(), args["cardinality"].asCardinality(),
		    nullptr, nullptr, args["transparent"].asBool(),
		    args["isRoot"].asBool());
		structuredClass->setLocation(location());

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

class ImportHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		// Make sure imports are still possible
		if (scope().getFlag(ParserFlag::POST_HEAD)) {
			logger().error("Imports must be listed before other commands.",
			               location());
			return;
		}

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

		// Fetch the parameters
		const std::string &rel = args["rel"].asString();
		const std::string &type = args["type"].asString();
		const std::string &src = args["src"].asString();

		// Perform the actual import, register the imported node within the leaf
		// node
		Rooted<Node> imported =
		    context().import(src, type, rel, leafRootNode->getReferenceTypes());
		if (imported != nullptr) {
			leafRootNode->reference(imported);
		}
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new ImportHandler{handlerData};
	}
};

namespace ParserStates {
/* Document states */
static const ParserState Document =
    ParserStateBuilder()
        .parent(&None)
        .elementHandler(DocumentHandler::create)
        .arguments({Argument::String("name", "")});

/* Domain states */
static const ParserState Domain = ParserStateBuilder()
                                      .parents({&None, &Document})
                                      .elementHandler(DomainHandler::create)
                                      .arguments({Argument::String("name")});
static const ParserState DomainStruct =
    ParserStateBuilder()
        .parent(&Domain)
        .elementHandler(DomainStructHandler::create)
        .arguments({Argument::String("name"),
                    Argument::Cardinality("cardinality", AnyCardinality),
                    Argument::Bool("isRoot", false),
                    Argument::Bool("transparent", false),
                    Argument::String("isa", "")});
static const ParserState DomainStructFields =
    ParserStateBuilder().parent(&DomainStruct).arguments({});
static const ParserState DomainStructField =
    ParserStateBuilder().parent(&DomainStructFields).arguments(
        {Argument::String("name", ""), Argument::Bool("isSubtree", false),
         Argument::Bool("optional", false)});
static const ParserState DomainStructPrimitive =
    ParserStateBuilder().parent(&DomainStructFields).arguments(
        {Argument::String("name", ""), Argument::Bool("optional", false),
         Argument::String("type")});

/* Typesystem states */
static const ParserState Typesystem =
    ParserStateBuilder()
        .parents({&None, &Domain})
        .elementHandler(TypesystemHandler::create)
        .arguments({Argument::String("name", "")});
static const ParserState TypesystemEnum =
    ParserStateBuilder().parent(&Typesystem);
static const ParserState TypesystemStruct =
    ParserStateBuilder()
        .parent(&Typesystem)
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
        .elementHandler(TypesystemConstantHandler::create)
        .arguments({Argument::String("name"), Argument::String("type"),
                    Argument::Any("value")});

/* Special states for import and include */
static const ParserState Import =
    ParserStateBuilder()
        .parents({&Document, &Typesystem, &Domain})
        .elementHandler(ImportHandler::create)
        .arguments({Argument::String("rel", ""), Argument::String("type", ""),
                    Argument::String("src")});
static const ParserState Include = ParserStateBuilder().parent(&All).arguments(
    {Argument::String("rel", ""), Argument::String("type", ""),
     Argument::String("src")});

static const std::multimap<std::string, const ParserState *> XmlStates{
    {"document", &Document},
    {"domain", &Domain},
    {"struct", &DomainStruct},
    {"fields", &DomainStructFields},
    {"field", &DomainStructField},
    {"primitive", &DomainStructPrimitive},
    {"typesystem", &Typesystem},
    {"enum", &TypesystemEnum},
    {"struct", &TypesystemStruct},
    {"field", &TypesystemStructField},
    {"constant", &TypesystemConstant},
    {"import", &Import},
    {"include", &Include}};
}
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
	ParserStack *stack = static_cast<ParserStack *>(XML_GetUserData(p));

	// Fetch the current location in the XML file
	size_t offs = XML_GetCurrentByteIndex(p);

	// Build the source location and update the default location of the
	// current
	// logger instance
	SourceLocation loc{stack->getContext().getSourceId(), offs, offs + len};
	stack->getContext().getLogger().setDefaultLocation(loc);
	return loc;
}

static void xmlStartElementHandler(void *p, const XML_Char *name,
                                   const XML_Char **attrs)
{
	XML_Parser parser = static_cast<XML_Parser>(p);
	SourceLocation loc = syncLoggerPosition(parser);

	ParserStack *stack = static_cast<ParserStack *>(XML_GetUserData(parser));

	Variant::mapType args;
	const XML_Char **attr = attrs;
	while (*attr) {
		const std::string key{*(attr++)};
		std::pair<bool, Variant> value = VariantReader::parseGenericString(
		    *(attr++), stack->getContext().getLogger());
		args.emplace(std::make_pair(key, value.second));
	}
	stack->start(std::string(name), args, loc);
}

static void xmlEndElementHandler(void *p, const XML_Char *name)
{
	XML_Parser parser = static_cast<XML_Parser>(p);
	ParserStack *stack = static_cast<ParserStack *>(XML_GetUserData(parser));

	syncLoggerPosition(parser);
	stack->end();
}

static void xmlCharacterDataHandler(void *p, const XML_Char *s, int len)
{
	XML_Parser parser = static_cast<XML_Parser>(p);
	ParserStack *stack = static_cast<ParserStack *>(XML_GetUserData(parser));

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

	// Create the parser stack instance and pass the reference to the state
	// machine descriptor
	ParserStack stack{ctx, ParserStates::XmlStates};
	XML_SetUserData(&p, &stack);
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

