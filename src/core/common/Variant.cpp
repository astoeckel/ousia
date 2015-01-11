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

#include <core/managed/Managed.hpp>

#include "Logger.hpp"
#include "Utils.hpp"
#include "Variant.hpp"
#include "VariantConverter.hpp"
#include "VariantWriter.hpp"

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
		case Type::MAGIC:
			return "magic";
		case Type::ARRAY:
			return "array";
		case Type::MAP:
			return "map";
		case Type::OBJECT:
			return "object";
		case Type::FUNCTION:
			return "function";
	}
	return "unknown";
}

Variant::boolType Variant::toBool() const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toBool(res, logger, VariantConverter::Mode::ALL);
	return res.asBool();
}

Variant::intType Variant::toInt() const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toInt(res, logger, VariantConverter::Mode::ALL);
	return res.asInt();
}

Variant::doubleType Variant::toDouble() const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toDouble(res, logger, VariantConverter::Mode::ALL);
	return res.asDouble();
}

Variant::stringType Variant::toString(bool escape) const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toString(res, logger, VariantConverter::Mode::ALL);
	return res.asString();
}

/* Output stream operators */

std::ostream &operator<<(std::ostream &os, const Variant &v)
{
	VariantWriter::writeJson(v, os, true);
	return os;
}

/* Comparison operators */

bool operator<(const Variant &lhs, const Variant &rhs)
{
	// If the types do not match, we can not do a meaningful comparison.
	if (lhs.getType() != rhs.getType()) {
		throw Variant::TypeException(lhs.getType(), rhs.getType());
	}
	switch (lhs.getType()) {
		case Variant::Type::NULLPTR:
			return false;
		case Variant::Type::BOOL:
			return lhs.boolVal < rhs.boolVal;
		case Variant::Type::INT:
			return lhs.intVal < rhs.intVal;
		case Variant::Type::DOUBLE:
			return lhs.doubleVal < rhs.doubleVal;
		case Variant::Type::MAGIC:
		case Variant::Type::STRING:
			return lhs.asString() < rhs.asString();
		case Variant::Type::ARRAY:
			return lhs.asArray() < rhs.asArray();
		case Variant::Type::MAP:
			return lhs.asMap() < rhs.asMap();
		case Variant::Type::OBJECT:
			return lhs.asObject().get() < rhs.asObject().get();
		case Variant::Type::FUNCTION:
			return lhs.asFunction() < rhs.asFunction();
	}
	throw OusiaException("Internal Error! Unknown type!");
}

bool operator>(const Variant &lhs, const Variant &rhs)
{
	return rhs < lhs;
}

bool operator<=(const Variant &lhs, const Variant &rhs)
{
	return !(lhs > rhs);
}

bool operator>=(const Variant &lhs, const Variant &rhs)
{
	return !(lhs < rhs);
}

bool operator==(const Variant &lhs, const Variant &rhs)
{
	if (lhs.getType() != rhs.getType()) {
		return false;
	}
	switch (lhs.getType()) {
		case Variant::Type::NULLPTR:
			return true;
		case Variant::Type::BOOL:
			return lhs.boolVal == rhs.boolVal;
		case Variant::Type::INT:
			return lhs.intVal == rhs.intVal;
		case Variant::Type::DOUBLE:
			return lhs.doubleVal == rhs.doubleVal;
		case Variant::Type::STRING:
		case Variant::Type::MAGIC:
			return lhs.asString() == rhs.asString();
		case Variant::Type::ARRAY:
			return lhs.asArray() == rhs.asArray();
		case Variant::Type::MAP:
			return lhs.asMap() == rhs.asMap();
		case Variant::Type::OBJECT:
			return lhs.asObject() == rhs.asObject();
		case Variant::Type::FUNCTION:
			return lhs.asFunction() == rhs.asFunction();
	}
	throw OusiaException("Internal Error! Unknown type!");
}

bool operator!=(const Variant &lhs, const Variant &rhs)
{
	return !(lhs == rhs);
}

}

