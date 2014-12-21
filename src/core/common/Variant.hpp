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

/**
 * @file Variant.hpp
 *
 * The Variant class is used to efficiently represent a variables of varying
 * type. Variant instances are used to represent data given by the end user and
 * to exchange information between the host application and the script clients.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_VARIANT_HPP_
#define _OUSIA_VARIANT_HPP_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <ostream>

// TODO: Use
// http://nikic.github.io/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html
// later (will allow to use 8 bytes for a variant)

#include "Exceptions.hpp"

namespace ousia {

/* Forward declaration of the Managed class */
class Managed;

/* Forward declaration of the Function class */
class Function;

/**
 * Instances of the Variant class represent any kind of data that is exchanged
 * between the host application and the script engine. Variants are immutable.
 */
class Variant {
public:
	/**
	 * Enum containing the possible types a variant may have.
	 */
	enum class Type : int16_t {
		NULLPTR,
		BOOL,
		INT,
		DOUBLE,
		STRING,
		ARRAY,
		MAP,
		OBJECT,
		FUNCTION
	};

	/**
	 * Exception thrown whenever a variant is accessed via a getter function
	 * that is not supported for the current variant type.
	 */
	class TypeException : public OusiaException {
	private:
		/**
		 * Internally used string holding the exception message.
		 */
		const std::string msg;

	public:
		/**
		 * Contains the actual type of the variant.
		 */
		const Type actualType;

		/**
		 * Contains the requested type of the variant.
		 */
		const Type requestedType;

		/**
		 * Constructor of the TypeException.
		 *
		 * @param actualType describes the actual type of the variant.
		 * @param requestedType describes the type in which the variant was
		 * requested.
		 */
		TypeException(Type actualType, Type requestedType);
	};

	using boolType = bool;
	using intType = int32_t;
	using doubleType = double;
	using stringType = std::string;
	using arrayType = std::vector<Variant>;
	using mapType = std::map<std::string, Variant>;
	using objectType = Managed *;
	using functionType = std::shared_ptr<Function>;

private:
	/**
	 * Used to store the actual type of the variant.
	 */
	Type type = Type::NULLPTR;

	/**
	 * Anonymous union containing the possible value of the variant.
	 */
	union {
		/**
		 * The boolean value. Only valid if type is Type::BOOL.
		 */
		boolType boolVal;
		/**
		 * The integer value. Only valid if type is Type::INT.
		 */
		intType intVal;
		/**
		 * The number value. Only valid if type is Type::DOUBLE.
		 */
		doubleType doubleVal;
		/**
		 * Pointer to the more complex data structures on the free store. Only
		 * valid if type is one of Type::STRING, Type::ARRAY,
		 * Type::MAP.
		 */
		void *ptrVal;
	};

	/**
	 * Internally used to convert the current pointer value to a reference of
	 * the specified type.
	 */
	template <typename T>
	T &asObj(Type requestedType) const
	{
		const Type actualType = getType();
		if (actualType == requestedType) {
			return *(static_cast<T *>(ptrVal));
		}
		throw TypeException{actualType, requestedType};
	}

	/**
	 * Function used to copy a reference to a managed object (not defined in the
	 * header to prevent an explicit reference to the Managed type).
	 *
	 * @param o is the pointer at the object that should be copied.
	 */
	void copyObject(objectType o);

	/**
	 * Function used internally to destroy a reference to a managed object (not
	 * defined in the header to prevent an explicit reference to the Managed
	 * type).
	 */
	void destroyObject();

