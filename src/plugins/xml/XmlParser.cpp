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

class HeadHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		// Make sure the "HEAD" node is actually allowed here
		if (scope().getFlag(ParserFlag::POST_HEAD)) {
			throw LoggableException{
			    "\"head\" tag not allowed here, head was already specified or "
			    "another command was given first",
			    location()};
		}

		// Insert a new HeadNode instance
		scope().push(new HeadNode{manager()});
	}

	void end() override
	{
		// Remove the HeadNode instance from the stack
		scope().pop();
		scope().setFlag(ParserFlag::POST_HEAD, true);
	}

	static Handler *create(const HandlerData &handlerData)
	{
		return new HeadHandler{handlerData};
	}
};

class DisableHeadHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		scope().setFlag(ParserFlag::POST_HEAD, true);
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new DisableHeadHandler{handlerData};
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

class StructHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
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
		return new StructHandler{handlerData};
	}
};

class StructFieldHandler : public Handler {
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
			scope().resolveTypeWithValue(type, attribute, attribute->getDefaultValue(), logger(),
				                  [](Handle<Node> type, Handle<Node> attribute,
				                     Logger &logger) {
				if (type != nullptr) {
					attribute.cast<Attribute>()->setType(type.cast<Type>(), logger);
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

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new StructFieldHandler{handlerData};
	}
};

class ConstantHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		// Read the argument values
		const std::string &name = args["name"].asString();
		const std::string &type = args["type"].asString();
		const Variant &value = args["value"];

		Rooted<Typesystem> typesystem = scope().select<Typesystem>();
		Rooted<Constant> constant = typesystem->createConstant(name, value);
		constant->setLocation(location());

		// Try to resolve the type
		scope().resolveTypeWithValue(type, constant, constant->getValue(), logger(),
		                      [](Handle<Node> type, Handle<Node> constant,
		                         Logger &logger) {
			if (type != nullptr) {
				constant.cast<Constant>()->setType(type.cast<Type>(), logger);
			}
		});
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new ConstantHandler{handlerData};
	}
};

/* Document structure */
static const State STATE_DOCUMENT = 0;
static const State STATE_DOCUMENT_HEAD = 1;

/* Special commands */
static const State STATE_IMPORT = 100;
static const State STATE_INCLUDE = 101;

/* Type system definitions */
static const State STATE_TYPESYSTEM = 200;
static const State STATE_TYPESYSTEM_HEAD = 201;
static const State STATE_TYPES = 202;
static const State STATE_CONSTANTS = 203;
static const State STATE_CONSTANT = 204;
static const State STATE_ENUM = 205;
static const State STATE_STRUCT = 206;
static const State STATE_FIELD = 207;

/* Domain definitions */
static const State STATE_DOMAIN = 300;
static const State STATE_DOMAIN_HEAD = 301;

static const std::multimap<std::string, HandlerDescriptor> XML_HANDLERS{
    /* Document tags */
    {"document",
     {{STATE_NONE},
      DocumentHandler::create,
      STATE_DOCUMENT,
      true,
      {Argument::String("name", "")}}},
    {"head", {{STATE_DOCUMENT}, HeadHandler::create, STATE_DOCUMENT_HEAD}},

    /* Special commands */
    {"import",
     {{STATE_DOCUMENT_HEAD, STATE_TYPESYSTEM_HEAD}, nullptr, STATE_IMPORT}},
    {"include", {{STATE_ALL}, nullptr, STATE_INCLUDE}},

    /* Typesystem */
    {"typesystem",
     {{STATE_NONE, STATE_DOMAIN_HEAD},
      TypesystemHandler::create,
      STATE_TYPESYSTEM,
      false,
      {Argument::String("name")}}},
    {"head", {{STATE_TYPESYSTEM}, HeadHandler::create, STATE_TYPESYSTEM}},
    {"types", {{STATE_TYPESYSTEM}, DisableHeadHandler::create, STATE_TYPES}},
    {"enum", {{STATE_TYPES}, nullptr, STATE_ENUM}},
    {"struct",
     {{STATE_TYPES},
      StructHandler::create,
      STATE_STRUCT,
      false,
      {Argument::String("name"), Argument::String("parent", "")}}},
    {"field",
     {{STATE_STRUCT},
      StructFieldHandler::create,
      STATE_FIELD,
      false,
      {Argument::String("name"), Argument::String("type"),
       Argument::Any("default", Variant::fromObject(nullptr))}}},
    {"constants",
     {{STATE_TYPESYSTEM}, DisableHeadHandler::create, STATE_CONSTANTS}},
    {"constant",
     {{STATE_CONSTANTS},
      ConstantHandler::create,
      STATE_CONSTANT,
      false,
      {Argument::String("name"), Argument::String("type"),
       Argument::Any("value")}}}};

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

static SourceLocation syncLoggerPosition(XML_Parser p)
{
	// Fetch the parser stack and the associated user data
	ParserStack *stack = static_cast<ParserStack *>(XML_GetUserData(p));

	// Fetch the current location in the XML file
	size_t offs = XML_GetCurrentByteIndex(p);

	// Build the source location and update the default location of the current
	// logger instance
	SourceLocation loc{stack->getContext().getSourceId(), offs};
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

	const std::string data =
	    Utils::trim(std::string{s, static_cast<size_t>(len)});
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
	ParserStack stack{ctx, XML_HANDLERS};
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

