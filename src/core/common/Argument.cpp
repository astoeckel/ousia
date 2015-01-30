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

#include "Argument.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"
#include "Rtti.hpp"
#include "Utils.hpp"
#include "VariantConverter.hpp"

namespace ousia {

/* Class Argument */

Argument::Argument(std::string name, const Rtti &type, const Rtti &innerType,
                   Variant defaultValue, bool hasDefault)
    : type(type),
      innerType(innerType),
      name(std::move(name)),
      defaultValue(std::move(defaultValue)),
      hasDefault(hasDefault)
{
}

Argument::Argument(std::string name, const Rtti &type, Variant defaultValue)
    : Argument(std::move(name), type, RttiTypes::None, defaultValue, true)
{
}

Argument::Argument(std::string name, const Rtti &type)
    : Argument(std::move(name), type, RttiTypes::None, nullptr, false)
{
}

Argument Argument::Any(std::string name)
{
	return Argument{name, RttiTypes::None, RttiTypes::None, nullptr, false};
}

Argument Argument::Any(std::string name, Variant defaultValue)
{
	return Argument{name, RttiTypes::None, RttiTypes::None, defaultValue, true};
}

Argument Argument::Bool(std::string name)
{
	return Argument{name, RttiTypes::Bool};
}

Argument Argument::Bool(std::string name, Variant::boolType defaultValue)
{
	return Argument{name, RttiTypes::Bool, defaultValue};
}

Argument Argument::Int(std::string name)
{
	return Argument{name, RttiTypes::Int};
}

Argument Argument::Int(std::string name, Variant::intType defaultValue)
{
	return Argument{name, RttiTypes::Int, defaultValue};
}

Argument Argument::Double(std::string name)
{
	return Argument{name, RttiTypes::Double};
}

Argument Argument::Double(std::string name, Variant::doubleType defaultValue)
{
	return Argument{name, RttiTypes::Double, defaultValue};
}

Argument Argument::String(std::string name)
{
	return Argument{name, RttiTypes::String};
}

Argument Argument::String(std::string name,
                          const Variant::stringType &defaultValue)
{
	return Argument{name, RttiTypes::String, Variant::fromString(defaultValue)};
}

Argument Argument::Object(std::string name, const Rtti &type)
{
	return Argument(std::move(name), type, RttiTypes::None,
	                Variant::fromObject(nullptr), false);
}

Argument Argument::Object(std::string name, const Rtti &type, std::nullptr_t)
{
	return Argument(std::move(name), type, RttiTypes::None,
	                Variant::fromObject(nullptr), true);
}

Argument Argument::Function(std::string name)
{
	return Argument(std::move(name), RttiTypes::Function);
}

Argument Argument::Function(std::string name,
                            Variant::functionType defaultValue)
{
	return Argument(std::move(name), RttiTypes::Function,
	                Variant::fromFunction(defaultValue));
}

Argument Argument::Array(std::string name)
{
	return Argument(std::move(name), RttiTypes::Array);
}

Argument Argument::Array(std::string name,
                         const Variant::arrayType &defaultValue)
{
	return Argument(std::move(name), RttiTypes::Array, defaultValue);
}

Argument Argument::Array(std::string name, const Rtti &innerType)
{
	return Argument(std::move(name), RttiTypes::Array, innerType, nullptr,
	                false);
}

Argument Argument::Array(std::string name, const Rtti &innerType,
                         const Variant::arrayType &defaultValue)
{
	return Argument(std::move(name), RttiTypes::Array, innerType, defaultValue,
	                true);
}

Argument Argument::Map(std::string name)
{
	return Argument(std::move(name), RttiTypes::Map);
}

Argument Argument::Map(std::string name, const Variant::mapType &defaultValue)
{
	return Argument(std::move(name), RttiTypes::Map, defaultValue);
}

Argument Argument::Map(std::string name, const Rtti &innerType)
{
	return Argument(std::move(name), RttiTypes::Map, innerType, nullptr, false);
}

Argument Argument::Map(std::string name, const Rtti &innerType,
                       const Variant::mapType &defaultValue)
{
	return Argument(std::move(name), RttiTypes::Map, innerType, defaultValue,
	                true);
}

Argument Argument::Cardinality(std::string name)
{
	return Argument{name, RttiTypes::Cardinality};
}

Argument Argument::Cardinality(std::string name,
                               Variant::cardinalityType defaultValue)
{
	return Argument{name, RttiTypes::Cardinality, defaultValue};
}

bool Argument::validate(Variant &var, Logger &logger) const
{
	if (!VariantConverter::convert(var, type, innerType, logger,
	                               VariantConverter::Mode::SAFE)) {
		if (hasDefault) {
			var = defaultValue;
		}
		return false;
	}
	return true;
}

/* Class Arguments */

// Instantiations of the "None" arguments
const Arguments Arguments::None;

static std::unordered_map<std::string, size_t> buildArgumentNames(
    std::initializer_list<Argument> arguments)
{
	// Make sure the name is unique
	std::unordered_map<std::string, size_t> res;
	size_t i = 0;
	for (const Argument &arg : arguments) {
		if (!Utils::isIdentifier(arg.name)) {
			throw OusiaException{std::string("Argument name ") + arg.name +
			                     std::string(" is not a valid identifier")};
		}
		if (!res.emplace(arg.name, i++).second) {
			throw OusiaException{
			    std::string("Argument names must be unique (") + arg.name +
			    std::string(")")};
		}
	}
	return res;
}

Arguments::Arguments(std::initializer_list<Argument> arguments)
    : arguments(arguments), names(buildArgumentNames(arguments)), valid(true)
{
}

bool Arguments::validateArray(Variant::arrayType &arr, Logger &logger) const
{
	// Abort if no arguments were explicitly given -- everything is valid
	if (!valid) {
		return true;
	}

	Logger nullLogger;

	// Fetch the number of arguments N and the initial array size n
	const size_t n = arr.size();
	const size_t N = arguments.size();
	bool ok = true;

	// Make sure the argument list is not too long
	if (n > N) {
		ok = false;
		logger.error(std::string("Too many arguments: expected ") +
		             std::to_string(N) + std::string(" arguments, but got ") +
		             std::to_string(n));
	}

	// Resize the array to the total number of elements
	arr.resize(N);

	// Make sure the given attributes have to correct type, insert default
	// values
	for (size_t a = 0; a < N; a++) {
		if (a < n) {
			ok = ok && arguments[a].validate(arr[a], logger);
		} else {
			if (arguments[a].hasDefault) {
				arr[a] = arguments[a].defaultValue;
			} else {
				// Call "validate" to inject a standard value
				arr[a] = Variant::fromObject(nullptr);
				arguments[a].validate(arr[a], nullLogger);
				logger.error(std::string("Missing argument ") +
				             std::to_string(a + 1) + std::string(" \"") +
				             arguments[a].name + std::string("\""));
				ok = false;
			}
		}
	}

	return ok;
}

bool Arguments::validateMap(Variant::mapType &map, Logger &logger,
                            bool ignoreUnknown) const
{
	// Abort if no arguments were explicitly given -- everything is valid
	if (!valid) {
		return true;
	}

	Logger nullLogger;

	// Fetch the number of arguments N
	const size_t N = arguments.size();
	std::vector<bool> set(N);
	bool ok = true;

	// Iterate over the map entries and search for the corresponding argument
	for (auto &e : map) {
		// Check whether an argument with the name of the current entry exists
		auto it = names.find(e.first);
		if (it != names.end()) {
			// Fetch the corresponding index in the "arguments" array
			size_t idx = it->second;
			set[idx] = arguments[idx].validate(e.second, logger);
			ok = ok && set[idx];
		} else {
			if (ignoreUnknown) {
				logger.note(std::string("Ignoring argument \"") + e.first +
				            std::string("\""));
			} else {
				logger.error(std::string("Unknown argument \"") + e.first +
				             std::string("\""));
				ok = false;
			}
		}
	}

	// Insert all unset arguments
	for (size_t a = 0; a < N; a++) {
		if (!set[a]) {
			if (arguments[a].hasDefault) {
				map[arguments[a].name] = arguments[a].defaultValue;
			} else {
				// Call "validate" to inject a standard value
				map[arguments[a].name] = Variant::fromObject(nullptr);
				arguments[a].validate(map[arguments[a].name], nullLogger);
				logger.error(std::string("Missing argument \"") +
				             arguments[a].name + std::string("\""));
				ok = false;
			}
		}
	}

	return ok;
}
}

