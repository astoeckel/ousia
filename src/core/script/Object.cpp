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

bool Object::hasElement(const std::string &name) const
{
	return (properties.find(name) != properties.end()) ||
	       (methods.find(name) != methods.end());
}

void Object::addProperty(const std::string &name, const Property &property)
{
	if (hasElement(name)) {
		// TODO Throw another exception class here
		throw "Element already exists";
	}
	properties.insert(std::make_pair(name, property));
}

void Object::addProperty(const std::string &name, const Getter &get, const Setter &set)
{
	addProperty(name, Property{get, set});
}

void Object::addProperty(const std::string &name, VariantType type,
                         const GetterCallback get, const SetterCallback set)
{
	addProperty(name, Property{type, get, set, data});
}

void Object::addReadonlyProperty(const std::string &name, const Getter &get)
{
	addProperty(name, Property{get, Setter{VariantType::null, nullptr}});
}

void Object::addReadonlyProperty(const std::string &name, const GetterCallback get)
{
	addProperty(
	    name, Property{Getter{get, data}, Setter{VariantType::null, nullptr}});
}

void Object::addMethod(const std::string &name, const HostFunction &fun)
{
	if (hasElement(name)) {
		// TODO Throw another exception class here
		throw "Element already exists";
	}
	methods.insert(std::make_pair(name, fun));
}

void Object::addMethod(const std::string &name, const HostFunctionCallback fun)
{
	addMethod(name, HostFunction{fun, data});
}

void Object::addMethod(const std::string &name, const HostFunctionCallback fun,
                       const std::vector<Argument> &signature)
{
	addMethod(name, HostFunction{fun, signature, data});
}

const Property *Object::getProperty(const std::string &name) const
{
	auto it = properties.find(name);
	return (it != properties.end()) ? &(it->second) : nullptr;
}

const Function *Object::getMethod(const std::string &name) const
{
	auto it = methods.find(name);
	return (it != methods.end()) ? &(it->second) : nullptr;
}

bool Object::removeElement(const std::string &name)
{
	return removeProperty(name) || removeMethod(name);
}

bool Object::removeProperty(const std::string &name)
{
	return properties.erase(name) > 0;
}

bool Object::removeMethod(const std::string &name)
{
	return methods.erase(name) > 0;
}

}
}

