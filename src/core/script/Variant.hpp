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

/* Class forward declarations */
class Object;
class Function;

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
class VariantTypeException : public std::exception {
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
	virtual const char *what() const noexcept override;
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
	Variant(const Variant &v);
	Variant(Variant &&v);

	Variant();
	Variant(bool b);
	Variant(int64_t i);
	Variant(double d);
	Variant(const char *s);
	Variant(const std::vector<Variant> &a);
	Variant(const std::map<std::string, Variant> &m);
	Variant(const Function *f);
	Variant(const Object &o);
	~Variant();

	Variant &operator=(const Variant &v) = delete;
	Variant &operator=(Variant &&v) = delete;

	VariantType getType() const
	{
		return type;
	}

	bool getBooleanValue() const;
	int64_t getIntegerValue() const;
	double getNumberValue() const;
	const std::string &getStringValue() const;
	const std::vector<Variant> &getArrayValue() const;
	const std::map<std::string, Variant> &getMapValue() const;
	const Function *getFunctionValue() const;
	const Object &getObjectValue() const;

	static const char *getTypeName(VariantType type);

	friend std::ostream &operator<<(std::ostream &os, const Variant &v);
};

/**
 * Shorthand for a constant representing a "null" as a variant.
 */
static const Variant VarNull;
}
}

#endif /* _OUSIA_VARIANT_HPP_ */

