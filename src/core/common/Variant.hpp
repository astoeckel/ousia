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

#include <core/managed/Managed.hpp>

#include "Exceptions.hpp"

namespace ousia {

// Forward declarations
class Function;
class RttiType;

/**
 * Enum containing the possible types a variant may have.
 */
enum class VariantType : int16_t {
	NULLPTR,
	BOOL,
	INT,
	DOUBLE,
	STRING,
	MAGIC,
	ARRAY,
	MAP,
	OBJECT,
	FUNCTION
};

/**
 * Instances of the Variant class represent any kind of data that is exchanged
 * between the host application and the script engine. Variants are immutable.
 */
class Variant {
public:
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
		const VariantType actualType;

		/**
		 * Contains the requested type of the variant.
		 */
		const VariantType requestedType;

		/**
		 * Constructor of the TypeException.
		 *
		 * @param actualType describes the actual type of the variant.
		 * @param requestedType describes the type in which the variant was
		 * requested.
		 */
		TypeException(VariantType actualType, VariantType requestedType);
	};

	using boolType = bool;
	using intType = int32_t;
	using doubleType = double;
	using stringType = std::string;
	using arrayType = std::vector<Variant>;
	using mapType = std::map<std::string, Variant>;
	using objectType = Rooted<Managed>;
	using functionType = std::shared_ptr<Function>;

private:
	/**
	 * Used to store the actual type of the variant.
	 */
	VariantType type = VariantType::NULLPTR;

	/**
	 * Anonymous union containing the possible value of the variant.
	 */
	union {
		/**
		 * The boolean value. Only valid if type is VariantType::BOOL.
		 */
		boolType boolVal;
		/**
		 * The integer value. Only valid if type is VariantType::INT.
		 */
		intType intVal;
		/**
		 * The number value. Only valid if type is VariantType::DOUBLE.
		 */
		doubleType doubleVal;
		/**
		 * Pointer to the more complex data structures on the free store. Only
		 * valid if type is one of VariantType::STRING, VariantType::ARRAY,
		 * VariantType::MAP.
		 */
		void *ptrVal;
	};

