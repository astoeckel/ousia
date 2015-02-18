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

#include <vector>

#include <core/common/CharReader.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Variant.hpp>
#include <core/common/VariantReader.hpp>
#include <core/common/Utils.hpp>
#include <core/common/WhitespaceHandler.hpp>

#include "OsxmlAttributeLocator.hpp"
#include "OsxmlEventParser.hpp"

namespace ousia {

/* Class OsxmlEventParser */

/**
 * Class containing data used by the internal functions.
 */
class OsxmlEventParserData {
public:
	/**
	 * Contains the current depth of the parsing process.
	 */
	ssize_t depth;

	/**
	 * Set to a value larger or equal to zero if the parser is currently inside
	 * an annotation end tag -- the value represents the depth in which the
	 * tag was opened.
	 */
	ssize_t annotationEndTagDepth;

	/**
	 * Current character data buffer.
	 */
	std::vector<char> textBuf;

	/**
	 * Current whitespace buffer (for the trimming whitspace mode)
	 */
	std::vector<char> whitespaceBuf;

	/**
	 * Flag indicating whether a whitespace character was present (for the
	 * collapsing whitespace mode).
	 */
	bool hasWhitespace;

	/**
	 * Current character data start.
	 */
	size_t textStart;

	/**
	 * Current character data end.
	 */
	size_t textEnd;

	/**
	 * Default constructor.
	 */
	OsxmlEventParserData();

	/**
	 * Increments the depth.
	 */
	void incrDepth();

	/**
	 * Decrement the depth and reset the annotationEndTagDepth flag.
	 */
	void decrDepth();

	/**
	 * Returns true if we're currently inside an end tag.
	 */
	bool inAnnotationEndTag();

	/**
	 * Returns true if character data is available.
	 *
	 * @return true if character data is available.
	 */
	bool hasText();

	/**
	 * Returns a Variant containing the character data and its location.
	 *
	 * @return a string variant containing the text data and the character
	 * location.
	 */
	Variant getText(SourceId sourceId);
};

/* Class GuardedExpatXmlParser */

/**
 * Wrapper class around the XML_Parser pointer which safely frees it whenever
 * the scope is left (e.g. because an exception was thrown).
 */
class GuardedExpatXmlParser {
private:
	/**
	 * Internal pointer to the XML_Parser instance.
	 */
	XML_Parser parser;

public:
	/**
	 * Constructor of the GuardedExpatXmlParser class. Calls XML_ParserCreateNS
	 * from the expat library. Throws a parser exception if the XML parser
	 * cannot be initialized.
	 *
	 * @param encoding is the protocol-defined encoding passed to expat (or
	 * nullptr if expat should determine the encoding by itself).
	 */
	GuardedExpatXmlParser(const XML_Char *encoding) : parser(nullptr)
	{
		parser = XML_ParserCreate(encoding);
		if (!parser) {
			throw LoggableException{
			    "Internal error: Could not create expat XML parser!"};
		}
	}

