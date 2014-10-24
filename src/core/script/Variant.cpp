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
#include "Function.hpp"
#include "Object.hpp"

namespace ousia {
namespace script {

/* Class Variant */

const Variant Variant::Null;

Variant::Variant(const Variant &v) : type(v.type)
{
	switch (v.type) {
		case VariantType::null:
			break;
		case VariantType::boolean:
			booleanValue = v.booleanValue;
			break;
		case VariantType::integer:
			integerValue = v.integerValue;
			break;
		case VariantType::number:
			numberValue = v.numberValue;
			break;
		case VariantType::string:
			objectValue =
			    new std::string(*static_cast<std::string *>(v.objectValue));
			break;
		case VariantType::array:
			objectValue = new std::vector<Variant>(
			    *static_cast<std::vector<Variant> *>(v.objectValue));
			break;
		case VariantType::map:
			objectValue = new std::map<std::string, Variant>(
			    *static_cast<std::map<std::string, Variant> *>(v.objectValue));
			break;
		case VariantType::function:
			objectValue = static_cast<Function *>(v.objectValue)->clone();
			break;
		case VariantType::object:
			objectValue = new Object(*static_cast<Object *>(v.objectValue));
			break;
		case VariantType::buffer:
			// TODO
			break;
	}
}

Variant::Variant(Variant &&v) : type(v.type)
{
	switch (type) {
		case VariantType::null:
			break;
		case VariantType::boolean:
			booleanValue = v.booleanValue;
			break;
		case VariantType::integer:
			integerValue = v.integerValue;
			break;
		case VariantType::number:
			numberValue = v.numberValue;
			break;
		case VariantType::string:
		case VariantType::array:
		case VariantType::map:
		case VariantType::function:
		case VariantType::object:
		case VariantType::buffer:
			objectValue = v.objectValue;
			v.objectValue = nullptr;
			break;
	}
}

Variant::Variant() : type(VariantType::null)
{
}

Variant::Variant(bool b) : type(VariantType::boolean), booleanValue(b)
{
}

Variant::Variant(int64_t i) : type(VariantType::integer), integerValue(i)
{
}

Variant::Variant(double d) : type(VariantType::number), numberValue(d)
{
}

Variant::Variant(const char *s)
    : type(VariantType::string), objectValue(new std::string(s))
{
}

Variant::Variant(const std::vector<Variant> &a)
    : type(VariantType::array), objectValue(new std::vector<Variant>(a))
{
}

Variant::Variant(const std::map<std::string, Variant> &m)
    : type(VariantType::map), objectValue(new std::map<std::string, Variant>(m))
{
}

Variant::Variant(const Function *f)
    : type(VariantType::function), objectValue(f->clone())
{
}

Variant::Variant(const Object &o)
    : type(VariantType::object), objectValue(new Object(o))
{
}

Variant::~Variant()
{
	switch (type) {
		case VariantType::string:
			delete static_cast<std::string *>(objectValue);
			break;
		case VariantType::array:
			delete static_cast<std::vector<Variant> *>(objectValue);
			break;
		case VariantType::map:
			delete static_cast<std::map<std::string, Variant> *>(objectValue);
			break;
		case VariantType::function:
			delete static_cast<Function*>(objectValue);
			break;
		case VariantType::object:
			delete static_cast<Object*>(objectValue);
			break;
		default:
			break;
	}
}

bool Variant::getBooleanValue() const
{
	switch (type) {
		case VariantType::null:
			return false;
		case VariantType::boolean:
			return booleanValue;
		case VariantType::integer:
			return integerValue != 0;
		case VariantType::number:
			return numberValue != 0.0;
		case VariantType::string:
			return !getStringValue().empty();
		case VariantType::array:
			return !getArrayValue().empty();
		case VariantType::map:
			return !getMapValue().empty();
		default:
			throw VariantTypeException{type, VariantType::boolean};
	}
}

int64_t Variant::getIntegerValue() const
{
	switch (type) {
		case VariantType::boolean:
			return booleanValue ? 1 : 0;
		case VariantType::integer:
			return integerValue;
		case VariantType::number:
			return static_cast<int64_t>(numberValue);
		default:
			throw VariantTypeException{type, VariantType::integer};
	}
}

double Variant::getNumberValue() const
{
	switch (type) {
		case VariantType::boolean:
			return booleanValue ? 1.0 : 0.0;
		case VariantType::integer:
			return static_cast<double>(integerValue);
		case VariantType::number:
			return numberValue;
		default:
			throw VariantTypeException{type, VariantType::number};
	}
}

const std::string &Variant::getStringValue() const
{
	switch (type) {
		case VariantType::string:
			return *(static_cast<std::string *>(objectValue));
		default:
			throw VariantTypeException{type, VariantType::string};
	}
}

const std::vector<Variant> &Variant::getArrayValue() const
{
	switch (type) {
		case VariantType::array:
			return *(static_cast<std::vector<Variant> *>(objectValue));
		default:
			throw VariantTypeException{type, VariantType::array};
	}
}

const std::map<std::string, Variant> &Variant::getMapValue() const
{
	switch (type) {
		case VariantType::map:
			return *(static_cast<std::map<std::string, Variant> *>(
			    objectValue));
		default:
			throw VariantTypeException{type, VariantType::map};
	}
}

const Function *Variant::getFunctionValue() const
{
	switch (type) {
		case VariantType::function: return static_cast<Function *>(objectValue);
		    default:
			throw VariantTypeException{type, VariantType::function};
	}
}

const Object &Variant::getObjectValue() const
{
	switch (type) {
		case VariantType::object: return *(static_cast<Object *>(objectValue));
		    default:
			throw VariantTypeException{type, VariantType::function};
	}
}

const char *Variant::getTypeName(VariantType type)
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

/* Class VariantTypeException */

VariantTypeException::VariantTypeException(VariantType actualType,
                                           VariantType requestedType)
    : msg(std::string("Cannot get value of variant of type \"") +
          Variant::getTypeName(actualType) + std::string("\" as \"") +
          Variant::getTypeName(requestedType) + std::string("\"")),
      actualType(actualType),
      requestedType(requestedType)
{
}

const char *VariantTypeException::what() const noexcept
{
	return msg.c_str();
}

/* Global scope operator */

std::ostream &operator<<(std::ostream &os, const Variant &v)
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