	/**
	 * Internally used to convert the current pointer value to a reference of
	 * the specified type.
	 */
	template <typename T>
	T &asObj(VariantType requestedType) const
	{
		const VariantType actualType = getType();
		if (actualType == requestedType) {
			return *(static_cast<T *>(ptrVal));
		}
		throw TypeException{actualType, requestedType};
	}

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
			case VariantType::NULLPTR:
				break;
			case VariantType::BOOL:
				boolVal = v.boolVal;
				break;
			case VariantType::INT:
				intVal = v.intVal;
				break;
			case VariantType::DOUBLE:
				doubleVal = v.doubleVal;
				break;
			case VariantType::STRING:
			case VariantType::MAGIC:
				ptrVal = new stringType(v.asString());
				break;
			case VariantType::ARRAY:
				ptrVal = new arrayType(v.asArray());
				break;
			case VariantType::MAP:
				ptrVal = new mapType(v.asMap());
				break;
			case VariantType::OBJECT:
				ptrVal = new objectType(v.asObject());
				break;
			case VariantType::FUNCTION:
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
			case VariantType::NULLPTR:
				break;
			case VariantType::BOOL:
				boolVal = v.boolVal;
				break;
			case VariantType::INT:
				intVal = v.intVal;
				break;
			case VariantType::DOUBLE:
				doubleVal = v.doubleVal;
				break;
			case VariantType::STRING:
			case VariantType::MAGIC:
			case VariantType::ARRAY:
			case VariantType::MAP:
			case VariantType::OBJECT:
			case VariantType::FUNCTION:
				ptrVal = v.ptrVal;
				v.ptrVal = nullptr;
				break;
		}
		v.type = VariantType::NULLPTR;
	}

	/**
	 * Used internally to destroy any value that was allocated on the heap.
	 */
	void destroy()
	{
		if (ptrVal) {
			switch (type) {
				case VariantType::STRING:
				case VariantType::MAGIC:
					delete static_cast<stringType *>(ptrVal);
					break;
				case VariantType::ARRAY:
					delete static_cast<arrayType *>(ptrVal);
					break;
				case VariantType::MAP:
					delete static_cast<mapType *>(ptrVal);
					break;
				case VariantType::OBJECT:
					delete static_cast<objectType *>(ptrVal);
					break;
				case VariantType::FUNCTION:
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
	 * Default constructor. VariantType is set to VariantType:NULLPTR.
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
	 * Named constructor for function values.
	 *
	 * @param f is a shared pointer pointing at the Function instance.
	 */
	static Variant fromFunction(const functionType &f)
	{
		Variant res;
		res.setFunction(f);
		return res;
	}

	/**
	 * Named constructor for strings values.
	 *
	 * @param s is the std::string from which the variant should be constructed.
	 */
	static Variant fromString(const stringType &s)
	{
		Variant res;
		res.setString(s.c_str());
		return res;
	}

	/**
	 * Constructor for storing managed objects. The reference at the managed
	 * object is stored as a Rooted object.
	 *
	 * @param o is a reference to the object.
	 */
	template <class T>
	Variant(Handle<T> o)
	    : ptrVal(nullptr)
	{
		setObject(o);
	}

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
	bool isNull() const { return type == VariantType::NULLPTR; }

	/**
	 * Checks whether this Variant instance is a boolean.
	 *
	 * @return true if the Variant instance is a boolean, false otherwise.
	 */
	bool isBool() const { return type == VariantType::BOOL; }

	/**
	 * Checks whether this Variant instance is an integer.
	 *
	 * @return true if the Variant instance is an integer, false otherwise.
	 */
	bool isInt() const { return type == VariantType::INT; }

	/**
	 * Checks whether this Variant instance is a double.
	 *
	 * @return true if the Variant instance is a double, false otherwise.
	 */
	bool isDouble() const { return type == VariantType::DOUBLE; }

	/**
	 * Checks whether this Variant instance is a string or a magic string.
	 *
	 * @return true if the Variant instance is a string, false otherwise.
	 */
	bool isString() const
	{
		return type == VariantType::STRING || type == VariantType::MAGIC;
	}

	/**
	 * Checks whether this Variant instance is a magic string. Magic strings
	 * are created if a unquoted string is parsed and may e.g. be treated as
	 * constants.
	 *
	 * @return true if the Variant instance is a string, false otherwise.
	 */
	bool isMagic() const { return type == VariantType::MAGIC; }

	/**
	 * Checks whether this Variant instance is an array.
	 *
	 * @return true if the Variant instance is an array, false otherwise.
	 */
	bool isArray() const { return type == VariantType::ARRAY; }

	/**
	 * Checks whether this Variant instance is a map.
	 *
	 * @return true if the Variant instance is a map, false otherwise.
	 */
	bool isMap() const { return type == VariantType::MAP; }

	/**
	 * Checks whether this Variant instance is an object.
	 *
	 * @return true if the Variant instance is an object, false otherwise.
	 */
	bool isObject() const { return type == VariantType::OBJECT; }

	/**
	 * Checks whether this Variant instance is a function.
	 *
	 * @return true if the Variant instance is a function, false otherwise.
	 */
	bool isFunction() const { return type == VariantType::FUNCTION; }

	/**
	 * Checks whether this Variant instance is a primitive type.
	 *
	 * @return true if the Variant instance is a primitive type.
	 */
	bool isPrimitive() const
	{
		switch (type) {
			case VariantType::NULLPTR:
			case VariantType::BOOL:
			case VariantType::INT:
			case VariantType::DOUBLE:
			case VariantType::STRING:
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
		throw TypeException{getType(), VariantType::BOOL};
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
		throw TypeException{getType(), VariantType::INT};
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
		throw TypeException{getType(), VariantType::DOUBLE};
	}

	/**
	 * Returns a const reference to the string value. Performs no type
	 * conversion. Throws an exception if the underlying type is not a string.
	 *
	 * @return the string value as const reference.
	 */
	const stringType &asString() const
	{
		return asObj<stringType>(VariantType::STRING);
	}

	/**
	 * Returns a reference to the string value. Performs no type conversion.
	 * Throws an exception if the underlying type is not a string.
	 *
	 * @return the string value as reference.
	 */
	stringType &asString() { return asObj<stringType>(VariantType::STRING); }

	/**
	 * Returns a const reference to the magic string value. Performs no type
	 * conversion. Throws an exception if the underlying type is not a magic
	 * string.
	 *
	 * @return the magic string value as const reference.
	 */
	const stringType &asMagic() const
	{
		if (type == VariantType::MAGIC) {
			return asObj<stringType>(VariantType::STRING);
		}
		throw TypeException{getType(), VariantType::MAGIC};
	}

	/**
	 * Returns a reference to the magic string value. Performs no type
	 * conversion. Throws an exception if the underlying type is not a magic
	 * string.
	 *
	 * @return the magic string value as const reference.
	 */
	stringType &asMagic()
	{
		if (type == VariantType::MAGIC) {
			return asObj<stringType>(VariantType::STRING);
		}
		throw TypeException{getType(), VariantType::MAGIC};
	}

	/**
	 * Returns a const reference to the array value. Performs no type
	 * conversion. Throws an exception if the underlying type is not an array.
	 *
	 * @return the array value as const reference.
	 */
	const arrayType &asArray() const
	{
		return asObj<arrayType>(VariantType::ARRAY);
	}

	/**
	 * Returns a const reference to the array value. Performs no type
	 * conversion. Throws an exception if the underlying type is not an array.
	 *
	 * @return the array value as reference.
	 */
	arrayType &asArray() { return asObj<arrayType>(VariantType::ARRAY); }

	/**
	 * Returns a const reference to the map value. Performs no type
	 * conversion. Throws an exception if the underlying type is not a map.
	 *
	 * @return the map value as const reference.
	 */
	const mapType &asMap() const { return asObj<mapType>(VariantType::MAP); }

	/**
	 * Returns a pointer pointing at the stored managed object. Performs no type
	 * conversion. Throws an exception if the underlying type is not a managed
	 * object.
	 *
	 * @return pointer at the stored managed object.
	 */
	objectType asObject() { return asObj<objectType>(VariantType::OBJECT); }

	/**
	 * Returns a pointer pointing at the stored managed object. Performs no type
	 * conversion. Throws an exception if the underlying type is not a managed
	 * object.
	 *
	 * @return const pointer at the stored managed object.
	 */
	const objectType asObject() const
	{
		return asObj<objectType>(VariantType::OBJECT);
	}

	/**
	 * Returns a reference to the map value. Performs no type conversion.
	 * Throws an exception if the underlying type is not a map.
	 *
	 * @return the map value as reference.
	 */
	mapType &asMap() { return asObj<mapType>(VariantType::MAP); }

	/**
	 * Returns a shared pointer pointing at the stored function object. Performs
	 * no type conversion. Throws an exception if the underlying type is not a
	 * function.
	 *
	 * @return pointer at the stored managed object.
	 */
	functionType &asFunction()
	{
		return asObj<functionType>(VariantType::FUNCTION);
	}

	/**
	 * Returns a shared pointer pointing at the stored function object. Performs
	 * no type conversion. Throws an exception if the underlying type is not a
	 * function.
	 *
	 * @return const pointer at the stored managed object.
	 */
	const functionType &asFunction() const
	{
		return asObj<functionType>(VariantType::FUNCTION);
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
	 */
	stringType toString() const;

	/**
	 * Returns the value of the Variant as array, performs type conversion. If
	 * the variant is not an array yet, the current value is inserted into a
	 * one-element array.
	 *
	 * @return the value of the variant as array.
	 */
	arrayType toArray() const;

	/**
	 * Returns the value of the Variant as array, performs type conversion. If
	 * the variant is not an array yet, the current value is inserted into a
	 * one-element array.
	 *
	 * @param innerType is the inner type the array entries should be converted
	 * to.
	 * @return the value of the variant as array.
	 */
	arrayType toArray(const RttiType &innerType) const;

	/**
	 * Returns the value of the Variant as map.
	 *
	 * @return the value of the variant as map.
	 */
	mapType toMap() const;

	/**
	 * Returns the value of the Variant as map, performs type conversion of the
	 * map entries to the given inner type.
	 *
	 * @param innerType is the inner type the map entries should be converted
	 * to.
	 * @return the value of the variant as map.
	 */
	mapType toMap(const RttiType &innerType) const;

	/**
	 * Sets the variant to null.
	 */
	void setNull()
	{
		destroy();
		type = VariantType::NULLPTR;
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
		type = VariantType::BOOL;
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
		type = VariantType::INT;
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
		type = VariantType::DOUBLE;
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
			type = VariantType::STRING;
			asString().assign(s);
		} else {
			destroy();
			type = VariantType::STRING;
			ptrVal = new stringType(s);
		}
	}

	/**
	 * Sets the variant to the given magic string value.
	 *
	 * @param d is the new magic string value.
	 */
	void setMagic(const char *s)
	{
		if (isString()) {
			type = VariantType::MAGIC;
			asString().assign(s);
		} else {
			destroy();
			type = VariantType::MAGIC;
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
			type = VariantType::ARRAY;
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
			type = VariantType::MAP;
			ptrVal = new mapType(std::move(m));
		}
	}

	/**
	 * Sets the variant to the given managed object. The variant is equivalent
	 * to a Rooted handle.
	 */
	template <class T>
	void setObject(Handle<T> o)
	{
		destroy();
		type = VariantType::OBJECT;
		ptrVal = new objectType(o);
	}

	/**
	 * Sets the variant to the given function.
	 *
	 * @param f is a std::shared_ptr pointing at a instance of the Function
	 * class the Variant should be set to.
	 */
	void setFunction(functionType f)
	{
		destroy();
		type = VariantType::FUNCTION;
		ptrVal = new functionType(f);
	}

	/**
	 * Returns the current type of the Variant.
	 *
	 * @return the current type of the Variant.
	 */
	VariantType getType() const
	{
		if (isMagic()) {
			return VariantType::STRING;
		}
		return type;
	}

	/**
	 * Returns the current Rtti type descriptor of the Variant.
	 *
	 * @return the Rtti type descriptor. Either one of RttiTypes::Int,
	 * RttiTypes::Bool, RttiTypes::Double, RttiTypes::String, RttiTypes::Array
	 * or RttiTypes::Function or -- in case an object is stored inside the
	 * variant -- the RttiType of that object.
	 */
	const RttiType &getRttiType() const;

	/**
	 * Returns the name of the given variant type as C-style string.
	 */
	static const char *getTypeName(VariantType type);

	/**
	 * Returns the name of the type of this variant instance.
	 */
	const char *getTypeName() const { return Variant::getTypeName(getType()); }

	/*
	 * Output stream operator.
	 */

	/**
	 * Prints the Variant to the output stream as JSON data.
	 *
	 * @param os is the output stream the variant should be written to.
	 * @param v is the variant that should be written to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &os, const Variant &v);

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
	friend bool operator<(const Variant &lhs, const Variant &rhs);

	/**
	 * Returns true if the given left hand side is larger than the right hand
	 * side. Uses the comparison algorithm of the stored object. Throws an
	 * exception if the types of the two variants are not equal.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is larger than rhs.
	 */
	friend bool operator>(const Variant &lhs, const Variant &rhs);

	/**
	 * Returns true if the given left hand side is smaller or equal to the
	 * right hand side. Uses the comparison algorithm of the stored object.
	 * Throws an exception if the types of the two variants are not equal.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is smaller than or equal to rhs.
	 */
	friend bool operator<=(const Variant &lhs, const Variant &rhs);

	/**
	 * Returns true if the given left hand side is larger or equal to the
	 * right hand side. Uses the comparison algorithm of the stored object.
	 * Throws an exception if the types of the two variants are not equal.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is larger than or equal to rhs.
	 */
	friend bool operator>=(const Variant &lhs, const Variant &rhs);

	/**
	 * Returns true if the given left hand side and right hand side are equal.
	 * Uses the comparison algorithm of the stored object. Returns false if the
	 * two variants do not have the same type.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs equals rhs.
	 */
	friend bool operator==(const Variant &lhs, const Variant &rhs);

	/**
	 * Returns true if the given left hand side are equal. Uses the comparison
	 * algorithm of the stored object. Returns true if the two variants do not
	 * have the same type.
	 *
	 * @param lhs is the left hand side of the comparison.
	 * @param rhs is the right hand side of the comparison.
	 * @return true if lhs is not equal to rhs.
	 */
	friend bool operator!=(const Variant &lhs, const Variant &rhs);
};
}

#endif /* _OUSIA_VARIANT_HPP_ */

