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

