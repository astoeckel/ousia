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

#include "Typesystem.hpp"

#include <core/common/Rtti.hpp>
#include <core/common/Utils.hpp>

namespace ousia {
namespace model {

/* Class Type */

bool Type::build(Variant &data, Logger &logger) const
{
	try {
		return doBuild(data, logger);
	}
	catch (LoggableException ex) {
		logger.log(ex);
		data = create();
		return false;
	}
}

/* Class StringType */

bool StringType::doBuild(Variant &data, Logger &logger) const
{
	// Cannot convert non-primitive values to strings
	if (!data.isPrimitive()) {
		throw LoggableException{"Expected a string or primitive input."};
	}

	// Perform an implicit type conversion
	if (!data.isString() || data.isMagic()) {
		// Convert the variant value to a string and set it
		data = data.toString().c_str();

		// Log conversions as these may be potentially unwanted
		logger.note(std::string("Implicit conversion from ") +
		            data.getTypeName() + " to string.");
	}
	return true;
}

/* Class IntType */

bool IntType::doBuild(Variant &data, Logger &logger) const
{
	if (!data.isInt()) {
		throw LoggableException{"Expected an integer value."};
	}
	return true;
}

/* Class DoubleType */

bool DoubleType::doBuild(Variant &data, Logger &logger) const
{
	if (!data.isInt() && !data.isDouble()) {
		throw LoggableException{"Expected a double value."};
	}
	data = Variant{data.toDouble()};
	return true;
}

/* Class BoolType */

bool BoolType::doBuild(Variant &data, Logger &logger) const
{
	if (!data.isBool()) {
		throw LoggableException("Expected boolean value!");
	}
	return true;
}

/* Class EnumType */

bool EnumType::doBuild(Variant &data, Logger &logger) const
{
	// If the variant is an int, check whether the value is in range
	if (data.isInt()) {
		int i = data.asInt();
		if (i < 0 || i >= (int)values.size()) {
			throw LoggableException("Value is out of range.");
		}
		return true;
	}

	// If the given variant is a magic value it may be an enumeration constant.
	// Set the variant to the numeric value
	if (data.isMagic()) {
		// Fetch the given constant name and look it up in the value map
		const std::string &name = data.asMagic();
		auto it = values.find(name);

		// Throw an execption if the given string value is not found
		if (it == values.end()) {
			throw LoggableException(std::string("Unknown enum constant: \"") +
			                        name + std::string("\""));
		}
		data = it->second;
		return true;
	}
	throw LoggableException{"Expected integer or identifier"};
}

Rooted<EnumType> EnumType::createValidated(
    Manager &mgr, std::string name, Handle<Typesystem> system,
    const std::vector<std::string> &values, Logger &logger)
{
	// Map used to store the unique values of the enum
	std::map<std::string, Ordinal> unique_values;

	// The given vector may not be empty
	if (values.empty()) {
		logger.error("Enumeration constants may not be empty.");
	}

	// Iterate over the input vector, check the constant names for validity and
	// uniqueness and insert them into the internal values map
	for (size_t i = 0; i < values.size(); i++) {
		if (!Utils::isIdentifier(values[i])) {
			logger.error(values[i] + " is no valid identifier.");
		}

		if (!(unique_values.insert(std::make_pair(values[i], i))).second) {
			logger.error(std::string("The value ") + values[i] +
			             " was duplicated.");
		}
	}
	return new EnumType{mgr, name, system, unique_values};
}

std::string EnumType::nameOf(Ordinal i) const
{
	if (i >= 0 && i < (int)values.size()) {
		for (const auto &v : values) {
			if (v.second == i) {
				return v.first;
			}
		}
	}
	throw LoggableException("Ordinal value out of range.");
}

EnumType::Ordinal EnumType::valueOf(const std::string &name) const
{
	auto it = values.find(name);
	if (it != values.end()) {
		return it->second;
	}
	throw LoggableException(std::string("Unknown enum constant: ") + name);
}

/* Class StructType */

bool StructType::resolveIndexKey(const std::string &key, size_t &idx) const
{
	try {
		idx = stoul(key.substr(1));
		return true;
	}
	catch (std::exception ex) {
		return false;
	}
}

bool StructType::resolveIdentifierKey(const std::string &key, size_t &idx) const
{
	auto it = attributeNames.find(key);
	if (it == attributeNames.end()) {
		return false;
	}
	idx = it->second;
	return true;
}

bool StructType::resolveKey(const std::string &key, size_t &idx) const
{
	bool res;
	if (!key.empty() && key[0] == '#') {
		res = resolveIndexKey(key, idx);
	} else {
		res = resolveIdentifierKey(key, idx);
	}
	return res && (idx < attributes.size());
}

bool StructType::insertDefaults(Variant &data, const std::vector<bool> &set,
                                Logger &logger) const
{
	bool ok = true;
	Variant::arrayType &arr = data.asArray();
	for (size_t a = 0; a < arr.size(); a++) {
		if (!set[a]) {
			if (attributes[a]->optional) {
				arr[a] = attributes[a]->defaultValue;
			} else {
				ok = false;
				arr[a] = attributes[a]->getType()->create();
				logger.error(std::string("Expected attribute ") +
				             attributes[a]->getName() +
				             std::string(", but no value given."));
			}
		}
	}
	return ok;
}

bool StructType::buildFromArray(Variant &data, Logger &logger, bool trim) const
{
	bool ok = true;
	Variant::arrayType &arr = data.asArray();
	std::vector<bool> set;

	// Fetch the size of the input array n and the number of attributes N
	const size_t n = arr.size();
	const size_t N = attributes.size();
	arr.resize(N);
	set.resize(N);

	// Make sure the array has the correct size
	if (n > N && !trim) {
		ok = false;
		logger.error(std::string("Expected at most ") + std::to_string(N) +
		             std::string(" attributes, but got ") + std::to_string(n));
	}

	// Make sure the given attributes have to correct type
	for (size_t a = 0; a < n; a++) {
		set[a] = attributes[a]->getType()->build(arr[a], logger);
		ok = ok && set[a];
	}

	return insertDefaults(data, set, logger) && ok;
}

bool StructType::buildFromMap(Variant &data, Logger &logger, bool trim) const
{
	bool ok = true;
	const Variant::mapType &map = data.asMap();
	Variant::arrayType arr;
	std::vector<bool> set;

	// Fetch the size of the input map n and the number of attributes N
	const size_t N = attributes.size();
	arr.resize(N);
	set.resize(N);

	// Iterate over the map entries
	for (auto &m : map) {
		// Fetch key and value
		const std::string &key = m.first;
		const Variant &value = m.second;

		// Lookup the key index
		size_t idx = 0;
		if (resolveKey(key, idx)) {
			// Warn about overriding the same key
			if (set[idx]) {
				logger.warning(
				    std::string("Attribute \"") + key +
				    std::string("\" set multiple times, overriding!"));
			}

			// Convert the value to the type of the attribute
			arr[idx] = value;
			set[idx] = attributes[idx]->getType()->build(arr[idx], logger);
		} else if (!trim) {
			ok = false;
			logger.error(std::string("Invalid attribute key \"") + key +
			             std::string("\""));
		}
	}

	// Copy the built array to the result and insert missing default values
	data = arr;
	return insertDefaults(data, set, logger) && ok;
}

bool StructType::buildFromArrayOrMap(Variant &data, Logger &logger,
                                     bool trim) const
{
	if (data.isArray()) {
		return buildFromArray(data, logger, trim);
	}
	if (data.isMap()) {
		return buildFromMap(data, logger, trim);
	}
	throw LoggableException(
	    "Expected array or map for building a struct type!");
}

bool StructType::doBuild(Variant &data, Logger &logger) const
{
	return buildFromArrayOrMap(data, logger, false);
}

Rooted<StructType> StructType::createValidated(
    Manager &mgr, std::string name, Handle<Typesystem> system,
    Handle<StructType> parent, NodeVector<Attribute> attributes, Logger &logger)
{
	// Check the attributes for validity and uniqueness
	std::map<std::string, size_t> attributeNames;
	for (size_t idx = 0; idx < attributes.size(); idx++) {
		// Check for valid attribute names
		const std::string &attrName = attributes[idx]->getName();
		if (!Utils::isIdentifier(name)) {
			logger.error(std::string("Invalid attribute name \"") + name +
			             std::string("\""));
		}

		// Check for uniqueness
		auto res = attributeNames.emplace(attrName, idx);
		if (!res.second) {
			logger.error(std::string("Attribute with name \"") + name +
			             std::string("\" defined multiple times"));
		}
	}

	// Call the private constructor
	return new StructType(mgr, name, system, parent, attributes, attributeNames);
}

Variant StructType::create() const
{
	Variant::arrayType arr;
	arr.resize(attributes.size());
	for (size_t idx = 0; idx < attributes.size(); idx++) {
		arr[idx] = attributes[idx]->getType()->create();
	}
	return arr;
}

bool StructType::derivedFrom(Handle<StructType> other) const
{
	if (other == this) {
		return true;
	}
	if (parent != nullptr) {
		return parent->derivedFrom(other);
	}
	return false;
}

Variant StructType::cast(Variant &data, Logger &logger) const
{
	return buildFromArrayOrMap(data, logger, true);
}

/* Class ArrayType */

bool ArrayType::doBuild(Variant &data, Logger &logger) const
{
	if (!data.isArray()) {
		throw LoggableException("Expected array!");
	}
	bool res = true;
	for (auto &v : data.asArray()) {
		if (!innerType->build(v, logger)) {
			res = false;
		}
	}
	return res;
}
}

/* RTTI type registrations */

namespace RttiTypes {
const Rtti<model::Type> Type{"Type", {&Node}};
const Rtti<model::StringType> StringType{"StringType", {&Type}};
const Rtti<model::IntType> IntType{"IntType", {&Type}};
const Rtti<model::DoubleType> DoubleType{"DoubleType", {&Type}};
const Rtti<model::BoolType> BoolType{"BoolType", {&Type}};
const Rtti<model::EnumType> EnumType{"EnumType", {&Type}};
const Rtti<model::StructType> StructType{"StructType", {&Type}};
const Rtti<model::ArrayType> ArrayType{"ArrayType", {&Type}};
const Rtti<model::UnknownType> UnknownType{"UnknownType", {&Type}};
const Rtti<model::Constant> Constant{"Constant", {&Node}};
const Rtti<model::Typesystem> Typesystem{"Typesystem", {&Node}};
}
}

