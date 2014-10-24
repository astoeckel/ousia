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

#ifndef _OBJECT_HPP_
#define _OBJECT_HPP_

#include <string>
#include <map>

#include "Function.hpp"

namespace ousia {
namespace script {

/**
 * The Property struct represents an object property with corresponding getter
 * and setter function.
 */
struct Property {
	/**
	 * Constructor of the Property struct. Copies the given getter and setter.
	 *
	 * @param get is the getter that should be used for the property.
	 * @param set is the setter that should be used for the property.
	 */
	Property(const Getter &get, const Setter &set) : get(get), set(set){};

	/**
	 * Constructor of the Property struct. Creates new Getter and Setter
	 * instances from the given parameters.
	 *
	 * @param type is the VariantType used within the getter function.
	 * @param get is the pointer to the getter function.
	 * @param set is the pointer to the setter function.
	 * @param data is the used-defined data that should be used.
	 */
	Property(VariantType type, const GetterCallback get,
	         const SetterCallback set, void *data = nullptr)
	    : get(get, data), set(type, set, data){};

	/**
	 * Getter function.
	 */
	const Getter get;

	/**
	 * Setter function.
	 */
	const Setter set;
};

/**
 * The Object type represents an object on the script host. An object consits of
 * properties with corresponding getter and setter functions and a number of
 * methods which can be called on the object.
 */
class Object {
private:
	/**
	 * Pointer to user defined data that is automatically passed to the
	 * underlying functions.
	 */
	void *data;

	/**
	 * Map used internally for storing all properties along with their
	 * corresponding
	 * name.
	 */
	std::map<std::string, Property> properties;

	/**
	 * Map used internally for storing all methods along with their
	 * corresponding name.
	 */
	std::map<std::string, HostFunction> methods;

public:
	Object() : data(nullptr){};

	Object(void *data) : data(data){};

	bool hasElement(std::string name) const;

	void addProperty(std::string name, const Property &property);

	void addProperty(std::string name, const Getter &get, const Setter &set);

	void addProperty(std::string name, VariantType type,
	                 const GetterCallback get, const SetterCallback set);

	void addReadonlyProperty(std::string name, const Getter &get);

	void addReadonlyProperty(std::string name, const GetterCallback get);

	void addMethod(std::string name, const HostFunction &fun);

	void addMethod(std::string name, const HostFunctionCallback fun);

	void addMethod(std::string name, const HostFunctionCallback fun,
	               const std::vector<Argument> &signature);

	const std::map<std::string, Property> &getProperties()
	{
		return properties;
	}

	const std::map<std::string, HostFunction> &getMethods()
	{
		return methods;
	}
};
}
}

#endif /* _OBJECT_HPP_ */

