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

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>
#include <map>

namespace ousia {
namespace script {

// TODO: Make Variant immutable (?), store large objects in heap buffer

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
	VariantType type;

	union {
		bool booleanValue;
		int64_t integerValue;
		double numberValue;
		std::string stringValue;
		std::vector<Variant> arrayValue;
		std::map<std::string, Variant> mapValue;
	};

	/**
	 * Private function calling the destructor of the currently used union
	 * member.
	 */
	void free() {
		// Explicitly call the destructor
		switch (type) {
			case VariantType::string:
				stringValue.std::string::~string();
				break;
			case VariantType::array:
				arrayValue.std::vector<Variant>::~vector();
				break;
			case VariantType::map:
				mapValue.std::map<std::string, Variant>::~map();
				break;
			default:
				break;
		}

		// Reset the type
		type = VariantType::none;
	}

	/**
	 * Function for copying the content of the given instance v to this
	 * instance. Callers must make sure the storage space has been freed
	 * beforehand.
	 */
	void copy(const Variant &v)
	{
		type = v.type;
		switch (type) {
			case VariantType::integer:
				integerValue = v.integerValue;
				break;
			case VariantType::number:
				numberValue = v.numberValue;
				break;
			case VariantType::string:
				new (&stringValue) std::string(v.stringValue);
				break;
			case VariantType::array:
				new (&arrayValue) std::vector<Variant>(v.arrayValue);
				break;
			case VariantType::map:
				new (&mapValue) std::map<std::string, Variant>(v.mapValue);
				break;
			default:
				break;
		}
	}

	/**
	 * Function for moving the content of the given instance v to this instance.
	 * No copy operation is used. Callers must make sure the storage space has
	 * been freed beforehand.
	 */
	void move(Variant &v)
	{
		type = v.type;
		switch (type) {
			case VariantType::integer:
				integerValue = v.integerValue;
				break;
			case VariantType::number:
				numberValue = v.numberValue;
				break;
			case VariantType::string:
				new (&stringValue) std::string(std::move(v.stringValue));
				break;
			case VariantType::array:
				new (&arrayValue) std::vector<Variant>(std::move(v.arrayValue));
				break;
			case VariantType::map:
				new (&mapValue) std::map<std::string, Variant>(std::move(v.mapValue));
				break;
			default:
				break;
		}

		// Reset the type of v to "none"
		v.type = VariantType::none;
	}

public:

	class EBadEntry {};

	Variant(const Variant &v)
	{
		copy(v);
	}

	Variant(Variant &&v)
	{
		move(v);
	}

	Variant& operator=(const Variant &v)
	{
		free();
		copy(v);
		return *this;
	}

	Variant& operator=(Variant &&v)
	{
		free();
		move(v);
		return *this;
	}


	Variant(int64_t i) :
		type(VariantType::integer),
		integerValue(i)
	{
		// Do nothing here
	}

	Variant(double d) :
		type(VariantType::number),
		numberValue(d)
	{
		// Do nothing here
	}

	Variant(const char *s) :
		type(VariantType::string)
	{
		new (&stringValue) std::string(s);
	}

	Variant(const std::vector<Variant> &a) :
		type(VariantType::array)
	{
		new (&arrayValue) std::vector<Variant>(a);
	}


	Variant(const std::map<std::string, Variant> &m) :
		type(VariantType::map)
	{
		new (&mapValue) std::map<std::string, Variant>(m);
	}

	~Variant()
	{
		free();
	}

	VariantType getType() const
	{
		return type;
	}

	int64_t getIntegerValue() const
	{
		switch (type) {
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
				return stringValue;
			default:
				throw EBadEntry {};
		}
	}

	const std::vector<Variant>& getArrayValue() const
	{
		switch (type) {
			case VariantType::array:
				return arrayValue;
			default:
				throw EBadEntry {};
		}
	}

	const std::map<std::string, Variant>& getMapValue() const
	{
		switch (type) {
			case VariantType::map:
				return mapValue;
			default:
				throw EBadEntry {};
		}
	}

	friend std::ostream& operator<< (std::ostream& os, const Variant &v);

};

}
}

#endif /* _OUSIA_VARIANT_HPP_ */

