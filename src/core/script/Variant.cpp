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
namespace script {

/* Class VariantTypeException */

static const char* getVariantTypeName(VariantType type)
{
	switch (type) {
		case VariantType::null:
			return "null";
		case VariantType::boolean:
			return "boolean";
		case VariantType::integer:
			return "integer";
		case VariantType::number:
			return "number";
		case VariantType::string:
			return "string";
		case VariantType::array:
			return "array";
		case VariantType::map:
			return "map";
		case VariantType::function:
			return "function";
		case VariantType::object:
			return "object";
		case VariantType::buffer:
			return "buffer";
	}
	return "unknown";
}

VariantTypeException::VariantTypeException(VariantType actualType,
		VariantType requestedType) :
	msg(std::string("Cannot get value of variant of type \"")
			+ getVariantTypeName(actualType)
			+ std::string("\" as \"") + getVariantTypeName(requestedType)),
	actualType(actualType), requestedType(requestedType) {}

const char* VariantTypeException::what() const noexcept
{
	return msg.c_str();
}

/* Global scope operator */

std::ostream& operator<< (std::ostream& os, const Variant &v)
{
	switch (v.type) {
		case VariantType::null:
			os << "null";
			break;
		case VariantType::boolean:
			os << (v.booleanValue ? "true" : "false");
			break;
		case VariantType::integer:
			os << v.integerValue;
			break;
		case VariantType::number:
			os << v.numberValue;
			break;
		case VariantType::string:
			os << "\"" << v.getStringValue() << "\"";
			break;
		case VariantType::array: {
			bool first = true;
			os << "[";
			for (auto &v2 : v.getArrayValue()) {
				if (!first) {
					os << ", ";
				}
				os << v2;
				first = false;
			}
			os << "]";
			break;
		}
		case VariantType::map: {
			bool first = true;
			os << "{";
			for (auto &v2 : v.getMapValue()) {
				if (!first) {
					os << ", ";
				}
				os << "\"" << v2.first << "\": " << v2.second;
				first = false;
			}
			os << "}";
			break;
		}
		case VariantType::function:
			os << "<Function>";
			break;
		case VariantType::object:
			os << "<Object>";
			break;
		case VariantType::buffer:
			os << "<Buffer>";
			break;
	}
	return os;
}

}
}

