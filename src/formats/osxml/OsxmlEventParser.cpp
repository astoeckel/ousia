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

#include <core/common/Logger.hpp>
#include <core/common/Variant.hpp>
#include <core/common/Utils.hpp>

#include "OsxmlEventParser.hpp"

namespace ousia {

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
	 * Default constructor.
	 */
	OsxmlEventParserData() : depth(0), annotationEndTagDepth(-1) {}

	/**
	 * Increments the depth.
	 */
	void incrDepth() { depth++; }

	/**
	 * Decrement the depth and reset the annotationEndTagDepth flag.
	 */
	void decrDepth()
	{
		if (depth > 0) {
			depth--;
		}
		if (depth < annotationEndTagDepth) {
			annotationEndTagDepth = -1;
		}
	}

	/**
	 * Returns true if we're currently inside an end tag.
	 */
	bool inAnnotationEndTag() { depth >= annotationEndTagDepth; }
};

namespace {
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

/**
 * Enum used internally in the statemachine of the micro-xml argument parser.
 */
enum class XmlAttributeState {
	IN_TAG_NAME,
	SEARCH_ATTR,
	IN_ATTR_NAME,
	HAS_ATTR_NAME,
	HAS_ATTR_EQUALS,
	IN_ATTR_DATA
};

/**
 * Function used to reconstruct the location of the attributes of a XML tag in
 * the source code. This is necessary, as the xml parser only returns an offset
 * to the begining of a tag and not to the position of the individual arguments.
 *
 * @param reader is the char reader from which the character data should be
 * read.
 * @param offs is a byte offset in the xml file pointing at the "<" character of
 * the tag.
 * @return a map from attribute keys to the corresponding location (including
 * range) of the atribute. Also contains the location of the tagname in the
 * form of the virtual attribute "$tag".
 */
static std::map<std::string, SourceLocation> xmlReconstructAttributeOffsets(
    CharReader &reader, size_t offs)
{
	std::map<std::string, SourceLocation> res;

	// Fork the reader, we don't want to mess up the XML parsing process, do we?
	CharReaderFork readerFork = reader.fork();

	// Move the read cursor to the start location, abort if this does not work
	if (!location.isValid() || offs != readerFork.seek(offs)) {
		return res;
	}

	// Now all we need to do is to implement one half of an XML parser. As this
	// is inherently complicated we'll totaly fail at it. Don't care. All we
	// want to get is those darn offsets for pretty error messages... (and we
	// can assume the XML is valid as it was already read by expat)
	XmlAttributeState state = XmlAttributeState::IN_TAG_NAME;
	char c;
	std::stringstream attrName;
	while (readerFork.read(c)) {
		// Abort at the end of the tag
		if (c == '>' && state != XmlAttributeState::IN_ATTR_DATA) {
			return res;
		}

		// One state machine to rule them all, one state machine to find them,
		// One state machine to bring them all and in the darkness bind them
		// (the byte offsets)
		switch (state) {
			case XmlAttributeState::IN_TAG_NAME:
				if (Utils::isWhitespace(c)) {
					res.emplace("$tag",
					            SourceLocation{reader.getSourceId(), offs + 1,
					                           readerFork.getOffset() - 1});
					state = XmlAttributeState::SEARCH_ATTR;
				}
				break;
			case XmlAttributeState::SEARCH_ATTR:
				if (!Utils::isWhitespace(c)) {
					state = XmlAttributeState::IN_ATTR_NAME;
					attrName << c;
				}
				break;
			case XmlAttributeState::IN_ATTR_NAME:
				if (Utils::isWhitespace(c)) {
					state = XmlAttributeState::HAS_ATTR_NAME;
				} else if (c == '=') {
					state = XmlAttributeState::HAS_ATTR_EQUALS;
				} else {
					attrName << c;
				}
				break;
			case XmlAttributeState::HAS_ATTR_NAME:
				if (!Utils::isWhitespace(c)) {
					if (c == '=') {
						state = XmlAttributeState::HAS_ATTR_EQUALS;
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
			case XmlAttributeState::HAS_ATTR_EQUALS:
				if (!Utils::isWhitespace(c)) {
					if (c == '"') {
						// Here we are! We have found the beginning of an
						// attribute. Let's quickly lock the current offset away
						// in the result map
						res.emplace(attrName.str(),
						            SourceLocation{reader.getSourceId(),
						                           readerFork.getOffset()});
						state = XmlAttributeState::IN_ATTR_DATA;
					} else {
						// No, this XML file is not well formed. Assume we're in
						// an attribute name once again
						attrName.str(std::string{&c, 1});
						state = XmlAttributeState::IN_ATTR_NAME;
					}
				}
				break;
			case XmlAttributeState::IN_ATTR_DATA:
				if (c == '"') {
					// We're at the end of the attribute data, set the end
					// location
					auto it = res.find(attrName.str());
					if (it != res.end()) {
						it->second.setEnd(readerFork.getOffset() - 1);
					}

					// Reset the attribute name and restart the search
					attrName.str(std::string{});
					state = XmlAttributeState::SEARCH_ATTR;
				}
				break;
		}
	}
	return res;
}

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
	parser->getLogger().setDefaultLocation(location);

	// Return the fetched location
	return loc;
}

/**
 * Prefix used to indicate the start of an annoation,
 */
static const std::string ANNOTATION_START_PREFIX{"a:start:"};

/**
 * Prefix used to indicate the end of an annotation.
 */
static const std::string ANNOTATION_END_PREFIX{"a:end"};

/**
 * Callback called by eXpat whenever a start handler is reached.
 */
static void xmlStartElementHandler(void *ref, const XML_Char *name,
                                   const XML_Char **attrs)
{
	// Fetch the XML_Parser pointer p and a pointer at the OsxmlEventParser
	XML_Parser p = static_cast<XML_Parser>(ref);
	OsxmlEventParser *parser = static_cast<XMLUserData *>(XML_GetUserData(p));

	// Read the argument locations -- this is only a stupid and slow hack,
	// but it is necessary, as expat doesn't give use the byte offset of the
	// arguments.
	std::map<std::string, SourceLocation> attributeOffsets =
	    xmlReconstructXMLAttributeOffsets(*userData->reader,
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
		logger.error("No tags allowed inside an annotation end tag", nameLoc);
		return;
	}

	// Assemble the arguments
	Variant::mapType args;
	const XML_Char **attr = attrs;
	while (*attr) {
		// Convert the C string to a std::string
		const std::string key{*(attr++)};

		// Search the location of the key
		SourceLocation keyLoc;
		auto it = attributeOffsets.find(key);
		if (it != attributeOffsets.end()) {
			keyLoc = it->second;
		}

		// Parse the string, pass the location of the key
		std::pair<bool, Variant> value = VariantReader::parseGenericString(
		    *(attr++), stack->getContext().getLogger(), keyLoc.getSourceId(),
		    keyLoc.getStart());

		// Set the overall location of the parsed element to the attribute
		// location
		value.second->setLocation(keyLoc);

		// Store the
		if (!args.emplace(key, value.second).second) {
			parser->getLogger().warning(
			    std::string("Attribute \"") + key +
			        "\" defined multiple times, only using first definition",
			    keyLoc);
		}
	}

	// Fetch the name of the tag, check for special tags
	std::string nameStr(name);
	if (nameStr == "ousia" && parser->getData().depth == 1) {
		// We're in the top-level and the magic "ousia" tag is reached -- just
		// ignore it and issue a warning for each argument that has been given
		for (const auto &arg : args) {
			parser->getLogger().warning(
			    std::string("Ignoring attribute \"") + arg.first +
			        std::string("\" for magic tag \"ousia\""),
			    arg.second);
		}
	} else if (Utils::startsWith(nameStr, ANNOTATION_START_PREFIX)) {
		// Assemble a name variant containing the name minus the prefix
		Variant nameVar = nameStr.substr(ANNOTATION_START_PREFIX.size());
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
		parser->getEvents().commandStart(nameVar, args);
	}
}

static void xmlEndElementHandler(void *p, const XML_Char *name)
{
	// Fetch the XML_Parser pointer p and a pointer at the OsxmlEventParser
	XML_Parser p = static_cast<XML_Parser>(ref);
	OsxmlEventParser *parser = static_cast<XMLUserData *>(XML_GetUserData(p));

	// Synchronize the position of the logger with teh position
	xmlSyncLoggerPosition(parser);

	// Decrement the current depth
	parser->getData().decrDepth();

	// Abort as long as we're in an annotation end tag
	if (parser->getData().inAnnotationEndTag()) {
		return;
	}

	// Abort if the special ousia tag ends here
	if (nameStr == "ousia" && parser->getData().depth == 0) {
		return;
	}

	// Issue the "fieldEnd" event
	parser->getEvents().fieldEnd();
}

static void xmlCharacterDataHandler(void *p, const XML_Char *s, int len)
{
	// Fetch the XML_Parser pointer p and a pointer at the OsxmlEventParser
	XML_Parser p = static_cast<XML_Parser>(ref);
	OsxmlEventParser *parser = static_cast<XMLUserData *>(XML_GetUserData(p));

	// TODO
/*	size_t ulen = len > 0 ? static_cast<size_t>(len) : 0;
	syncLoggerPosition(parser, ulen);
	const std::string data = Utils::trim(std::string{s, ulen});
	if (!data.empty()) {
		stack->data(data);
	}*/
}
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

void OsxmlEventParser::parse(CharReader &reader)
{
	// Create the parser object
	ScopedExpatXmlParser p{"UTF-8"};

	// Reset the depth
	depth = 0;

	// Pass the reference to the ParserStack to the XML handler
	XMLUserData data(&stack, &reader);
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
			    xmlSyncLoggerPosition(p)};
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

CharReader &OsxmlEventParser::getCharReader() { return charReader; }

Logger &OsxmlEventParser::getLogger() { return logger; }

OsxmlEvents &OsxmlEventParser::getEvents() { return events; }

OsxmlEventParserData &OsxmlEventParser::getData() { return *data; }
}

