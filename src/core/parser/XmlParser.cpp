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

#include <expat.h>

#include "XmlParser.hpp"

namespace ousia {

/**
 * The XmlParserData struct holds all information relevant to the expat callback
 * functions.
 */
struct XmlParserData {
	Rooted<Node> context;
	Logger &logger;

	XmlParserData(Handle<Node> context, Logger &logger)
	    : context(context), logger(logger)
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
	 * @param namespaceSeparator is the separator used to separate the namespace
	 * components in the node name given by expat.
	 */
	ScopedExpatXmlParser(const XML_Char *encoding, XML_Char namespaceSeparator)
	    : parser(nullptr)
	{
		parser = XML_ParserCreateNS("UTF-8", ':');
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

std::set<std::string> XmlParser::mimetypes()
{
	return std::set<std::string>{{"text/vnd.ousia.oxm", "text/vnd.ousia.oxd"}};
}

Rooted<Node> XmlParser::parse(std::istream &is, Handle<Node> context,
                              Logger &logger)
{
	// Create the parser object
	ScopedExpatXmlParser p{"UTF-8", ':'};

	// Set the callback functions, provide a pointer to a XmlParserData instance
	// as user data.
	XmlParserData ctx{context, logger};

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
			logger.error("XML: " + msg, line, column);
			break;
		}

		// Abort once there are no more bytes in the stream
		if (bytesRead == 0) {
			break;
		}
	}

	return nullptr;
}
}

