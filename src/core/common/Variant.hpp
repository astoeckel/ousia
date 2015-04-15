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
#include <limits>
#include <string>
#include <vector>
#include <ostream>

// TODO: Use
// http://nikic.github.io/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html
// later (will allow to use 8 bytes for a variant)

#include <core/RangeSet.hpp>
#include <core/managed/Managed.hpp>

#include "Exceptions.hpp"

namespace ousia {

// Forward declarations
class Function;
class Rtti;
class SourceLocation;
class ManagedVariant;

/**
 * Enum containing the possible types a variant may have.
 */
enum class VariantType : uint8_t {
	BOOL = 1,
	INT = 2,
	DOUBLE = 3,
	STRING = 4,
	MAGIC = 5,
	ARRAY = 6,
	MAP = 7,
	OBJECT = 8,
	CARDINALITY = 9,
	FUNCTION = 10,
	NULLPTR = 15
};

#pragma pack(push, 1)
/**
 * Structure used to store the type of a variant and the location at which it
 * was found in 8 Bytes.
 */
struct VariantMetadata {
	/**
	 * Structure holding the actual metadata.
	 */
	struct Meta {
		/**
		 * Field used to store the type of a Variant (4 Bit, space for 16
		 * objects).
		 */
		uint8_t variantType : 4;

		/**
		 * Field used to store the location at which the Variant was found
		 * (30 Bit).
		 */
		uint32_t locationOffset : 30;  // Enough for 1GB

		/**
		 * Field used to store the length of the value from which the
		 * variant was parsed (14 Bit).
		 */
		uint16_t locationLength : 14;  // 16.000 Bytes of context

		/**
		 * Unique id of the file from which the variant was parsed.
		 */
		uint16_t locationSourceId : 16;  // 65.000 Source files
	};

	union {
		/**
		 * The actual metadata.
		 */
		Meta data;

		/**
		 * The metadata as 64 Bit value.
		 */
		uint64_t intData;
	};

	/**
	 * Maximum byte offset for locations that can be stored.
	 */
	static constexpr uint32_t InvalidLocationOffset = 0x3FFFFFFF;

	/**
	 * Maximum length for locations that can be sotred.
	 */
	static constexpr uint16_t InvalidLocationLength = 0x3FFF;

	/**
	 * Maximum source id that can be stored.
	 */
	static constexpr uint16_t InvalidLocationSourceId = 0xFFFF;

	/**
	 * Default constructor. Sets the type to nullptr and all other fields to
	 * invalid.
	 */
	VariantMetadata() { intData = std::numeric_limits<uint64_t>::max(); }

	/**
	 * Sets the type to the given type and all other fields to invalid.
	 *
	 * @param type is the type of the variant.
	 */
	VariantMetadata(VariantType type) : VariantMetadata()
	{
		data.variantType = static_cast<uint8_t>(type);
	}

	/**
	 * Returns the internally stored type.
	 *
	 * @return the variant type.
	 */
	VariantType getType() const
	{
		return static_cast<VariantType>(data.variantType);
	}

	/**
	 * Sets the type to the given value.
	 *
	 * @param type is the variant type that should be stored.
	 */
	void setType(VariantType type)
	{
		data.variantType = static_cast<uint8_t>(type);
	}

	/**
	 * Returns true if the stored source id is not invalid.
	 *
	 * @retun true if the
	 */
	bool hasLocation() const;

	/**
	 * Unpacks ans returns the stored source location. Note that the returned
	 * location may differ from the one given in "setLocation", if the values
	 * were too large to represent.
	 *
	 * @return the stored SourceLocation.
	 */
	SourceLocation getLocation() const;

