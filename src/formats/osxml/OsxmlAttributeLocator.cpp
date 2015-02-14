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

#include <core/common/Location.hpp>
#include <core/common/CharReader.hpp>
#include <core/common/Utils.hpp>

#include "OsxmlAttributeLocator.hpp"

namespace ousia {

/**
 * Enum used internally in the statemachine of the xml argument parser.
 */
enum class XmlAttributeState {
	IN_TAG_NAME,
	SEARCH_ATTR,
	IN_ATTR_NAME,
	HAS_ATTR_NAME,
	HAS_ATTR_EQUALS,
	IN_ATTR_DATA
};

std::map<std::string, SourceLocation> OsxmlAttributeLocator::locate(
    CharReader &reader, size_t offs)
{
	std::map<std::string, SourceLocation> res;

	// Fork the reader, we don't want to mess up the XML parsing process, do we?
	CharReaderFork readerFork = reader.fork();

	// Move the read cursor to the start location, abort if this does not work
	if (offs != readerFork.seek(offs)) {
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
}

