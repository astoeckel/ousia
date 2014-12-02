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

#include "Variant.hpp"

namespace ousia {

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
			return "number";
		case Type::STRING:
			return "string";
		case Type::ARRAY:
			return "array";
		case Type::MAP:
			return "map";
	}
	return "unknown";
}

/* Class VariantTypeException */

Variant::TypeException::TypeException(Type actualType, Type requestedType)
    : OusiaException(std::string("Variant: Requested \"") +
                     Variant::getTypeName(actualType) +
                     std::string("\" but is \"") +
                     Variant::getTypeName(requestedType) + std::string("\"")),
      actualType(actualType),
      requestedType(requestedType)
{
}
}