	/**
	 * Destuctor of the GuardedExpatXmlParser, frees the XML parser instance.
	 */
	~GuardedExpatXmlParser()
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

/**
 * Name of the special outer tag used for allowing multiple top-level elements
 * in an xml file.
 */
static const std::string TOP_LEVEL_TAG{"ousia"};

/**
 * Prefix used to indicate the start of an annoation (note the trailing colon)
 */
static const std::string ANNOTATION_START_PREFIX{"a:start:"};

/**
 * Prefix used to indicate the end of an annotation.
 */
static const std::string ANNOTATION_END_PREFIX{"a:end"};

/**
 * Synchronizes the position of the xml parser with the default location of the
 * logger instance.
 *
 * @param p is a pointer at the xml parser instance.
 * @param len is the length of the string that should be refered to.
 * @return the SourceLocation that has been set in the logger.
 */
static SourceLocation xmlSyncLoggerPosition(XML_Parser p, size_t len = 0)
{
	// Fetch the OsxmlEventParser instance
	OsxmlEventParser *parser =
	    static_cast<OsxmlEventParser *>(XML_GetUserData(p));

	// Fetch the current location in the XML file and set the default location
	// in the logger
	size_t offs = XML_GetCurrentByteIndex(p);
	SourceLocation loc =
	    SourceLocation{parser->getReader().getSourceId(), offs, offs + len};
	parser->getLogger().setDefaultLocation(loc);

	// Return the fetched location
	return loc;
}

/**
 * Callback called by eXpat whenever a start handler is reached.
 */
static void xmlStartElementHandler(void *ref, const XML_Char *name,
                                   const XML_Char **attrs)
{
	// Fetch the XML_Parser pointer p and a pointer at the OsxmlEventParser
	XML_Parser p = static_cast<XML_Parser>(ref);
	OsxmlEventParser *parser =
	    static_cast<OsxmlEventParser *>(XML_GetUserData(p));

	// If there is any text data in the buffer, issue that first
	if (parser->getData().hasText()) {
		parser->getEvents().data(
		    parser->getData().getText(parser->getReader().getSourceId()));
	}

	// Read the argument locations -- this is only a stupid and slow hack,
	// but it is necessary, as expat doesn't give use the byte offset of the
	// arguments.
	std::map<std::string, SourceLocation> attributeOffsets =
	    OsxmlAttributeLocator::locate(parser->getReader(),
	                                  XML_GetCurrentByteIndex(p));

	// Update the logger position
	SourceLocation loc = xmlSyncLoggerPosition(p);

	// Fetch the location of the name
	SourceLocation nameLoc = loc;
	auto it = attributeOffsets.find("$tag");
	if (it != attributeOffsets.end()) {
		nameLoc = it->second;
	}
	// Increment the current depth
	parser->getData().incrDepth();

	// Make sure we're currently not inside an annotation end tag -- this would
	// be highly illegal!
	if (parser->getData().inAnnotationEndTag()) {
		parser->getLogger().error(
		    "No tags allowed inside an annotation end tag", nameLoc);
		return;
	}

	// Assemble the arguments
	Variant::mapType args;
	const XML_Char **attr = attrs;
	while (*attr) {
		// Convert the C string to a std::string
		const std::string key{*(attr++)};

		// Ignore xml namespace declarations
		if (Utils::startsWith(key, "xmlns:") && parser->getData().depth == 1) {
			attr++;
			continue;
		}

		// Search the location of the key
		SourceLocation keyLoc;
		auto it = attributeOffsets.find(key);
		if (it != attributeOffsets.end()) {
			keyLoc = it->second;
		}

		// Parse the string, pass the location of the key
		std::pair<bool, Variant> value = VariantReader::parseGenericString(
		    *(attr++), parser->getLogger(), keyLoc.getSourceId(),
		    keyLoc.getStart());

		// Set the overall location of the parsed element to the attribute
		// location
		value.second.setLocation(keyLoc);

		// Store the keys in the map
		args.emplace(key, value.second).second;
	}

	// Fetch the name of the tag, check for special tags
	std::string nameStr(name);
	if (nameStr == TOP_LEVEL_TAG && parser->getData().depth == 1) {
		// We're in the top-level and the magic tag is reached -- just
		// ignore it and issue a warning for each argument that has been given
		for (const auto &arg : args) {
			parser->getLogger().warning(std::string("Ignoring attribute \"") +
			                                arg.first +
			                                std::string("\" for magic tag \"") +
			                                TOP_LEVEL_TAG + std::string("\""),
			                            arg.second);
		}
	} else if (Utils::startsWith(nameStr, ANNOTATION_START_PREFIX)) {
		// Assemble a name variant containing the name minus the prefix
		Variant nameVar =
		    Variant::fromString(nameStr.substr(ANNOTATION_START_PREFIX.size()));
		nameVar.setLocation(nameLoc);

		// Issue the "annotationStart" event
		parser->getEvents().annotationStart(nameVar, args);
	} else if (Utils::startsWith(nameStr, ANNOTATION_END_PREFIX)) {
		// Assemble a name variant containing the name minus the prefix
		nameStr = nameStr.substr(ANNOTATION_END_PREFIX.size());

		// Discard a potentially leading colon
		if (!nameStr.empty() && nameStr[0] == ':') {
			nameStr = nameStr.substr(1);
		}

		// Assemble the variant containing the name and its location
		Variant nameVar = Variant::fromString(nameStr);
		nameVar.setLocation(nameLoc);

		// Check whether a "name" attribute was given
		Variant elementName;
		for (const auto &arg : args) {
			if (arg.first == "name") {
				elementName = arg.second;
			} else {
				parser->getLogger().warning(
				    std::string("Ignoring attribute \"") + arg.first +
				        "\" in annotation end tag",
				    arg.second);
			}
		}

		// Set the annotationEndTagDepth to disallow any further tags to be
		// opened inside the annotation end tag.
		parser->getData().annotationEndTagDepth = parser->getData().depth;

		// Issue the "annotationEnd" event
		parser->getEvents().annotationEnd(nameVar, args);
	} else {
		// Just issue a "commandStart" event in any other case
		Variant nameVar = Variant::fromString(nameStr);
		nameVar.setLocation(nameLoc);
		parser->getEvents().command(nameVar, args);
	}
}

static void xmlEndElementHandler(void *ref, const XML_Char *name)
{
	// Fetch the XML_Parser pointer p and a pointer at the OsxmlEventParser
	XML_Parser p = static_cast<XML_Parser>(ref);
	OsxmlEventParser *parser =
	    static_cast<OsxmlEventParser *>(XML_GetUserData(p));

	// Synchronize the position of the logger with teh position
	xmlSyncLoggerPosition(p);

	// Abort as long as we're in an annotation end tag
	if (parser->getData().inAnnotationEndTag()) {
		parser->getData().decrDepth();
		return;
	}

	// Decrement the current depth
	parser->getData().decrDepth();

	// If there is any text data in the buffer, issue that first
	if (parser->getData().hasText()) {
		parser->getEvents().data(
		    parser->getData().getText(parser->getReader().getSourceId()));
	}

	// Abort if the special ousia tag ends here
	std::string nameStr{name};
	if (nameStr == TOP_LEVEL_TAG && parser->getData().depth == 0) {
		return;
	}

	// Issue the "fieldEnd" event
	parser->getEvents().fieldEnd();
}

static void xmlCharacterDataHandler(void *ref, const XML_Char *s, int len)
{
	// Fetch the XML_Parser pointer p and a pointer at the OsxmlEventParser
	XML_Parser p = static_cast<XML_Parser>(ref);
	OsxmlEventParser *parser =
	    static_cast<OsxmlEventParser *>(XML_GetUserData(p));

	// Abort as long as we're in an annotation end tag
	if (parser->getData().inAnnotationEndTag()) {
		return;
	}

	// Convert the signed (smell the 90's C library here?) length to an usigned
	// value
	size_t ulen = len > 0 ? static_cast<size_t>(len) : 0;

	// Synchronize the logger position
	SourceLocation loc = xmlSyncLoggerPosition(p, ulen);

	// Fetch some variables for convenience
	const WhitespaceMode mode = parser->getWhitespaceMode();
	OsxmlEventParserData &data = parser->getData();
	std::vector<char> &textBuf = data.textBuf;
	std::vector<char> &whitespaceBuf = data.whitespaceBuf;
	bool &hasWhitespace = data.hasWhitespace;
	size_t &textStart = data.textStart;
	size_t &textEnd = data.textEnd;

	size_t pos = loc.getStart();
	for (size_t i = 0; i < ulen; i++, pos++) {
		switch (mode) {
			case WhitespaceMode::PRESERVE:
				PreservingWhitespaceHandler::append(s[i], pos, pos + 1, textBuf,
				                                    textStart, textEnd);
				break;
			case WhitespaceMode::TRIM:
				TrimmingWhitespaceHandler::append(s[i], pos, pos + 1, textBuf,
				                                  textStart, textEnd,
				                                  whitespaceBuf);
				break;
			case WhitespaceMode::COLLAPSE:
				CollapsingWhitespaceHandler::append(s[i], pos, pos + 1, textBuf,
				                                    textStart, textEnd,
				                                    hasWhitespace);
				break;
		}
	}
}

/* Class OsxmlEvents */

OsxmlEvents::~OsxmlEvents() {}

/* Class OsxmlEventParser */

OsxmlEventParserData::OsxmlEventParserData()
    : depth(0),
      annotationEndTagDepth(-1),
      hasWhitespace(false),
      textStart(0),
      textEnd(0)
{
}

void OsxmlEventParserData::incrDepth() { depth++; }

void OsxmlEventParserData::decrDepth()
{
	if (depth > 0) {
		depth--;
	}
	if (depth < annotationEndTagDepth) {
		annotationEndTagDepth = -1;
	}
}

bool OsxmlEventParserData::inAnnotationEndTag()
{
	return (annotationEndTagDepth > 0) && (depth >= annotationEndTagDepth);
}

bool OsxmlEventParserData::hasText() { return !textBuf.empty(); }

Variant OsxmlEventParserData::getText(SourceId sourceId)
{
	// Create a variant containing the string data and the location
	Variant var =
	    Variant::fromString(std::string{textBuf.data(), textBuf.size()});
	var.setLocation({sourceId, textStart, textEnd});

	// Reset the text buffers
	textBuf.clear();
	whitespaceBuf.clear();
	hasWhitespace = false;
	textStart = 0;
	textEnd = 0;

	// Return the variant
	return var;
}

/* Class OsxmlEventParser */

OsxmlEventParser::OsxmlEventParser(CharReader &reader, OsxmlEvents &events,
                                   Logger &logger)
    : reader(reader),
      events(events),
      logger(logger),
      whitespaceMode(WhitespaceMode::COLLAPSE),
      data(new OsxmlEventParserData())
{
}

OsxmlEventParser::~OsxmlEventParser() {}

void OsxmlEventParser::parse()
{
	// Create the parser object
	GuardedExpatXmlParser p{"UTF-8"};

	// Reset the depth
	data->depth = 0;

	// Pass the reference to this parser instance to the XML handler
	XML_SetUserData(&p, this);
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
			throw OusiaException{"Internal error: XML parser out of memory!"};
		}

		// Read into the buffer
		size_t bytesRead = reader.readRaw(buf, BUFFER_SIZE);

		// Parse the data and handle any XML error as exception
		if (!XML_ParseBuffer(&p, bytesRead, bytesRead == 0)) {
			throw LoggableException{
			    "XML: " + std::string{XML_ErrorString(XML_GetErrorCode(&p))},
			    xmlSyncLoggerPosition(&p)};
		}

		// Abort once there are no more bytes in the stream
		if (bytesRead == 0) {
			break;
		}
	}
}

void OsxmlEventParser::setWhitespaceMode(WhitespaceMode whitespaceMode)
{
	this->whitespaceMode = whitespaceMode;
}

WhitespaceMode OsxmlEventParser::getWhitespaceMode() const
{
	return whitespaceMode;
}

CharReader &OsxmlEventParser::getReader() const { return reader; }

Logger &OsxmlEventParser::getLogger() const { return logger; }

OsxmlEvents &OsxmlEventParser::getEvents() const { return events; }

OsxmlEventParserData &OsxmlEventParser::getData() const { return *data; }
}