	/**
	 * Used internally to assign the value of another Variant instance to this
	 * instance.
	 *
	 * @param v is the Variant instance that should be copied to this instance.
	 */
	void copy(const Variant &v)
	{
		destroy();
		type = v.type;
		switch (type) {
			case Type::NULLPTR:
				break;
			case Type::BOOL:
				boolVal = v.boolVal;
				break;
			case Type::INT:
				intVal = v.intVal;
				break;
			case Type::DOUBLE:
				doubleVal = v.doubleVal;
				break;
			case Type::STRING:
				ptrVal = new stringType(v.asString());
				break;
			case Type::ARRAY:
				ptrVal = new arrayType(v.asArray());
				break;
			case Type::MAP:
				ptrVal = new mapType(v.asMap());
				break;
			case Type::OBJECT:
				copyObject(v.asObject());
				break;
			case Type::FUNCTION:
				ptrVal = new functionType(v.asFunction());
				break;
		}
	}

	/**
	 * Used internally to move the value of another Variant instance to this
	 * instance.
	 *
	 * @param v is the Variant instance that should be copied to this instance.
	 */
	void move(Variant &&v)
	{
		destroy();
		type = v.type;
		switch (type) {
			case Type::NULLPTR:
				break;
			case Type::BOOL:
				boolVal = v.boolVal;
				break;
			case Type::INT:
				intVal = v.intVal;
				break;
			case Type::DOUBLE:
				doubleVal = v.doubleVal;
				break;
			case Type::STRING:
			case Type::ARRAY:
			case Type::MAP:
			case Type::OBJECT:
			case Type::FUNCTION:
				ptrVal = v.ptrVal;
				v.ptrVal = nullptr;
				break;
		}
		v.type = Type::NULLPTR;
	}

	/**
	 * Used internally to destroy any value that was allocated on the heap.
	 */
	void destroy()
	{
		if (ptrVal) {
			switch (type) {
				case Type::STRING:
					delete static_cast<stringType *>(ptrVal);
					break;
				case Type::ARRAY:
					delete static_cast<arrayType *>(ptrVal);
					break;
				case Type::MAP:
					delete static_cast<mapType *>(ptrVal);
					break;
				case Type::OBJECT:
					destroyObject();
					break;
				case Type::FUNCTION:
					delete static_cast<functionType *>(ptrVal);
					break;
				default:
					break;
			}
#ifndef NDEBUG
			ptrVal = nullptr;
#endif
		}
	}

public:
	/**
	 * Copy constructor of the Variant class.
	 *
	 * @param v is the Variant instance that should be cloned.
	 */
	Variant(const Variant &v) : ptrVal(nullptr) { copy(v); }

	/**
	 * Move constructor of the Variant class.
	 *
	 * @param v is the reference to the Variant instance that should be moved,
	 * this instance is invalidated afterwards.
	 */
	Variant(Variant &&v) : ptrVal(nullptr) { move(std::move(v)); }

	/**
	 * Default constructor. Type is set to Type:null.
	 */
	Variant() : ptrVal(nullptr) { setNull(); }

	/**
	 * Default destructor, frees any memory that was allocated on the heap.
	 */
	~Variant() { destroy(); }

	/**
	 * Constructor for null values. Initializes the variant as null value.
	 */
	Variant(std::nullptr_t) : ptrVal(nullptr) { setNull(); }

	/**
	 * Constructor for boolean values.
	 *
	 * @param b boolean value.
	 */
	Variant(boolType b) : ptrVal(nullptr) { setBool(b); }

	/**
	 * Constructor for integer values.
	 *
	 * @param i integer value.
	 */
	Variant(intType i) : ptrVal(nullptr) { setInt(i); }

	/**
	 * Constructor for double values.
	 *
	 * @param d double value.
	 */
	Variant(doubleType d) : ptrVal(nullptr) { setDouble(d); }

	/**
	 * Constructor for string values. The given string is copied and managed by
	 * the new Variant instance.
	 *
	 * @param s is a reference to a C-Style string used as string value.
	 */
	Variant(const char *s) : ptrVal(nullptr) { setString(s); }

	/**
	 * Constructor for array values. The given array is copied and managed by
	 * the new Variant instance.
	 *
	 * @param a is a reference to the array
	 */
	Variant(arrayType a) : ptrVal(nullptr) { setArray(std::move(a)); }

