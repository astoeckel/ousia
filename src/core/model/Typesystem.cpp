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

bool Type::build(Variant &var, Logger &logger) const
{
	try {
		return doBuild(var, logger);
	}
	catch (LoggableException ex) {
		logger.log(ex);
		var = create();
		return false;
	}
}

/* Class StringType */

bool StringType::doBuild(Variant &var, Logger &logger) const
{
	// Cannot convert non-primitive values to strings
	if (!var.isPrimitive()) {
		throw LoggableException{"Expected a string or primitive input."};
	}

	// Perform an implicit type conversion
	if (!var.isString() || var.isMagic()) {
		// Convert the variant value to a string and set it
		var = var.toString().c_str();

		// Log conversions as these may be potentially unwanted
		logger.note(std::string("Implicit conversion from ") +
		            var.getTypeName() + " to string.");
	}
	return true;
}

/* Class IntType */

bool IntType::doBuild(Variant &var, Logger &logger) const
{
	if (!var.isInt()) {
		throw LoggableException{"Expected an integer value."};
	}
	return true;
}

/* Class DoubleType */

bool DoubleType::doBuild(Variant &var, Logger &logger) const
{
	if (!var.isInt() && !var.isDouble()) {
		throw LoggableException{"Expected a double value."};
	}
	var = Variant{var.toDouble()};
	return true;
}

/* Class BoolType */

bool BoolType::doBuild(Variant &var, Logger &logger) const
{
	if (!var.isBool()) {
		throw LoggableException("Expected boolean value!");
	}
	return true;
}

/* Class EnumType */

bool EnumType::doBuild(Variant &var, Logger &logger) const
{
	// If the variant is an int, check whether the value is in range
	if (var.isInt()) {
		int i = var.asInt();
		if (i < 0 || i >= (int)values.size()) {
			throw LoggableException("Value is out of range.");
		}
		return true;
	}

	// If the given variant is a magic value it may be an enumeration constant.
	// Set the variant to the numeric value
	if (var.isMagic()) {
		// Fetch the given constant name and look it up in the value map
		const std::string &name = var.asMagic();
		auto it = values.find(name);

		// Throw an execption if the given string value is not found
		if (it == values.end()) {
			throw LoggableException(std::string("Unknown enum constant: \"") +
			                        name + std::string("\""));
		}
		var = it->second;
		return true;
	}
	throw LoggableException{"Expected integer or identifier"};
}

Rooted<EnumType> EnumType::createValidated(Manager &mgr, std::string name,
                                   Handle<Typesystem> system,
                                   const std::vector<std::string> &values,
                                   Logger &logger)
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

/* Class ArrayType */

bool ArrayType::doBuild(Variant &var, Logger &logger) const
{
	if (!var.isArray()) {
		throw LoggableException("Expected array!");
	}
	bool res = true;
	for (auto &v : var.asArray()) {
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

