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
#include <core/common/Utils.hpp>
#include <core/common/VariantReader.hpp>
#include <core/parser/ParserStack.hpp>
#include <core/model/Typesystem.hpp>

#include "XmlParser.hpp"

namespace ousia {
namespace parser {
namespace xml {

using namespace ousia::model;

/* Document structure */
static const State STATE_DOCUMENT = 0;
static const State STATE_HEAD = 1;
static const State STATE_BODY = 2;

/* Special commands */
static const State STATE_USE = 100;
static const State STATE_INCLUDE = 101;
static const State STATE_INLINE = 102;

/* Type system definitions */
static const State STATE_TYPESYSTEM = 200;
static const State STATE_TYPES = 201;
static const State STATE_CONSTANTS = 202;
static const State STATE_CONSTANT = 203;
static const State STATE_ENUM = 204;
static const State STATE_STRUCT = 205;
static const State STATE_FIELD = 206;

class TypesystemHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override
	{
		scope().push(new model::Typesystem(manager(), args["name"].asString()));
	}

	void end() override
	{
		scope().performDeferredResolution(logger());
		// TODO: Automatically call validate in "pop"?
		scope().getLeaf()->validate(logger());
		scope().pop();
	}

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
		Rooted<Typesystem> typesystem = scope().getLeaf().cast<Typesystem>();
		Rooted<StructType> structType = typesystem->createStructType(name);

		// Try to resolve the parent type and set it as parent structure
		if (!parent.empty()) {
			scope().resolve<StructType>(Utils::split(parent, '.'), logger(),
			                            [structType](Handle<StructType> parent,
			                                         Logger &logger) mutable {
				                            structType->setParentStructure(
				                                parent, logger);
				                        },
			                            location());
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
		/*		const std::string &name = args["name"].asString();
		        const std::string &type = args["parent"].asString();
		        const Variant &defaultValue = args["default"];
		        const bool optional = !(defaultValue.isObject() &&
		   defaultValue.asObject() == nullptr);*/

		// Try to resolve the
	}

	void end() override {}

	static Handler *create(const HandlerData &handlerData)
	{
		return new StructFieldHandler{handlerData};
	}
};

static const std::multimap<std::string, HandlerDescriptor> XML_HANDLERS{
    /* Document tags */
    {"document", {{STATE_NONE}, nullptr, STATE_DOCUMENT}},
    {"head", {{STATE_DOCUMENT}, nullptr, STATE_HEAD}},
    {"body", {{STATE_DOCUMENT}, nullptr, STATE_BODY, true}},

    /* Special commands */
    {"use", {{STATE_HEAD}, nullptr, STATE_USE}},
    {"include", {{STATE_ALL}, nullptr, STATE_INCLUDE}},
    {"inline", {{STATE_ALL}, nullptr, STATE_INLINE}},

    /* Typesystem */
    {"typesystem",
     {{STATE_NONE, STATE_HEAD},
      TypesystemHandler::create,
      STATE_TYPESYSTEM,
      false,
      {Argument::String("name")}}},
    {"types", {{STATE_TYPESYSTEM}, nullptr, STATE_TYPES}},
    {"enum", {{STATE_TYPES}, nullptr, STATE_ENUM}},
    {"struct",
     {{STATE_TYPES},
      StructHandler::create,
      STATE_STRUCT,
      false,
      {Argument::String("name"), Argument::String("parent", "")}}},
    {"field",
     {{{STATE_STRUCT}},
      StructFieldHandler::create,
      STATE_FIELD,
      false,
      {Argument::String("name"), Argument::String("type"),
       Argument::Any("default", Variant::fromObject(nullptr))}}},
    {"constants", {{STATE_TYPESYSTEM}, nullptr, STATE_CONSTANTS}},
    {"constant", {{STATE_CONSTANTS}, nullptr, STATE_CONSTANT}}};

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
	// Fetch the current location in the XML file
	int line = XML_GetCurrentLineNumber(p);
	int column = XML_GetCurrentColumnNumber(p);
	size_t offs = XML_GetCurrentByteIndex(p);
	SourceLocation loc{line, column, offs};

	// Update the default location of the current logger instance
	ParserStack *stack = static_cast<ParserStack *>(XML_GetUserData(p));
	stack->getContext().logger.setDefaultLocation(loc);
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
		    *(attr++), stack->getContext().logger);
		args.emplace(std::make_pair(key, value.second));
	}
	stack->start(std::string(name), args, loc);
}

static void xmlEndElementHandler(void *p, const XML_Char *name)
{
	XML_Parser parser = static_cast<XML_Parser>(p);
	syncLoggerPosition(parser);

	ParserStack *stack = static_cast<ParserStack *>(XML_GetUserData(parser));
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

std::set<std::string> XmlParser::mimetypes()
{
	return std::set<std::string>{{"text/vnd.ousia.oxm", "text/vnd.ousia.oxd"}};
}

Rooted<Node> XmlParser::parse(CharReader &reader, ParserContext &ctx)
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
			// Fetch the current line number and column
			int line = XML_GetCurrentLineNumber(&p);
			int column = XML_GetCurrentColumnNumber(&p);
			size_t offs = XML_GetCurrentByteIndex(&p);

			// Throw a corresponding exception
			XML_Error code = XML_GetErrorCode(&p);
			std::string msg = std::string{XML_ErrorString(code)};
			throw LoggableException{"XML: " + msg, line, column, offs};
		}

		// Abort once there are no more bytes in the stream
		if (bytesRead == 0) {
			break;
		}
	}
	return nullptr;
}
}
}
}