	/**
	 * Constructor for map values. The given map is copied and managed by the
	 * new Variant instance.
	 *
	 * @param m is a reference to the map.
	 */
	Variant(mapType m) : ptrVal(nullptr) { setMap(std::move(m)); }

	/**
	 * Constructor for storing managed objects. The reference at the managed
	 * object is stored as a Rooted object.
	 *
	 * @param o is a reference to the object.
	 */
	Variant(objectType o) : ptrVal(nullptr) { setObject(o); }

	/**
	 * Copy assignment operator.
	 */
	Variant &operator=(const Variant &v)
	{
		copy(v);
		return *this;
	}

	/**
	 * Move assignment operator.
	 */
	Variant &operator=(Variant &&v)
	{
		move(std::move(v));
		return *this;
	}

	/**
	 * Assign nullptr_t operator (allows to write Variant v = nullptr).
	 *
	 * @param p is an instance of std::nullptr_t.
	 */
	Variant &operator=(std::nullptr_t)
	{
		setNull();
		return *this;
	}

	/**
	 * Assign a boolean value.
	 *
	 * @param b is the boolean value to which the variant should be set.
	 */
	Variant &operator=(boolType b)
	{
		setBool(b);
		return *this;
	}

	/**
	 * Assign an integer value.
	 *
	 * @param i is the integer value to which the variant should be set.
	 */
	Variant &operator=(intType i)
	{
		setInt(i);
		return *this;
	}

	/**
	 * Assign a double value.
	 *
	 * @param d is the double value to which the variant should be set.
	 */
	Variant &operator=(doubleType d)
	{
		setDouble(d);
		return *this;
	}

	/**
	 * Assign a zero terminated const char array.
	 *
	 * @param s is the zero terminated const char array to which the variant
	 * should be set.
	 */
	Variant &operator=(const char *s)
	{
		setString(s);
		return *this;
	}

	/**
	 * Checks whether this Variant instance represents the nullptr.
	 *
	 * @return true if the Variant instance represents the nullptr, false
	 * otherwise.
	 */
	bool isNull() const { return type == Type::NULLPTR; }

	/**
	 * Checks whether this Variant instance is a boolean.
	 *
	 * @return true if the Variant instance is a boolean, false otherwise.
	 */
	bool isBool() const { return type == Type::BOOL; }

	/**
	 * Checks whether this Variant instance is an integer.
	 *
	 * @return true if the Variant instance is an integer, false otherwise.
	 */
	bool isInt() const { return type == Type::INT; }

	/**
	 * Checks whether this Variant instance is a double.
	 *
	 * @return true if the Variant instance is a double, false otherwise.
	 */
	bool isDouble() const { return type == Type::DOUBLE; }

	/**
	 * Checks whether this Variant instance is a string.
	 *
	 * @return true if the Variant instance is a string, false otherwise.
	 */
	bool isString() const { return type == Type::STRING; }

	/**
	 * Checks whether this Variant instance is an array.
	 *
	 * @return true if the Variant instance is an array, false otherwise.
	 */
	bool isArray() const { return type == Type::ARRAY; }

	/**
	 * Checks whether this Variant instance is a map.
	 *
	 * @return true if the Variant instance is a map, false otherwise.
	 */
	bool isMap() const { return type == Type::MAP; }

	/**
	 * Checks whether this Variant instance is an object.
	 *
	 * @return true if the Variant instance is an object, false otherwise.
	 */
	bool isObject() const { return type == Type::OBJECT; }

	/**
	 * Checks whether this Variant instance is a function.
	 *
	 * @return true if the Variant instance is a function, false otherwise.
	 */
	bool isFunction() const { return type == Type::FUNCTION; }

