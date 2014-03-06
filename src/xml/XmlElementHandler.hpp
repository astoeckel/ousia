/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#ifndef _OUSIA_XML_XML_ELEMENT_HANDLER_HPP_
#define _OUSIA_XML_XML_ELEMENT_HANDLER_HPP_

#include <functional>

namespace ousia {
namespace xml {

/**
 * Structure used internally for representing a function that is capable of
 * handling a certain xml element tag.
 */
class XmlElementHandler {

private:
	/**
	 * Name of the XML element the handler is capable of handling.
	 */
	const char *name;

	/**
	 * Handler function.
	 */
	std::function<bool()> handler;

	/**
	 * Counter that can be used in order to realize elements that may only
	 * appear a certain number of times.
	 */
	int count;

	/**
	 * Contains the current count of matches. Contains the maximum count a
	 * certain element must appear. If -1 the element may appear a unlimited
	 * number of times.
	 */
	int maxCount;

	/**
	 * A certain other handler this one depends on (the other element must have
	 * appeared at least once in order for this handler to match). If set to
	 * nullptr no requirement relationship is established.
	 */
	const XmlElementHandler *requiredElement;

public:

	/**
	 * Constructor of the XmlElementHandler structure.
	 */
	XmlElementHandler(const char *name, const std::function<bool()> &handler,
			int maxCount = -1, const XmlElementHandler *requiredElement = nullptr) :
		name(name), handler(handler), count(0), maxCount(maxCount),
		requiredElement(requiredElement)
	{
		// Do nothing here
	}

	/**
	 * Returns the name of the handler.
	 */
	const char* getName() const
	{
		return name;
	}

	/**
	 * Returns true if this handler is currently valid.
	 */
	bool valid() const
	{
		return ((maxCount < 0) || (count < maxCount))
			&& (!requiredElement || (requiredElement->count > 0));
	}

	/**
	 * Returns true if this handler matches the current state of the given
	 * QXmlStreamReader.
	 */
	template<typename StrType>
	bool matches(const StrType &tagName) const
	{
		return valid() && (tagName == name);
	}

	/**
	 * Executes the given handler.
	 */
	bool execute()
	{
		count++;
		return handler();
	}

	/**
	 * Function which assembles a string containing the names of the expected
	 * element types. Used for displaying error messages.
	 */
	static std::string expectedElementsStr(const std::vector<XmlElementHandler> &handlers)
	{
		// Calculate a list of valid element handlers
		std::vector<const XmlElementHandler*> validHandlers;
		for (auto &h : handlers) {
			if (h.valid()) {
				validHandlers.push_back(&h);
			}
		}

		// Assemble the string containing the list of expected elements
		std::stringstream ss;
		bool first = true;
		for (auto &h : validHandlers) {
			if (!first) {
				ss << ", ";
			}
			ss << h->getName();
			first = false;
		}
		return ss.str();
	}

};

}
}

#endif /* _OUSIA_XML_XML_ELEMENT_HANDLER_HPP_ */

