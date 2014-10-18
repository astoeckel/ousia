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

#ifndef _OUSIA_VARIANT_HPP_
#define _OUSIA_VARIANT_HPP_

#include <iostream>

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>
#include <map>

namespace ousia {
namespace script {

/**
 * Enum containing the possible types a variant may have.
 */
enum class VariantType {
	null, boolean, integer, number, string, array, map, function, object, buffer
};

/**
 * Instances of the Variant class represent any kind of data that is exchanged
 * between the host application and the script engine.
 */
class Variant {

private:
	const VariantType type;

	union {
		bool booleanValue;
		int64_t integerValue;
		double numberValue;
		void *objectValue = nullptr;
	};

public:

	class EBadEntry {};

	Variant(const Variant &v) :
		type(v.type)
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
				objectValue = new std::string(
						*static_cast<std::string*>(v.objectValue));
				break;
			case VariantType::array:
				objectValue = new std::vector<Variant>(
						*static_cast<std::vector<Variant>*>(v.objectValue));
				break;
			case VariantType::map:
				objectValue = new std::map<std::string, Variant>(
						*static_cast<std::map<std::string, Variant>*>(v.objectValue));
				break;
			case VariantType::function:
			case VariantType::object:
			case VariantType::buffer:
				// TODO
				break;
		}
	}

	Variant(Variant &&v) :
		type(v.type)
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

	~Variant()
	{
		switch (type) {
			case VariantType::string:
				delete static_cast<std::string*>(objectValue);
				break;
			case VariantType::array:
				delete static_cast<std::vector<Variant>*>(objectValue);
				break;
			case VariantType::map:
				delete static_cast<std::map<std::string, Variant>*>(objectValue);
				break;
			default:
				break;
		}
	}

	Variant& operator=(const Variant &v) = delete;
	Variant& operator=(Variant &&v) = delete;

	Variant() :
		type(VariantType::null) {}

	Variant(bool b) :
		type(VariantType::boolean),
		booleanValue(b) {}

	Variant(int64_t i) :
		type(VariantType::integer),
		integerValue(i) {}

	Variant(double d) :
		type(VariantType::number),
		numberValue(d) {}

	Variant(const char *s) :
		type(VariantType::string),
		objectValue(new std::string(s)) {}

	Variant(const std::vector<Variant> &a) :
		type(VariantType::array),
		objectValue(new std::vector<Variant>(a)) {}

	Variant(const std::map<std::string, Variant> &m) :
		type(VariantType::map),
		objectValue(new std::map<std::string, Variant>(m)) {}

	VariantType getType() const
	{
		return type;
	}

	bool getBooleanValue() const
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
				throw EBadEntry{};
		}
	}

	int64_t getIntegerValue() const
	{
		switch (type) {
			case VariantType::boolean:
				return booleanValue ? 1 : 0;
			case VariantType::integer:
				return integerValue;
			case VariantType::number:
				return static_cast<int64_t>(numberValue);
			default:
				throw EBadEntry{};
		}
	}

	double getNumberValue() const
	{
		switch (type) {
			case VariantType::boolean:
				return booleanValue ? 1.0 : 0.0;
			case VariantType::integer:
				return static_cast<double>(integerValue);
			case VariantType::number:
				return numberValue;
			default:
				throw EBadEntry{};
		}
	}

	const std::string& getStringValue() const
	{
		switch (type) {
			case VariantType::string:
				return *(static_cast<std::string*>(objectValue));
			default:
				throw EBadEntry {};
		}
	}

	const std::vector<Variant>& getArrayValue() const
	{
		switch (type) {
			case VariantType::array:
				return *(static_cast<std::vector<Variant>*>(objectValue));
			default:
				throw EBadEntry {};
		}
	}

	const std::map<std::string, Variant>& getMapValue() const
	{
		switch (type) {
			case VariantType::map:
				return *(static_cast<std::map<std::string, Variant>*>(objectValue));
			default:
				throw EBadEntry {};
		}
	}

	friend std::ostream& operator<< (std::ostream& os, const Variant &v);

};

}
}

#endif /* _OUSIA_VARIANT_HPP_ */