	/**
	 * Checks whether this Variant instance is a primitive type.
	 *
	 * @return true if the Variant instance is a primitive type.
	 */
	bool isPrimitive() const
	{
		switch (type) {
			case Type::NULLPTR:
			case Type::BOOL:
			case Type::INT:
			case Type::DOUBLE:
			case Type::STRING:
				return true;
			default:
				return false;
		}
	}

	/**
	 * Returns the Variant boolean value. Performs no type conversion. Throws an
	 * exception if the underlying type is not a boolean.
	 *
	 * @return the boolean value.
	 */
	boolType asBool() const
	{
		if (isBool()) {
			return boolVal;
		}
		throw TypeException{getType(), Type::BOOL};
	}

	/**
	 * Returns the Variant integer value. Performs no type conversion. Throws an
	 * exception if the underlying type is not an integer.
	 *
	 * @return the integer value.
	 */
	intType asInt() const
	{
		if (isInt()) {
			return intVal;
		}
		throw TypeException{getType(), Type::INT};
	}

	/**
	 * Returns the Variant double value. Performs no type conversion. Throws an
	 * exception if the underlying type is not a double.
	 *
	 * @return the double value.
	 */
	doubleType asDouble() const
	{
		if (isDouble()) {
			return doubleVal;
		}
		throw TypeException{getType(), Type::DOUBLE};
	}

	/**
	 * Returns a const reference to the string value. Performs no type
	 * conversion. Throws an exception if the underlying type is not a string.
	 *
	 * @return the string value as const reference.
	 */
	const stringType &asString() const
	{
		return asObj<stringType>(Type::STRING);
	}

	/**
	 * Returns a const reference to the string value. Performs no type
	 * conversion. Throws an exception if the underlying type is not a string.
	 *
	 * @return the string value as reference.
	 */
	stringType &asString() { return asObj<stringType>(Type::STRING); }

	/**
	 * Returns a const reference to the array value. Performs no type
	 * conversion. Throws an exception if the underlying type is not an array.
	 *
	 * @return the array value as const reference.
	 */
	const arrayType &asArray() const { return asObj<arrayType>(Type::ARRAY); }

	/**
	 * Returns a const reference to the array value. Performs no type
	 * conversion. Throws an exception if the underlying type is not an array.
	 *
	 * @return the array value as reference.
	 */
	arrayType &asArray() { return asObj<arrayType>(Type::ARRAY); }

	/**
	 * Returns a const reference to the map value. Performs no type
	 * conversion. Throws an exception if the underlying type is not a map.
	 *
	 * @return the map value as const reference.
	 */
	const mapType &asMap() const { return asObj<mapType>(Type::MAP); }

	/**
	 * Returns a pointer pointing at the stored managed object. Performs no type
	 * conversion. Throws an exception if the underlying type is not a managed
	 * object.
	 *
	 * @return pointer at the stored managed object.
	 */
	objectType asObject()
	{
		if (isObject()) {
			return static_cast<objectType>(ptrVal);
		}
		throw TypeException(getType(), Type::OBJECT);
	}

	/**
	 * Returns a pointer pointing at the stored managed object. Performs no type
	 * conversion. Throws an exception if the underlying type is not a managed
	 * object.
	 *
	 * @return const pointer at the stored managed object.
	 */
	const objectType asObject() const
	{
		if (isObject()) {
			return static_cast<objectType>(ptrVal);
		}
		throw TypeException(getType(), Type::OBJECT);
	}

	/**
	 * Returns a reference to the map value. Performs no type conversion.
	 * Throws an exception if the underlying type is not a map.
	 *
	 * @return the map value as reference.
	 */
	mapType &asMap() { return asObj<mapType>(Type::MAP); }

	/**
	 * Returns a shared pointer pointing at the stored function object. Performs
	 * no type conversion. Throws an exception if the underlying type is not a
	 * function.
	 *
	 * @return pointer at the stored managed object.
	 */
	functionType &asFunction() { return asObj<functionType>(Type::FUNCTION); }

