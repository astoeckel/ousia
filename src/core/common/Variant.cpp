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

#include <sstream>

#include "Utils.hpp"
#include "Variant.hpp"

namespace ousia {

/* Class Variant::TypeException */

Variant::TypeException::TypeException(Type actualType, Type requestedType)
    : OusiaException(std::string("Variant: Requested \"") +
                     Variant::getTypeName(requestedType) +
                     std::string("\" but is \"") +
                     Variant::getTypeName(actualType) + std::string("\"")),
      actualType(actualType),
      requestedType(requestedType)
{
}

/* Class Variant */

const char *Variant::getTypeName(Type type)
{
	switch (type) {
		case Type::NULLPTR:
			return "null";
		case Type::BOOL:
			return "boolean";
		case Type::INT:
			return "integer";
		case Type::DOUBLE:
			return "double";
		case Type::STRING:
			return "string";
		case Type::ARRAY:
			return "array";
		case Type::MAP:
			return "map";
	}
	return "unknown";
}

Variant::boolType Variant::toBool() const
{
	switch (getType()) {
		case Type::NULLPTR:
			return false;
		case Type::BOOL:
			return asBool();
		case Type::INT:
			return asInt() != 0;
		case Type::DOUBLE:
			return asDouble() != 0.0;
		case Type::STRING:
			return true;
		case Type::ARRAY:
			return true;
		case Type::MAP:
			return true;
	}
	return false;
}

Variant::intType Variant::toInt() const
{
	switch (getType()) {
		case Type::NULLPTR:
			return 0;
		case Type::BOOL:
			return asBool() ? 1 : 0;
		case Type::INT:
			return asInt();
		case Type::DOUBLE:
			return asDouble();
		case Type::STRING:
			return 0; // TODO: Parse string as int
		case Type::ARRAY: {
			const arrayType &a = asArray();
			return (a.size() == 1) ? a[0].toInt() : 0;
		}
		case Type::MAP:
			return 0;
	}
	return false;
}

Variant::doubleType Variant::toDouble() const
{
	switch (getType()) {
		case Type::NULLPTR:
			return 0.0;
		case Type::BOOL:
			return asBool() ? 1.0 : 0.0;
		case Type::INT:
			return asInt();
		case Type::DOUBLE:
			return asDouble();
		case Type::STRING:
			return 0.0; // TODO: Parse string as double
		case Type::ARRAY: {
			const arrayType &a = asArray();
			return (a.size() == 1) ? a[0].toDouble() : 0;
		}
		case Type::MAP:
			return 0;
	}
	return false;
}

Variant::stringType Variant::toString(bool escape) const
{
	switch (getType()) {
		case Type::NULLPTR:
			return "null";
		case Type::BOOL:
			return asBool() ? "true" : "false";
		case Type::INT: {
			std::stringstream ss;
			ss << asInt();
			return ss.str();
		}
		case Type::DOUBLE: {
			std::stringstream ss;
			ss << asDouble();
			return ss.str();
		}
		case Type::STRING: {
			// TODO: Use proper serialization function
			if (escape) {
				std::stringstream ss;
				ss << "\"" << asString() << "\"";
				return ss.str();
			} else {
				return asString();
			}
		}
		case Type::ARRAY:
			return Utils::join(asArray(), ", ", "[", "]");
		case Type::MAP:
			return Utils::join(asMap(), ", ", "{", "}");
	}
	return "";
}

}

