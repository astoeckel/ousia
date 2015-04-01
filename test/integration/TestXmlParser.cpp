/*
    Ousía
    Copyright (C) 2015  Benjamin Paaßen, Andreas Stöckel

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

#include <core/common/Utils.hpp>

#include "TestXmlParser.hpp"

namespace ousia {
namespace test {

/* Class XmlNode */

std::string XmlNode::path()
{
	std::string p;
	if (!parent.expired()) {
		std::shared_ptr<XmlNode> parentPtr = parent.lock();
		if (parentPtr.get() != nullptr) {
			p = parentPtr->path() + "/";
		}
	}
	return p + name;
}

bool XmlNode::compareTo(Logger &logger, std::shared_ptr<XmlNode> other,
               std::set<int> &errExpected, std::set<int> &errActual)
{
	bool ok = true;

	// Compare name and text
	if (name != other->name) {
		logger.fail(path() + ": names differ, expected \"" + name +
		            "\", but got \"" + other->name + "\"");
		ok = false;
	}
	if (text != other->text) {
		logger.fail(path() + ": texts differ, expected \"" + text +
		            "\", but got \"" + other->text + "\"");
		ok = false;
	}

	// Compare the attributes
	if (attributes.size() != other->attributes.size()) {
		logger.fail(
		    path() + ": attribute count differs, expected " +
		    std::to_string(attributes.size()) + " attributes, but got " +
		    std::to_string(other->attributes.size()) + " attributes");
		ok = false;
	}
	for (const auto &attribute : attributes) {
		auto it = other->attributes.find(attribute.first);
		if (it == other->attributes.end()) {
			logger.fail(path() + ": attribute \"" + attribute.first +
			            "\" is missing in actual output");
			ok = false;
		} else if (it->second != attribute.second) {
			logger.fail(path() + ": expected \"" + attribute.second +
			            "\" for attribute \"" + attribute.first +
			            "\" but got \"" + it->second + "\"");
			ok = false;
		}
	}

	// Compare the children
	if (children.size() != other->children.size()) {
		logger.fail(path() + ": children count differs, expected " +
		            std::to_string(children.size()) +
		            " children, but got " +
		            std::to_string(other->children.size()) + " children");
		ok = false;
	}

	// Store the actual position
	if (!ok) {
		logger.fail("Location in expected output is " +
		            std::to_string(line) + ":" + std::to_string(column) +
		            ", location in actual output is " +
		            std::to_string(other->line) + ":" +
		            std::to_string(other->column));
		errExpected.insert(line);
		errActual.insert(other->line);
	}

	// Compare the children
	const size_t count = std::min(children.size(), other->children.size());
	for (size_t i = 0; i < count; i++) {
		ok = children[i]->compareTo(logger, other->children[i], errExpected,
		                            errActual) &
		     ok;
	}

	return ok;
}

static const std::vector<std::string> IGNORE_TAGS{"import"};
static const std::vector<std::string> IGNORE_ATTRS{"xmlns"};

static bool checkIgnore(const std::vector<std::string> &ignoreList,
                        const std::string &name)
{
	for (const auto &s : ignoreList) {
		if (Utils::startsWith(s, name)) {
			return true;
		}
	}
	return false;
}

/**
 * Callback called by eXpat whenever a start handler is reached.
 */
static void xmlStartElementHandler(void *ref, const XML_Char *name,
                                   const XML_Char **attrs)
{
	XML_Parser parser = static_cast<XML_Parser>(ref);
	std::shared_ptr<XmlNode> &node =
	    *(static_cast<std::shared_ptr<XmlNode> *>(XML_GetUserData(parser)));

	// Store the child node in the parent node, check for ignoring nodes once
	// an element ends
	std::shared_ptr<XmlNode> childNode =
	    std::make_shared<XmlNode>(node, name);
	childNode->line = XML_GetCurrentLineNumber(parser);
	childNode->column = XML_GetCurrentColumnNumber(parser);
	node->children.push_back(childNode);
	node = childNode;

	// Assemble the node attributes
	const XML_Char **attr = attrs;
	while (*attr) {
		// Convert the C string to a std::string
		const std::string key{*(attr++)};
		const std::string value{*(attr++)};

		// Ignore certain attributes
		if (!checkIgnore(IGNORE_ATTRS, key)) {
			childNode->attributes.emplace(key, value);
		}
	}
}

static void xmlEndElementHandler(void *ref, const XML_Char *name)
{
	XML_Parser parser = static_cast<XML_Parser>(ref);
	std::shared_ptr<XmlNode> &node =
	    *(static_cast<std::shared_ptr<XmlNode> *>(XML_GetUserData(parser)));

	// Set the current node to the parent node
	node = node->parent.lock();

	// If the child node should have been ignored, remove it now
	if (checkIgnore(IGNORE_TAGS, name)) {
		node->children.pop_back();
	}
}

static void xmlCharacterDataHandler(void *ref, const XML_Char *s, int len)
{
	// Fetch a reference at the currently active node
	XML_Parser parser = static_cast<XML_Parser>(ref);
	std::shared_ptr<XmlNode> &node =
	    *(static_cast<std::shared_ptr<XmlNode> *>(XML_GetUserData(parser)));

	// Store a new text node in the current node
	std::string text = Utils::trim(std::string(s, len));
	if (!text.empty()) {
		std::shared_ptr<XmlNode> textNode =
		    std::make_shared<XmlNode>(node, "$text");
		textNode->text = text;
		textNode->line = XML_GetCurrentLineNumber(parser);
		textNode->column = XML_GetCurrentColumnNumber(parser);
		node->children.push_back(textNode);
	}
}

std::pair<bool, std::shared_ptr<XmlNode>> parseXml(
    Logger &logger, std::istream &is, std::set<int> &errLines)
{
	std::shared_ptr<XmlNode> root = std::make_shared<XmlNode>();
	std::shared_ptr<XmlNode> currentNode = root;

	XML_Parser parser = XML_ParserCreate("UTF-8");

	// Pass the reference to this parser instance to the XML handler
	XML_UseParserAsHandlerArg(parser);
	XML_SetUserData(parser, &currentNode);

	// Set the callback functions
	XML_SetStartElementHandler(parser, xmlStartElementHandler);
	XML_SetEndElementHandler(parser, xmlEndElementHandler);
	XML_SetCharacterDataHandler(parser, xmlCharacterDataHandler);

	// Feed data into expat while there is data to process
	constexpr size_t BUFFER_SIZE = 64 * 1024;
	bool ok = true;
	while (true) {
		// Fetch a buffer from expat for the input data
		char *buf = static_cast<char *>(XML_GetBuffer(parser, BUFFER_SIZE));
		if (!buf) {
			logger.fail("Cannot parse XML, out of memory");
			ok = false;
			break;
		}

		// Read into the buffer
		size_t bytesRead = is.read(buf, BUFFER_SIZE).gcount();

		// Parse the data and handle any XML error as exception
		if (!XML_ParseBuffer(parser, bytesRead, bytesRead == 0)) {
			int line = XML_GetCurrentLineNumber(parser);
			int column = XML_GetCurrentColumnNumber(parser);
			logger.fail("Cannot parse XML, " +
			            std::string(XML_ErrorString(XML_GetErrorCode(parser))) +
			            ", at line " + std::to_string(line) + ", column " +
			            std::to_string(column));
			errLines.insert(line);
			ok = false;
			break;
		}

		// Abort once there are no more bytes in the stream
		if (bytesRead == 0) {
			break;
		}
	}

	return std::pair<bool, std::shared_ptr<XmlNode>>(ok, root);
}

}
}

