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

#include "Object.hpp"

namespace ousia {
namespace script {

bool Object::hasElement(std::string name) const
{
	return (properties.find(name) != properties.end()) ||
	       (methods.find(name) != methods.end());
}

void Object::addProperty(std::string name, const Property &property)
{
	if (hasElement(name)) {
		// TODO Throw another exception class here
		throw "Element already exists";
	}
	properties.emplace(name, property);
}

void Object::addProperty(std::string name, const Getter &get, const Setter &set)
{
	addProperty(name, Property{get, set});
}

void Object::addProperty(std::string name, VariantType type,
                         const GetterCallback get, const SetterCallback set)
{
	addProperty(name, Property{type, get, set, data});
}

void Object::addReadonlyProperty(std::string name, const Getter &get)
{
	addProperty(name, Property{get, Setter{VariantType::null, nullptr}});
}

void Object::addReadonlyProperty(std::string name, const GetterCallback get)
{
	addProperty(
	    name, Property{Getter{get, data}, Setter{VariantType::null, nullptr}});
}

void Object::addMethod(std::string name, const HostFunction &fun)
{
	if (hasElement(name)) {
		// TODO Throw another exception class here
		throw "Element already exists";
	}
	methods.emplace(name, fun);
}

void Object::addMethod(std::string name, const HostFunctionCallback fun)
{
	addMethod(name, HostFunction{fun, data});
}

void Object::addMethod(std::string name, const HostFunctionCallback fun,
                       const std::vector<Argument> &signature)
{
	addMethod(name, HostFunction{fun, signature, data});
}

}
}

