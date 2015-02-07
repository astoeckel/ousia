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

#include "Location.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "Variant.hpp"
#include "VariantConverter.hpp"
#include "VariantWriter.hpp"
#include "Rtti.hpp"

namespace ousia {

/* Struct VariantMetadata */

bool VariantMetadata::hasLocation() const
{
	return locationSourceId != InvalidSourceId;
}

SourceLocation VariantMetadata::getLocation() const
{
	if (locationOffset == InvalidLocationOffset) {
		return SourceLocation(locationSourceId);
	}
	if (locationLength == InvalidLocationLength) {
		return SourceLocation(locationSourceId, locationOffset);
	}
	return SourceLocation(locationSourceId, locationOffset,
	                      static_cast<size_t>(locationOffset) +
	                          static_cast<size_t>(locationLength));
}

void VariantMetadata::setLocation(const SourceLocation &location)
{
	// Copy the location members
	const SourceId sourceId = location.getSourceId();
	const size_t offset = location.getStart();
	const size_t length = location.getLength();

	// Copy the location, mark values that cannot be stored as invalid
	locationSourceId =
	    sourceId < InvalidLocationSourceId ? sourceId : InvalidLocationSourceId;
	locationOffset =
	    offset < InvalidLocationOffset ? offset : InvalidLocationOffset;
	locationLength =
	    length < InvalidLocationLength ? length : InvalidLocationLength;
}

/* Class Variant::TypeException */

Variant::TypeException::TypeException(VariantType actualType,
                                      VariantType requestedType)
    : OusiaException(std::string("Variant: Requested \"") +
                     Variant::getTypeName(requestedType) +
                     std::string("\" but is \"") +
                     Variant::getTypeName(actualType) + std::string("\"")),
      actualType(actualType),
      requestedType(requestedType)
{
}

/* Class Variant */

const char *Variant::getTypeName(VariantType type)
{
	switch (type) {
		case VariantType::NULLPTR:
			return "null";
		case VariantType::BOOL:
			return "boolean";
		case VariantType::INT:
			return "integer";
		case VariantType::DOUBLE:
			return "double";
		case VariantType::STRING:
			return "string";
		case VariantType::MAGIC:
			return "magic";
		case VariantType::ARRAY:
			return "array";
		case VariantType::MAP:
			return "map";
		case VariantType::OBJECT:
			return "object";
		case VariantType::CARDINALITY:
			return "cardinality";
		case VariantType::FUNCTION:
			return "function";
	}
	return "unknown";
}

/* Conversion functions */

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

Variant::stringType Variant::toString() const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toString(res, logger, VariantConverter::Mode::ALL);
	return res.asString();
}

Variant::arrayType Variant::toArray() const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toArray(res, &RttiTypes::None, logger,
	                          VariantConverter::Mode::ALL);
	return res.asArray();
}

Variant::arrayType Variant::toArray(const Rtti *innerType) const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toArray(res, innerType, logger,
	                          VariantConverter::Mode::ALL);
	return res.asArray();
}

Variant::mapType Variant::toMap() const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toMap(res, &RttiTypes::None, logger,
	                        VariantConverter::Mode::ALL);
	return res.asMap();
}

Variant::mapType Variant::toMap(const Rtti *innerType) const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toMap(res, innerType, logger,
	                        VariantConverter::Mode::ALL);
	return res.asMap();
}

Variant::cardinalityType Variant::toCardinality() const
{
	ExceptionLogger logger;
	Variant res{*this};
	VariantConverter::toCardinality(res, logger, VariantConverter::Mode::ALL);
	return res.asCardinality();
}

/* Type management */

const Rtti *Variant::getRtti() const
{
	switch (meta.getType()) {
		case VariantType::NULLPTR:
			return &RttiTypes::Nullptr;
		case VariantType::BOOL:
			return &RttiTypes::Bool;
		case VariantType::INT:
			return &RttiTypes::Int;
		case VariantType::DOUBLE:
			return &RttiTypes::Double;
		case VariantType::STRING:
		case VariantType::MAGIC:
			return &RttiTypes::String;
		case VariantType::ARRAY:
			return &RttiTypes::Array;
		case VariantType::MAP:
			return &RttiTypes::Map;
		case VariantType::CARDINALITY:
			return &RttiTypes::Cardinality;
		case VariantType::FUNCTION:
			return &RttiTypes::Function;
		case VariantType::OBJECT: {
			Variant::objectType o = asObject();
			return (o == nullptr) ? &RttiTypes::Nullptr : o->type();
		}
	}
	return &RttiTypes::None;
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
		case VariantType::NULLPTR:
			return false;
		case VariantType::BOOL:
			return lhs.boolVal < rhs.boolVal;
		case VariantType::INT:
			return lhs.intVal < rhs.intVal;
		case VariantType::DOUBLE:
			return lhs.doubleVal < rhs.doubleVal;
		case VariantType::MAGIC:
		case VariantType::STRING:
			return lhs.asString() < rhs.asString();
		case VariantType::ARRAY:
			return lhs.asArray() < rhs.asArray();
		case VariantType::MAP:
			return lhs.asMap() < rhs.asMap();
		case VariantType::CARDINALITY:
			throw OusiaException(
			    "No sensible comparison on cardinalities is possible!");
		case VariantType::OBJECT:
			throw OusiaException(
			    "No sensible comparison on objects is possible!");
		case VariantType::FUNCTION:
			throw OusiaException(
			    "No sensible comparison on functions is possible!");
	}
	throw OusiaException("Internal Error! Unknown type!");
}

bool operator>(const Variant &lhs, const Variant &rhs) { return rhs < lhs; }

bool operator<=(const Variant &lhs, const Variant &rhs) { return !(lhs > rhs); }

bool operator>=(const Variant &lhs, const Variant &rhs) { return !(lhs < rhs); }

bool operator==(const Variant &lhs, const Variant &rhs)
{
	if (lhs.getType() != rhs.getType()) {
		return false;
	}
	switch (lhs.getType()) {
		case VariantType::NULLPTR:
			return true;
		case VariantType::BOOL:
			return lhs.boolVal == rhs.boolVal;
		case VariantType::INT:
			return lhs.intVal == rhs.intVal;
		case VariantType::DOUBLE:
			return lhs.doubleVal == rhs.doubleVal;
		case VariantType::STRING:
		case VariantType::MAGIC:
			return lhs.asString() == rhs.asString();
		case VariantType::ARRAY:
			return lhs.asArray() == rhs.asArray();
		case VariantType::MAP:
			return lhs.asMap() == rhs.asMap();
		case VariantType::OBJECT:
			return lhs.asObject() == rhs.asObject();
		case VariantType::CARDINALITY:
			return lhs.asCardinality() == rhs.asCardinality();
		case VariantType::FUNCTION:
			return lhs.asFunction() == rhs.asFunction();
	}
	throw OusiaException("Internal Error! Unknown type!");
}

bool operator!=(const Variant &lhs, const Variant &rhs)
{
	return !(lhs == rhs);
}
}