	/**
	 * Packs the given source location and stores it in the metadata. Not all
	 * SourceLocation values may be representable, as they are stored with fewer
	 * bits as in the SourceLocation structure.
	 *
	 * @param location is the SourceLocation that should be stored.
	 */
	void setLocation(const SourceLocation &location);
};
#pragma pack(pop)

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
	using objectType = Owned<Managed>;
	using cardinalityType = Cardinality;
	using rangeType = Range<size_t>;
	using functionType = std::shared_ptr<Function>;

private:
	/**
	 * Used to store the actual type of the variant and the location from which
	 * the variant was parsed.
	 */
	VariantMetadata meta;

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
		meta = v.meta;
		switch (meta.getType()) {
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
			case VariantType::CARDINALITY:
				ptrVal = new cardinalityType(v.asCardinality());
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
	void move(Variant &&v) noexcept
	{
		destroy();
		meta = v.meta;
		switch (meta.getType()) {
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
			case VariantType::CARDINALITY:
			case VariantType::FUNCTION:
				ptrVal = v.ptrVal;
				v.ptrVal = nullptr;
				break;
		}
		v.meta.setType(VariantType::NULLPTR);
	}

	/**
	 * Used internally to destroy any value that was allocated on the heap.
	 */
	void destroy()
	{
		if (ptrVal) {
			switch (meta.getType()) {
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
				case VariantType::CARDINALITY:
					delete static_cast<cardinalityType *>(ptrVal);
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
	Variant(Variant &&v) noexcept : ptrVal(nullptr) { move(std::move(v)); }

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
	 * Named constructor for object values.
	 *
	 * @param o is an owned handle that should be copied.
	 */
	template <class T>
	static Variant fromObject(Owned<T> o)
	{
		Variant res;
		res.setObject(o);
		return res;
	}

	/**
	 * Named constructor for object values.
	 *
	 * @param o is an object that can be converted to an Owned handle.
	 * @param owner is the owner of the object handle. If nullptr is given, the
	 * Owned handle will behave like a Rooted handle.
	 */
	template <class T>
	static Variant fromObject(T o, Managed *owner = nullptr)
	{
		Variant res;
		res.setObject(o, owner);
		return res;
	}

	/**
	 * Constructor for cardinality values. The given cardinality is copied and
	 *managed by the
	 * new Variant instance.
	 *
	 * @param c is a reference to the cardinality.
	 */
	Variant(cardinalityType c) : ptrVal(nullptr)
	{
		setCardinality(std::move(c));
	}

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
	Variant &operator=(Variant &&v) noexcept
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
	bool isNull() const { return meta.getType() == VariantType::NULLPTR; }

	/**
	 * Checks whether this Variant instance is a boolean.
	 *
	 * @return true if the Variant instance is a boolean, false otherwise.
	 */
	bool isBool() const { return meta.getType() == VariantType::BOOL; }

	/**
	 * Checks whether this Variant instance is an integer.
	 *
	 * @return true if the Variant instance is an integer, false otherwise.
	 */
	bool isInt() const { return meta.getType() == VariantType::INT; }

	/**
	 * Checks whether this Variant instance is a double.
	 *
	 * @return true if the Variant instance is a double, false otherwise.
	 */
	bool isDouble() const { return meta.getType() == VariantType::DOUBLE; }

	/**
	 * Checks whether this Variant instance is a string or a magic string.
	 *
	 * @return true if the Variant instance is a string, false otherwise.
	 */
	bool isString() const
	{
		return meta.getType() == VariantType::STRING ||
		       meta.getType() == VariantType::MAGIC;
	}

	/**
	 * Checks whether this Variant instance is a magic string. Magic strings
	 * are created if a unquoted string is parsed and may e.g. be treated as
	 * constants.
	 *
	 * @return true if the Variant instance is a string, false otherwise.
	 */
	bool isMagic() const { return meta.getType() == VariantType::MAGIC; }

	/**
	 * Checks whether this Variant instance is an array.
	 *
	 * @return true if the Variant instance is an array, false otherwise.
	 */
	bool isArray() const { return meta.getType() == VariantType::ARRAY; }

	/**
	 * Checks whether this Variant instance is a map.
	 *
	 * @return true if the Variant instance is a map, false otherwise.
	 */
	bool isMap() const { return meta.getType() == VariantType::MAP; }

	/**
	 * Checks whether this Variant instance is an object.
	 *
	 * @return true if the Variant instance is an object, false otherwise.
	 */
	bool isObject() const { return meta.getType() == VariantType::OBJECT; }

	/**
	 * Checks whether this Variant instance is a cardinality.
	 *
	 * @return true if the Variant instance is an cardinality, false otherwise.
	 */
	bool isCardinality() const
	{
		return meta.getType() == VariantType::CARDINALITY;
	}

	/**
	 * Checks whether this Variant instance is a function.
	 *
	 * @return true if the Variant instance is a function, false otherwise.
	 */
	bool isFunction() const { return meta.getType() == VariantType::FUNCTION; }

	/**
	 * Checks whether this Variant instance is a primitive type.
	 *
	 * @return true if the Variant instance is a primitive type.
	 */
	bool isPrimitive() const
	{
		switch (meta.getType()) {
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
		if (meta.getType() == VariantType::MAGIC) {
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
		if (meta.getType() == VariantType::MAGIC) {
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
	 * Returns a reference to the map value. Performs no type conversion.
	 * Throws an exception if the underlying type is not a map.
	 *
	 * @return the map value as reference.
	 */
	mapType &asMap() { return asObj<mapType>(VariantType::MAP); }

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
	 * Returns a pointer pointing at the stored managed object. Performs no type
	 * conversion. Throws an exception if the underlying type is not a managed
	 * object.
	 *
	 * @return pointer at the stored managed object.
	 */
	objectType asObject() { return asObj<objectType>(VariantType::OBJECT); }

	/**
	 * Returns a reference to the cardinality value. Performs no type
	 * conversion.
	 * Throws an exception if the underlying type is not a cardinality.
	 *
	 * @return the cardinality value as reference.
	 */
	const cardinalityType &asCardinality() const
	{
		return asObj<cardinalityType>(VariantType::CARDINALITY);
	}

	/**
	 * Returns a reference to the cardinality value. Performs no type
	 * conversion.
	 * Throws an exception if the underlying type is not a cardinality.
	 *
	 * @return the cardinality value as reference.
	 */
	cardinalityType &asCardinality()
	{
		return asObj<cardinalityType>(VariantType::CARDINALITY);
	}

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
	 * If the value of the variant already is a string, the markAsMagic function
	 * marks this string as a "magic" value (a variant which might also be an
	 * identifier). Throws an exception if the variant is not a string or magic
	 * value.
	 */
	void markAsMagic()
	{
		if (getType() == VariantType::STRING) {
			meta.setType(VariantType::MAGIC);
			return;
		}
		throw TypeException{getType(), VariantType::STRING};
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
	arrayType toArray(const Rtti *innerType) const;

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
	mapType toMap(const Rtti *innerType) const;

	/**
	 * Returns the value of the Variant as cardinality.
	 *
	 * @return the value of the variant as cardinality.
	 */
	cardinalityType toCardinality() const;

	/**
	 * Returns the Variant as a new ManagedVariant instance.
	 *
	 * @param mgr is the Manager instance the returned variant should belong to.
	 * @return a new ManagedVariant instance containing the value of this
	 * Variant instance.
	 */
	Rooted<ManagedVariant> toManaged(Manager &mgr) const;

	/**
	 * Sets the variant to null.
	 */
	void setNull()
	{
		destroy();
		meta.setType(VariantType::NULLPTR);
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
		meta.setType(VariantType::BOOL);
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
		meta.setType(VariantType::INT);
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
		meta.setType(VariantType::DOUBLE);
		doubleVal = d;
	}

	/**
	 * Sets the variant to the given string value.
	 *
	 * @param s is the new string value.
	 */
	void setString(const char *s)
	{
		if (isString()) {
			meta.setType(VariantType::STRING);
			asString().assign(s);
		} else {
			destroy();
			meta.setType(VariantType::STRING);
			ptrVal = new stringType(s);
		}
	}

	/**
	 * Sets the variant to the given magic string value.
	 *
	 * @param s is the new magic string value.
	 */
	void setMagic(const char *s)
	{
		if (isString()) {
			meta.setType(VariantType::MAGIC);
			asString().assign(s);
		} else {
			destroy();
			meta.setType(VariantType::MAGIC);
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
			meta.setType(VariantType::ARRAY);
			ptrVal = new arrayType(std::move(a));
		}
	}

	/**
	 * Sets the variant to the given map value.
	 *
	 * @param m is the new map value.
	 */
	void setMap(mapType m)
	{
		if (isMap()) {
			asMap().swap(m);
		} else {
			destroy();
			meta.setType(VariantType::MAP);
			ptrVal = new mapType(std::move(m));
		}
	}

	/**
	 * Sets the variant to the given owned handle.
	 *
	 * @param o is the owned handle that should be copied.
	 */
	template <class T>
	void setObject(const Owned<T> &o)
	{
		destroy();
		meta.setType(VariantType::OBJECT);
		ptrVal = new objectType(o);
	}

	/**
	 * Sets the variant to the given object and an optional owner.
	 *
	 * @param o is an object that can be converted to a Handle.
	 * @param owner is an optional owner of o. The object is guaranteed to live
	 * as long as the owner is alive.
	 */
	template <class T>
	void setObject(T o, Managed *owner = nullptr)
	{
		destroy();
		meta.setType(VariantType::OBJECT);
		ptrVal = new objectType(o, owner);
	}

	/**
	 * Sets the variant to the given cardinality value.
	 *
	 * @param c is the new cardinality value.
	 */
	void setCardinality(cardinalityType c)
	{
		destroy();
		meta.setType(VariantType::CARDINALITY);
		ptrVal = new cardinalityType(std::move(c));
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
		meta.setType(VariantType::FUNCTION);
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
		return meta.getType();
	}

	/**
	 * Returns the current Rtti type descriptor of the Variant.
	 *
	 * @return the Rtti type descriptor. Either one of RttiTypes::Int,
	 * RttiTypes::Bool, RttiTypes::Double, RttiTypes::String, RttiTypes::Array
	 * or RttiTypes::Function or -- in case an object is stored inside the
	 * variant -- the Rtti of that object.
	 */
	const Rtti *getRtti() const;

	/**
	 * Returns the name of the given variant type as C-style string.
	 */
	static const char *getTypeName(VariantType type);

	/**
	 * Returns the name of the type of this variant instance.
	 */
	const char *getTypeName() const { return Variant::getTypeName(getType()); }

	/*
	 * Source location
	 */

	/**
	 * Returns true if the stored source id is not invalid.
	 *
	 * @retun true if the
	 */
	bool hasLocation() const { return meta.hasLocation(); }

	/**
	 * Unpacks ans returns the stored source location. Note that the returned
	 * location may differ from the one given in "setLocation", if the values
	 * were too large to represent.
	 *
	 * @return the stored SourceLocation.
	 */
	SourceLocation getLocation() const { return meta.getLocation(); }

	/**
	 * Packs the given source location and stores it in the metadata. Not all
	 * SourceLocation values may be representable, as they are stored with fewer
	 * bits as in the SourceLocation structure.
	 *
	 * @param location is the SourceLocation that should be stored.
	 */
	void setLocation(const SourceLocation &location)
	{
		return meta.setLocation(location);
	}

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

/**
 * The ManagedVariant class allows to store a variant as a Managed object. This
 * class is used to store Variants as the data of "Managed" objects.
 *
 * TODO: Eventually Managed objects should support storing Variants as "data"
 * directly. Yet is hard to get the dependencies right...
 */
class ManagedVariant : public Managed {
public:
	/**
	 * Variant value of the ManagedVariant class.
	 */
	Variant v;

	/**
	 * Creates a new ManagedVariant instance, the variant is initialized to
	 * null.
	 *
	 * @param mgr is the Manager the ManagedVariant belongs to.
	 */
	ManagedVariant(Manager &mgr) : Managed(mgr){};

	/**
	 * Create a new ManagedVariant instance and initializes it with the given
	 * variant value.
	 *
	 * @param mgr is the Manager the ManagedVariant belongs to.
	 * @param v is the initial value for the Variant.
	 */
	ManagedVariant(Manager &mgr, const Variant &v) : Managed(mgr), v(v){};
};

/* Static Assertions */

// Make sure VariantData has a length of 8 bytes
static_assert(sizeof(VariantMetadata) == 8,
              "VariantMetadata should have a length of 8 Bytes");

// Make sure VariantData has a length of 16 bytes
static_assert(sizeof(Variant) == 16,
              "Variant should have a length of 16 bytes");

/* RttiTypes */

namespace RttiTypes {
extern const Rtti ManagedVariant;
}
}

#endif /* _OUSIA_VARIANT_HPP_ */

