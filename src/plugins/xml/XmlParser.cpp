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

#include <expat.h>

#include <core/parser/ParserStack.hpp>

#include "XmlParser.hpp"

namespace ousia {
namespace parser {
namespace xml {

/* Document structure */
static const State STATE_DOCUMENT = 0;
static const State STATE_HEAD = 1;
static const State STATE_BODY = 2;

/* Special commands */
static const State STATE_USE = 100;
static const State STATE_INCLUDE = 101;
static const State STATE_INLINE = 102;

/* Type system definitions */
static const State STATE_TYPES = 200;
static const State STATE_CONSTANT = 201;
static const State STATE_ENUM = 202;
static const State STATE_STRUCT = 203;

static Handler* createTestHandler(const ParserContext &ctx,
                                        std::string name, State state,
                                        State parentState, bool isChild)
{
	return nullptr;
}

static const std::multimap<std::string, HandlerDescriptor> XML_HANDLERS{
	/* Documents */
	{"document", {{STATE_NONE}, createTestHandler, STATE_DOCUMENT}},
	{"head", {{STATE_DOCUMENT}, createTestHandler, STATE_HEAD}},
	{"body", {{STATE_DOCUMENT}, createTestHandler, STATE_BODY, true}},

	/* Special commands */
	{"use", {{STATE_HEAD}, createTestHandler, STATE_USE}},
	{"include", {{STATE_ALL}, createTestHandler, STATE_INCLUDE}},
	{"inline", {{STATE_ALL}, createTestHandler, STATE_INLINE}},

	/* Typesystem definitions */
	{"types", {{STATE_NONE, STATE_HEAD}, createTestHandler, STATE_TYPES}},
	{"enum", {{STATE_TYPES}, createTestHandler, STATE_ENUM}},
	{"struct", {{STATE_TYPES}, createTestHandler, STATE_STRUCT}},
	{"constant", {{STATE_TYPES}, createTestHandler, STATE_CONSTANT}}
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
	ScopedExpatXmlParser(const XML_Char *encoding)
	    : parser(nullptr)
	{
		parser = XML_ParserCreate(encoding);
		if (!parser) {
			throw ParserException{
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

static void xmlStartElementHandler(void *userData, const XML_Char *name,
                                   const XML_Char **attrs)
{
	std::cout << "start tag: " << name << std::endl;
	const XML_Char **attr = attrs;
	while (*attr) {
		std::cout << "\t" << *attr;
		attr++;
		std::cout << " -> " << *attr << std::endl;
		attr++;
	}
}

static void xmlEndElementHandler(void *userData, const XML_Char *name) {
	std::cout << "end tag: " << name << std::endl;
}


static void xmlCharacterDataHandler(void *userData, const XML_Char *s, int len) {
	std::cout << "\tdata: " << std::string(s, len) << std::endl;
}

std::set<std::string> XmlParser::mimetypes()
{
	return std::set<std::string>{{"text/vnd.ousia.oxm", "text/vnd.ousia.oxd"}};
}

Rooted<Node> XmlParser::parse(std::istream &is, ParserContext &ctx)
{
	// Create the parser object
	ScopedExpatXmlParser p{"UTF-8"};
	XML_SetUserData(&p, &ctx);

	// Set the callback functions
	XML_SetStartElementHandler(&p, xmlStartElementHandler);
	XML_SetEndElementHandler(&p, xmlEndElementHandler);
	XML_SetCharacterDataHandler(&p, xmlCharacterDataHandler);

	// Feed data into expat while there is data to process
	const std::streamsize BUFFER_SIZE = 4096;  // TODO: Move to own header?
	while (true) {
		// Fetch a buffer from expat for the input data
		char *buf = static_cast<char *>(XML_GetBuffer(&p, BUFFER_SIZE));
		if (!buf) {
			throw ParserException{"Internal error: XML parser out of memory!"};
		}

		// Read the input data from the stream
		const std::streamsize bytesRead = is.read(buf, BUFFER_SIZE).gcount();

		// Parse the data and handle any XML error
		if (!XML_ParseBuffer(&p, bytesRead, bytesRead == 0)) {
			const int line = XML_GetCurrentLineNumber(&p);
			const int column = XML_GetCurrentColumnNumber(&p);
			const XML_Error code = XML_GetErrorCode(&p);
			const std::string msg = std::string{XML_ErrorString(code)};
			throw ParserException{"XML Syntax Error: " + msg, line, column,
			                      false};
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

