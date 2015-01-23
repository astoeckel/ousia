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

#include "Exceptions.hpp"
#include "Rtti.hpp"

namespace ousia {

/* Class RttiStore */

std::unordered_map<std::type_index, const Rtti *> &RttiStore::table()
{
	static std::unordered_map<std::type_index, const Rtti *> table;
	return table;
}

void RttiStore::store(const std::type_info &native, const Rtti *rtti)
{
	table().emplace(std::type_index{native}, rtti);
}

const Rtti &RttiStore::lookup(const std::type_info &native)
{
	const auto &tbl = table();
	auto it = tbl.find(std::type_index{native});
	if (it == tbl.end()) {
		return RttiTypes::None;
	} else {
		return *(it->second);
	}
}

/* Class RttiBuilderBase */

RttiBuilderBase &RttiBuilderBase::genericMethod(
    const std::string &name, std::shared_ptr<Function> function)
{
	if (!methods.emplace(name, function).second) {
		throw OusiaException(std::string("Method with name \"") + name +
		                     std::string("\" for type \"") + currentName +
		                     std::string("\" already registered!"));
	}
	return *this;
}

RttiBuilderBase &RttiBuilderBase::genericProperty(
    const std::string &name, std::shared_ptr<PropertyDescriptor> property)
{
	if (!properties.emplace(name, property).second) {
		throw OusiaException(std::string("Property with name \"") + name +
		                     std::string("\" for type \"") + currentName +
		                     std::string("\" already registered!"));
	}
	return *this;
}

/* Class Rtti */

void Rtti::initialize() const
{
	// Only run this function exactly once -- directly set the initialized flag
	// to prevent unwanted recursion
	if (!initialized) {
		initialized = true;

		// Register the parent properties and methods
		{
			for (const Rtti *parent : parents) {
				parent->initialize();
				methods.insert(parent->methods.begin(), parent->methods.end());
				properties.insert(parent->properties.begin(),
				                  parent->properties.end());
			}
		}

		// Insert the parent types of the parent types and the composite types
		// of the parents
		{
			std::unordered_set<const Rtti *> origParents = parents;
			for (const Rtti *parent : origParents) {
				parent->initialize();
				parents.insert(parent->parents.begin(), parent->parents.end());
			}
			for (const Rtti *parent : parents) {
				parent->initialize();
				compositeTypes.insert(parent->compositeTypes.begin(),
				                      parent->compositeTypes.end());
			}
			parents.insert(this);
		}

		// Insert the composite types of the composite types and the parents
		// of each composite type
		{
			std::unordered_set<const Rtti *> origCompositeTypes =
			    compositeTypes;
			for (const Rtti *compositeType : origCompositeTypes) {
				compositeType->initialize();
				compositeTypes.insert(compositeType->compositeTypes.begin(),
				                      compositeType->compositeTypes.end());
				compositeTypes.insert(compositeType->parents.begin(),
				                      compositeType->parents.end());
			}
		}
	}
}

bool Rtti::isa(const Rtti &other) const
{
	initialize();
	return parents.count(&other) > 0;
}

bool Rtti::isOneOf(const RttiSet &others) const
{
	initialize();
	for (const Rtti *other : others) {
		if (parents.count(other) > 0) {
			return true;
		}
	}
	return false;
}

bool Rtti::setIsOneOf(const RttiSet &s1, const RttiSet &s2)
{
	for (const Rtti *t1 : s1) {
		if (t1->isOneOf(s2)) {
			return true;
		}
	}
	return false;
}

bool Rtti::composedOf(const Rtti &other) const
{
	initialize();
	return compositeTypes.count(&other) > 0;
}

const RttiMethodMap &Rtti::getMethods() const
{
	initialize();
	return methods;
}

const RttiPropertyMap &Rtti::getProperties() const
{
	initialize();
	return properties;
}

std::shared_ptr<Function> Rtti::getMethod(const std::string &name) const
{
	initialize();
	auto it = methods.find(name);
	if (it == methods.end()) {
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<PropertyDescriptor> Rtti::getProperty(
    const std::string &name) const
{
	initialize();
	auto it = properties.find(name);
	if (it == properties.end()) {
		return nullptr;
	}
	return it->second;
}

bool Rtti::hasMethod(const std::string &name) const
{
	return methods.count(name) > 0;
}

bool Rtti::hasProperty(const std::string &name) const
{
	return properties.count(name) > 0;
}

/* Constant initialization */

namespace RttiTypes {
const Rtti None{"none"};
const Rtti Nullptr{"nullptr"};
const Rtti Bool{"bool"};
const Rtti Int{"int"};
const Rtti Double{"double"};
const Rtti String{"string"};
const Rtti Array{"array"};
const Rtti Map{"map"};
const Rtti Function{"function"};
}
}

