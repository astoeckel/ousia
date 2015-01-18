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

#include <cstdint>
#include <memory>
#include <string>
#include <sstream>

#include "Function.hpp"
#include "Logger.hpp"
#include "Number.hpp"
#include "Rtti.hpp"
#include "Variant.hpp"
#include "VariantConverter.hpp"
#include "VariantWriter.hpp"

namespace ousia {

static std::string msgUnexpectedType(const Variant &v,
                                     VariantType requestedType)
{
	return std::string("Cannot convert ") + v.getTypeName() + std::string(" (") +
	       VariantWriter::writeJsonToString(v, false) + std::string(") to ") +
	       Variant::getTypeName(requestedType);
}

static std::string msgImplicitConversion(VariantType actualType,
                                         VariantType requestedType)
{
	return std::string("Implicit conversion from ") +
	       Variant::getTypeName(actualType) + std::string(" to ") +
	       Variant::getTypeName(requestedType);
}

bool VariantConverter::toBool(Variant &var, Logger &logger, Mode mode)
{
	// Perform safe conversions
	const VariantType type = var.getType();
	switch (type) {
		case VariantType::BOOL:
			// No conversion needed if "var" already is a boolean
			return true;
		default:
			break;
	}

	// Perform potentially dangerous conversions in the "ALL" mode
	if (mode == Mode::ALL) {
		switch (var.getType()) {
			case VariantType::NULLPTR:
				var = false;
				return true;
			case VariantType::INT:
				var = var.asInt() != 0;
				return true;
			case VariantType::DOUBLE:
				var = var.asDouble() != 0.0;
				return true;
			default:
				var = true;
				return true;
		}
	}

	// No conversion possible, assign default value and log error
	logger.error(msgUnexpectedType(var, VariantType::BOOL));
	var = false;
	return false;
}

bool VariantConverter::toInt(Variant &var, Logger &logger, Mode mode)
{
	// Perform safe conversions
	const VariantType type = var.getType();
	switch (type) {
		case VariantType::INT:
			// No conversion needed if "var" already is an integer
			return true;
		default:
			break;
	}

	// Perform all potentially dangerous conversions in the "ALL" mode
	if (mode == Mode::ALL) {
		switch (type) {
			case VariantType::NULLPTR:
				var = 0;
				return true;
			case VariantType::BOOL:
				var = var.asBool() ? 1 : 0;
				return true;
			case VariantType::DOUBLE:
				var = (Variant::intType)var.asDouble();
				return true;
			case VariantType::STRING:
			case VariantType::MAGIC: {
				Number n;
				n.parse(var.asString(), logger);
				if (n.isInt()) {
					var = (Variant::intType)n.intValue();
					return true;
				}
				break;
			}
			case VariantType::ARRAY: {
				try {
					// JavaScript behaviour when converting arrays to doubles
					const Variant::arrayType &a = var.asArray();
					var = (a.size() == 1) ? a[0].toInt() : 0.0;
					return true;
				}
				catch (LoggableException ex) {
					logger.log(ex);
				}
			}
			default:
				break;
		}
	}

	// No conversion possible, assign default value and log error
	logger.error(msgUnexpectedType(var, VariantType::INT));
	var = 0;
	return false;
}

bool VariantConverter::toDouble(Variant &var, Logger &logger, Mode mode)
{
	// Perform safe conversions
	const VariantType type = var.getType();
	switch (type) {
		case VariantType::DOUBLE:
			// No conversion needed if "var" already is a double
			return true;
		case VariantType::INT:
			// Converting integers to doubles is safe
			var = (Variant::doubleType)var.asInt();
			return true;
		default:
			break;
	}

	// Perform all potentially dangerous conversions in the "ALL" mode
	if (mode == Mode::ALL) {
		switch (type) {
			case VariantType::NULLPTR:
				var = 0.0;
				return true;
			case VariantType::BOOL:
				var = var.asBool() ? 1.0 : 0.0;
				return true;
			case VariantType::STRING:
			case VariantType::MAGIC: {
				Number n;
				n.parse(var.asString(), logger);
				var = (Variant::doubleType)n.doubleValue();
				return true;
			}
			case VariantType::ARRAY: {
				try {
					// JavaScript behaviour when converting arrays to doubles
					const Variant::arrayType &a = var.asArray();
					var = (a.size() == 1) ? a[0].toDouble() : 0.0;
					return true;
				}
				catch (LoggableException ex) {
					logger.log(ex);
				}
			}
			default:
				break;
		}
	}

	// No conversion possible, assign default value and log error
	logger.error(msgUnexpectedType(var, VariantType::DOUBLE));
	var = 0.0;
	return false;
}

bool VariantConverter::toString(Variant &var, Logger &logger, Mode mode)
{
	// Perform safe conversions (all these operations are considered "lossless")
	const VariantType type = var.getType();
	switch (type) {
		case VariantType::NULLPTR:
			logger.warning(msgImplicitConversion(type, VariantType::STRING));
			var = "null";
			return true;
		case VariantType::BOOL:
			logger.warning(msgImplicitConversion(type, VariantType::STRING));
			var = var.asBool() ? "true" : "false";
			return true;
		case VariantType::INT: {
			logger.warning(msgImplicitConversion(type, VariantType::STRING));
			std::stringstream ss;
			ss << var.asInt();
			var = ss.str().c_str();
			return true;
		}
		case VariantType::DOUBLE: {
			logger.warning(msgImplicitConversion(type, VariantType::STRING));
			std::stringstream ss;
			ss << var.asDouble();
			var = ss.str().c_str();
			return true;
		}
		case VariantType::MAGIC:
		case VariantType::STRING:
			// No conversion needed if "var" already is a string (or a magic
			// string value)
			return true;
		default:
			break;
	}

	// Perform lossy conversions
	if (mode == Mode::ALL) {
		switch (type) {
			case VariantType::ARRAY:
			case VariantType::MAP: {
				std::stringstream ss;
				VariantWriter::writeJson(var, ss, false);
				var = ss.str().c_str();
				return true;
			}
			case VariantType::OBJECT: {
				// Print object address and type
				Variant::objectType obj = var.asObject();
				std::stringstream ss;
				ss << "<object " << obj.get();
				if (obj.get() != nullptr) {
					ss << " (" << obj->type().name << ")";
				}
				ss << ">";
				var = ss.str().c_str();
				return true;
			}
			case VariantType::FUNCTION: {
				// Print function pointer address
				Variant::functionType obj = var.asFunction();
				std::stringstream ss;
				ss << "<function " << obj.get() << ">";
				var = ss.str().c_str();
				return true;
			}
			default:
				break;
		}
	}

	// No conversion possible, assign default value and log error
	logger.error(msgUnexpectedType(var, VariantType::STRING));
	var = "";
	return false;
}

bool VariantConverter::toArray(Variant &var, const Rtti &innerType,
                               Logger &logger, Mode mode)
{
	// If unsafe conversions are allowed, encapsulate the given variant in an
	// array if it is not an array now.
	if (!var.isArray() && mode == Mode::ALL) {
		var.setArray(Variant::arrayType{var});
	}

	// Make sure the variant is an array
	if (var.isArray()) {
		// If no specific inner type is given, conversion is successful at this
		// point
		if (&innerType == &RttiTypes::None) {
			return true;
		}

		// Convert all entries of the array to the specified inner type, log all
		// failures to do so
		bool res = true;
		for (Variant &v : var.asArray()) {
			res = convert(v, innerType, RttiTypes::None, logger, mode) & res;
		}
		return res;
	}

	// No conversion possible, assign the default value and log an error
	logger.error(msgUnexpectedType(var, VariantType::ARRAY));
	var.setArray(Variant::arrayType{});
	return false;
}

bool VariantConverter::toMap(Variant &var, const Rtti &innerType,
                             Logger &logger, Mode mode)
{
	// Make sure the variant is a map
	if (var.isMap()) {
		// If no specific inner type is given, conversion is successful at this
		// point
		if (&innerType == &RttiTypes::None) {
			return true;
		}

		// Convert the inner type of the map to the specified inner type, log
		// all failures to do so
		bool res = true;
		for (auto &e : var.asMap()) {
			res = convert(e.second, innerType, RttiTypes::None, logger, mode) &
			      res;
		}
		return res;
	}

	// No conversion possible, assign the default value and log an error
	logger.error(msgUnexpectedType(var, VariantType::MAP));
	var.setMap(Variant::mapType{});
	return false;
}

bool VariantConverter::toFunction(Variant &var, Logger &logger)
{
	if (var.isFunction()) {
		return true;
	}

	// No conversion possible, assign the default value and log an error
	logger.error(msgUnexpectedType(var, VariantType::FUNCTION));
	var.setFunction(std::shared_ptr<Function>{new FunctionStub()});
	return false;
}

bool VariantConverter::convert(Variant &var, const Rtti &type,
                               const Rtti &innerType, Logger &logger,
                               Mode mode)
{
	// Check for simple Variant types
	if (&type == &RttiTypes::None) {
		return true;  // Everything is fine if no specific type was
		              // requested
	} else if (&type == &RttiTypes::Nullptr) {
		// Make sure the variant is set to null
		if (!var.isNull()) {
			logger.error(msgUnexpectedType(var, VariantType::NULLPTR));
			var.setNull();
			return false;
		}
		return true;
	} else if (&type == &RttiTypes::Bool) {
		return toBool(var, logger, mode);
	} else if (&type == &RttiTypes::Int) {
		return toInt(var, logger, mode);
	} else if (&type == &RttiTypes::Double) {
		return toDouble(var, logger, mode);
	} else if (&type == &RttiTypes::String) {
		return toString(var, logger, mode);
	} else if (&type == &RttiTypes::Array) {
		return toArray(var, innerType, logger, mode);
	} else if (&type == &RttiTypes::Map) {
		return toMap(var, innerType, logger, mode);
	} else if (&type == &RttiTypes::Function) {
		return toFunction(var, logger);
	}

	// If none of the above primitive types is requested, we were
	// obviously asked for a managed object.
	if (!var.isObject()) {
		logger.error(msgUnexpectedType(var, VariantType::OBJECT));
		var.setObject(nullptr);
		return false;
	}

	// Make sure the object type is correct
	if (!var.getRtti().isa(type)) {
		logger.error(std::string("Expected object of type ") + type.name +
		             " but got object of type " + var.getRtti().name);
		var.setObject(nullptr);
		return false;
	}
	return true;
}

bool VariantConverter::convert(Variant &var, const Rtti &type,
                               Logger &logger, Mode mode)
{
	return convert(var, type, RttiTypes::None, logger, mode);
}
}

