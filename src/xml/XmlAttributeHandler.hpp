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

#ifndef _OUSIA_XML_XML_ATTRIBUTE_HANDLER_HPP_
#define _OUSIA_XML_XML_ATTRIBUTE_HANDLER_HPP_

#include <functional>
#include <string>

namespace ousia {
namespace xml {

/**
 * The attribute handler class is responsible for handling attributes. It
 * performs type checks and conversion. Note that the name of the attribute is
 * not stored inside the handler, as the attribute handlers are meant to be used
 * alongside a map.
 */
class XmlAttributeHandler {

private:
	/**
	 * Specifies whether this attribute was actually handled (set to true once
	 * the setter is called).
	 */
	bool handled;

	/**
	 * Specifies whether this attribute is required or not.
	 */
	bool required;

	/**
	 * Function which returns true if the given string is a valid entry for the
	 * type the attribute handler represents.
	 */
	std::function<bool(const std::string&)> valid;

	/**
	 * Function which gets the attribute value and sets the type.
	 */
	std::function<void(const std::string&)> setter;

	/**
	 * Default value (as string) that should be used if no other value for that
	 * attribute is given.
	 */
	const char *defaultValue;

public:

	/**
	 * Constructor of the XmlAttributeHandler class.
	 *
	 * @param required if true, the attribute is marked as "required" and it
	 * must occur in the xml.
	 * @param valid is a function reference which specifies whether the given
	 * string is valid.
	 * @param setter is the function that is meant to actually set the value
	 * of the attached class.
	 * @param defaultValue if given (it does not equal the nullptr), the setter
	 * is automatically called with the default value, unless the attribute is
	 * actually specified in the XML.
	 */
	XmlAttributeHandler(bool required,
			const std::function<bool(const std::string&)> &valid,
			const std::function<void(const std::string&)> &setter,
			const char *defaultValue = nullptr) :
		handled(false), required(required), valid(valid), setter(setter),
		defaultValue(defaultValue)
	{
		// Do nothing here
	}

	/**
	 * Returns true if the given value for this attribute is valid.
	 */
	bool isValid(const std::string &value)
	{
		return valid(value);
	}

	/**
	 * Calls the setter with the given value. The value should have been checked
	 * for validity first.
	 */
	void executeSettter(const std::string &value)
	{
		handled = true;
		setter(value);
	}

	/**
	 * Returns true if this element is required.
	 */
	bool isRequired()
	{
		return required;
	}

	/**
	 * Returns the default value.
	 */
	const char* getDefaultValue()
	{
		return defaultValue;
	}

	/**
	 * Returns true if the attribute was handled.
	 */
	bool isHandled()
	{
		return handled;
	}

};

}
}

#endif /* _OUSIA_XML_XML_ATTRIBUTE_HANDLER_HPP_ */

