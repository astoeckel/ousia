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

/* Class forward declarations to avoid cyclic dependencies in the header */
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
	/**
	 * Used to store the actual type of the variant.
	 */
	const VariantType type;

	/**
	 * Anonymous union containing the possible value of the variant.
	 */
	union {
		/**
		 * The boolean value. Only valid if type is VariantType::boolean.
		 */
		bool booleanValue;
		/**
		 * The integer value. Only valid if type is VariantType::integer.
		 */
		int64_t integerValue;
		/**
		 * The number value. Only valid if type is VariantType::double.
		 */
		double numberValue;
		/**
		 * Pointer to the more complex data structures on the free store. Only
		 * valid if type is one of VariantType::string, VariantType::array,
		 * VariantType::map, VariantType::function, VariantType::object.
		 */
		void *objectValue = nullptr;
	};

public:
	/**
	 * Copy constructor of the Variant class.
	 *
	 * @param v is the Variant instance that should be cloned.
	 */
	Variant(const Variant &v);

	/**
	 * Move constructor of the Variant class.
	 *
	 * @param v is the reference to the Variant instance that should be moved,
	 * this instance is invalidated afterwards.
	 */
	Variant(Variant &&v);

	/**
	 * Default constructor. Type is set to VariantType:null.
	 */
	Variant();

	/**
	 * Constructor for boolean values.
	 *
	 * @param b boolean value.
	 */
	Variant(bool b);

	/**
	 * Constructor for integer values.
	 *
	 * @param i integer value.
	 */
	Variant(int64_t i);

	/**
	 * Constructor for number values.
	 *
	 * @param d number (double) value.
	 */
	Variant(double d);

	/**
	 * Constructor for string values. The given string is copied and managed by
	 * the new Variant instance.
	 *
	 * @param s is a reference to a C-Style string used as string value.
	 */
	Variant(const char *s);

	/**
	 * Constructor for array values. The given array is copied and managed by
	 * the new Variant instance.
	 *
	 * @param a is a reference to the array
	 */
	Variant(const std::vector<Variant> &a);

	/**
	 * Constructor for map values. The given map is copied and managed by the new
	 * Variant instance.
	 *
	 * @param m is a reference to the map.
	 */
	Variant(const std::map<std::string, Variant> &m);

	/**
	 * Constructor for function values. The given pointer to the function object is cloned and managed by the new Variant instance.
	 *
	 * @param f is a reference to the function.
	 */
	Variant(const Function *f);

	/**
	 * Constructor for object values. The given Object is copied and managed by
	 * the new Variant instance.
	 *
	 * @param o is a reference to the object.
	 */
	Variant(const Object &o);

	/**
	 * Destructor of the Variant class.
	 */
	~Variant();

	/**
	 * Assign operator.
	 */
	Variant &operator=(const Variant &v) = delete;

	/**
	 * Move assign operator.
	 */
	Variant &operator=(Variant &&v) = delete;

	/**
	 * Returns the current type of the Variant.
	 *
	 * @return the current type of the Variant.
	 */
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

	/**
	 * Shorthand for a constant representing a "null" as a variant.
	 */
	static const Variant Null;

	/**
	 * Returns the name of the given variant type as C-style string.
	 */
	static const char *getTypeName(VariantType type);

	/**
	 * Prints the object as JSON to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &os, const Variant &v);
};

}
}

#endif /* _OUSIA_VARIANT_HPP_ */