	/**
	 * Returns a shared pointer pointing at the stored function object. Performs
	 * no type conversion. Throws an exception if the underlying type is not a
	 * function.
	 *
	 * @return const pointer at the stored managed object.
	 */
	const functionType &asFunction() const
	{
		return asObj<functionType>(Type::FUNCTION);
	}

	/**
	 * Returns the value of the Variant as boolean, performs type conversion.
	 *
	 * @return the Variant value converted to a boolean value.
	 */
	boolType toBool() const;

	/**
	 * Returns the value of the Variant as integer, performs type conversion.
	 *
	 * @return the Variant value converted to an integer value.
	 */
	intType toInt() const;

	/**
	 * Returns the value of the Variant as double, performs type conversion.
	 *
	 * @return the Variant value converted to a double value.
	 */
	doubleType toDouble() const;

	/**
	 * Returns the value of the Variant as string, performs type conversion.
	 *
	 * @return the value of the variant as string.
	 * @param escape if set to true, adds double quotes to strings and escapes
	 * them properly (resulting in a more or less JSONesque output).
	 */
	stringType toString(bool escape = false) const;

	/**
	 * Sets the variant to null.
	 */
	void setNull()
	{
		destroy();
		type = Type::NULLPTR;
		ptrVal = nullptr;
	}

	/**
	 * Sets the variant to the given boolean value.
	 *
	 * @param b is the new boolean value.
	 */
	void setBool(boolType b)
	{
		destroy();
		type = Type::BOOL;
		boolVal = b;
	}

	/**
	 * Sets the variant to the given integer value.
	 *
	 * @param i is the new integer value.
	 */
	void setInt(intType i)
	{
		destroy();
		type = Type::INT;
		intVal = i;
	}

	/**
	 * Sets the variant to the given double value.
	 *
	 * @param d is the new double value.
	 */
	void setDouble(doubleType d)
	{
		destroy();
		type = Type::DOUBLE;
		doubleVal = d;
	}

	/**
	 * Sets the variant to the given string value.
	 *
	 * @param d is the new string value.
	 */
	void setString(const char *s)
	{
		if (isString()) {
			asString().assign(s);
		} else {
			destroy();
			type = Type::STRING;
			ptrVal = new stringType(s);
		}
	}

	/**
	 * Sets the variant to the given array value.
	 *
	 * @param a is the new array value.
	 */
	void setArray(arrayType a)
	{
		if (isArray()) {
			asArray().swap(a);
		} else {
			destroy();
			type = Type::ARRAY;
			ptrVal = new arrayType(std::move(a));
		}
	}

	/**
	 * Sets the variant to the given map value.
	 *
	 * @param a is the new map value.
	 */
	void setMap(mapType m)
	{
		if (isMap()) {
			asMap().swap(m);
		} else {
			destroy();
			type = Type::MAP;
			ptrVal = new mapType(std::move(m));
		}
	}

	/**
	 * Sets the variant to the given managed object. The variant is equivalent
	 * to a Rooted handle.
	 */
	void setObject(objectType o)
	{
		destroy();
		type = Type::OBJECT;
		copyObject(o);
	}

	/**
	 * Returns the current type of the Variant.
	 *
	 * @return the current type of the Variant.
	 */
	Type getType() const { return type; }

	/**
	 * Returns the name of the given variant type as C-style string.
	 */
	static const char *getTypeName(Type type);

	/**
	 * Returns the name of the type of this variant instance.
	 */
	const char *getTypeName() { return Variant::getTypeName(getType()); }

	/**
	 * Prints the Variant to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &os, const Variant &v)
	{
		return os << v.toString(true);
	}

	/**
	 * Prints a key value pair to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &os,
	                                const mapType::value_type &v)
	{
		// TODO: Use proper serialization function
		return os << "\"" << v.first << "\": " << v.second.toString(true);
	}

	/*
	 * Comprison operators.
	 */

