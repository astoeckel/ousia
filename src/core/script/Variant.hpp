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
#include <exception>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace ousia {
namespace script {

/**
 * Enum containing the possible types a variant may have.
 */
enum class VariantType : int16_t {
	null = 0x0001,
	boolean = 0x0002,
	integer = 0x0004,
	number = 0x0008,
	string = 0x0010,
	array = 0x0020,
	map = 0x0040,
	function = 0x0080,
	object = 0x0100,
	buffer = 0x0200
};

/**
 * Exception thrown whenever a variant is accessed via a getter function that
 * is not supported for the current variant type.
 */
class VariantTypeException : std::exception {

private:
	/**
	 * Internally used string holding the exception message.
	 */
	const std::string msg;

public:
	/**
	 * Contains the actual type of the variant.
	 */
	const VariantType actualType;

	/**
	 * Contains the requested type of the variant.
	 */
	const VariantType requestedType;

	/**
	 * Constructor of the VariantTypeException.
	 *
	 * @param actualType describes the actual type of the variant.
	 * @param requestedType describes the type in which the variant was
	 * requested.
	 */
	VariantTypeException(VariantType actualType, VariantType requestedType);

	/**
	 * Returns the error message of the exception.
	 *
	 * @return the error message as C string.
	 */
	virtual const char* what() const noexcept override;

};

/**
 * Instances of the Variant class represent any kind of data that is exchanged
 * between the host application and the script engine. Variants are immutable.
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
				throw VariantTypeException{type, VariantType::boolean};
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
				throw VariantTypeException{type, VariantType::integer};
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
				throw VariantTypeException{type, VariantType::number};
		}
	}

	const std::string& getStringValue() const
	{
		switch (type) {
			case VariantType::string:
				return *(static_cast<std::string*>(objectValue));
			default:
				throw VariantTypeException{type, VariantType::string};
		}
	}

	const std::vector<Variant>& getArrayValue() const
	{
		switch (type) {
			case VariantType::array:
				return *(static_cast<std::vector<Variant>*>(objectValue));
			default:
				throw VariantTypeException{type, VariantType::array};
		}
	}

	const std::map<std::string, Variant>& getMapValue() const
	{
		switch (type) {
			case VariantType::map:
				return *(static_cast<std::map<std::string, Variant>*>(objectValue));
			default:
				throw VariantTypeException{type, VariantType::map};
		}
	}

	friend std::ostream& operator<< (std::ostream& os, const Variant &v);

};


/**
 * Shorthand for a constant representing a "null" as a variant.
 */
static const Variant VarNull;

}
}

#endif /* _OUSIA_VARIANT_HPP_ */

