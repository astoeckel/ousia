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

#include <string>
#include <sstream>

#include "Logger.hpp"
#include "Number.hpp"
#include "Rtti.hpp"
#include "Variant.hpp"
#include "VariantConverter.hpp"
#include "VariantWriter.hpp"

namespace ousia {

static std::string msgUnexpectedType(VariantType actualType,
                                     VariantType requestedType)
{
	return std::string("Cannot convert ") + Variant::getTypeName(actualType) +
	       std::string(" to ") + Variant::getTypeName(requestedType);
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
	logger.error(msgUnexpectedType(var.getType(), VariantType::BOOL));
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
				} else {
					var = (Variant::doubleType)n.doubleValue();
				}
				return true;
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
	logger.error(msgUnexpectedType(var.getType(), VariantType::INT));
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
	logger.error(msgUnexpectedType(var.getType(), VariantType::DOUBLE));
	var = 0.0;
	return false;
}

bool VariantConverter::toString(Variant &var, Logger &logger, Mode mode)
{
	// Perform safe conversions (all these operations are considered "lossless")
	const VariantType type = var.getType();
	switch (type) {
		case VariantType::NULLPTR:
			var = "null";
			return true;
		case VariantType::BOOL:
			var = var.asBool() ? "true" : "false";
			return true;
		case VariantType::INT: {
			std::stringstream ss;
			ss << var.asInt();
			var = ss.str().c_str();
			return true;
		}
		case VariantType::DOUBLE: {
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
				ss << "<object " << obj.get() << " (" << obj->type().name << ")"
				   << ">";
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
	logger.error(msgUnexpectedType(var.getType(), VariantType::STRING));
	var = "";
	return false;
}
}