	/**
	 * Returns true if the given left hand side is smaller than the right hand
	 * side. Uses the comparison algorithm of the stored object. Throws an
	 * exception if the types of the two variants are not equal.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is smaller than rhs.
	 */
	friend bool operator<(const Variant &lhs, const Variant &rhs)
	{
		// If the types do not match, we can not do a meaningful comparison.
		if (lhs.getType() != rhs.getType()) {
			throw TypeException(lhs.getType(), rhs.getType());
		}
		switch (lhs.getType()) {
			case Type::NULLPTR:
				return false;
			case Type::BOOL:
				return lhs.boolVal < rhs.boolVal;
			case Type::INT:
				return lhs.intVal < rhs.intVal;
			case Type::DOUBLE:
				return lhs.doubleVal < rhs.doubleVal;
			case Type::STRING:
				return lhs.asString() < rhs.asString();
			case Type::ARRAY:
				return lhs.asArray() < rhs.asArray();
			case Type::MAP:
				return lhs.asMap() < rhs.asMap();
			case Type::OBJECT:
				return lhs.asObject() < rhs.asObject();
			case Type::FUNCTION:
				return lhs.asFunction() < rhs.asFunction();
		}
		throw OusiaException("Internal Error! Unknown type!");
	}

	/**
	 * Returns true if the given left hand side is larger than the right hand
	 * side. Uses the comparison algorithm of the stored object. Throws an
	 * exception if the types of the two variants are not equal.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is larger than rhs.
	 */
	friend bool operator>(const Variant &lhs, const Variant &rhs)
	{
		return rhs < lhs;
	}

	/**
	 * Returns true if the given left hand side is smaller or equal to the
	 * right hand side. Uses the comparison algorithm of the stored object.
	 * Throws an exception if the types of the two variants are not equal.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is smaller than or equal to rhs.
	 */
	friend bool operator<=(const Variant &lhs, const Variant &rhs)
	{
		return !(lhs > rhs);
	}

	/**
	 * Returns true if the given left hand side is larger or equal to the
	 * right hand side. Uses the comparison algorithm of the stored object.
	 * Throws an exception if the types of the two variants are not equal.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is larger than or equal to rhs.
	 */
	friend bool operator>=(const Variant &lhs, const Variant &rhs)
	{
		return !(lhs < rhs);
	}

	/**
	 * Returns true if the given left hand side and right hand side are equal.
	 * Uses the comparison algorithm of the stored object. Returns false if the
	 * two variants do not have the same type.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs equals rhs.
	 */
	friend bool operator==(const Variant &lhs, const Variant &rhs)
	{
		if (lhs.getType() != rhs.getType()) {
			return false;
		}
		switch (lhs.getType()) {
			case Type::NULLPTR:
				return true;
			case Type::BOOL:
				return lhs.boolVal == rhs.boolVal;
			case Type::INT:
				return lhs.intVal == rhs.intVal;
			case Type::DOUBLE:
				return lhs.doubleVal == rhs.doubleVal;
			case Type::STRING:
				return lhs.asString() == rhs.asString();
			case Type::ARRAY:
				return lhs.asArray() == rhs.asArray();
			case Type::MAP:
				return lhs.asMap() == rhs.asMap();
			case Type::OBJECT:
				return lhs.asObject() == rhs.asObject();
			case Type::FUNCTION:
				return lhs.asFunction() == rhs.asFunction();
		}
		throw OusiaException("Internal Error! Unknown type!");
	}

	/**
	 * Returns true if the given left hand side are equal. Uses the comparison
	 * algorithm of the stored object. Returns true if the two variants do not
	 * have the same type.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is not equal to rhs.
	 */
	friend bool operator!=(const Variant &lhs, const Variant &rhs)
	{
		return !(lhs == rhs);
	}
};
}

#endif /* _OUSIA_VARIANT_HPP_ */

