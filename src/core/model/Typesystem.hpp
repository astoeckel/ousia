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
 * @file Typesystem.hpp
 *
 * Contains the Entities described in a Typesystem. A Typesystem is a list
 * of type descriptors, where a type is either primitive or a user defined
 * type.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_TYPESYSTEM_HPP_
#define _OUSIA_MODEL_TYPESYSTEM_HPP_

#include <map>
#include <vector>

#include <core/common/Exceptions.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Variant.hpp>

#include "Node.hpp"

namespace ousia {

// Forward declarations
class Rtti;
class Typesystem;
class SystemTypesystem;

/**
 * The abstract Type class represents a type descriptor. Each Type node is part
 * of a Typesystem instance. Concrete instances of the Type class are immutable
 * (they are guaranteed to represent exactly one type). Note that Type classes
 * only contain the type description, instances of the Type class do not hold
 * any data. Data is held by instances of the Variant class. How exactly the
 * data is represented within the Variant instances is defined by the type
 * definitions.
 */
class Type : public Node {
protected:
	/**
	 * Protected constructor to be called by the classes derived from the Type
	 * class.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the type.
	 * @param system is a reference to the parent Typesystem instance.
	 * @param primitive is set to true for primitive types, such as ints,
	 * doubles, strings and enums.
	 */
	Type(Manager &mgr, std::string name, Handle<Typesystem> system,
	     bool primitive)
	    : Node(mgr, std::move(name), system), primitive(primitive)
	{
	}

	/**
	 * Validates and completes the given variant. This pure virtual doBuild
	 * method must be overridden by derived classes. This function may throw
	 * an LoggableException in case the given data cannot be converted to
	 * the internal representation given by the type descriptor.
	 *
	 * @param data is a variant containing the data that should be checked and
	 * -- if possible and necessary -- converted to a variant adhering to the
	 * internal representation used by the Type class.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	virtual bool doBuild(Variant &data, Logger &logger) const = 0;

public:
	/**
	 * Set to true, if this type descriptor is a primitive type.
	 */
	const bool primitive;

	/**
	 * Pure virtual function which must construct a valid, default instance of
	 * the type that is being described by the typesystem. This function is
	 * usually called as a last resort if an instance of a certain type is
	 * requested but cannot be generated from the given user data.
	 */
	virtual Variant create() const = 0;

	/**
	 * Validates and completes the given variant which was read from a
	 * user-supplied source.
	 *
	 * @param data is a variant containing the data that should be checked and
	 * -- if possible and necessary -- converted to a variant adhering to the
	 * internal representation used by the Type class.
	 * @param logger is the Logger instance into which errors should be
	 * written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool build(Variant &data, Logger &logger) const;

	/**
	 * Returns true if and only if the given Variant adheres to this Type. In
	 * essence this just calls the build method on a copy of the input Variant.
	 *
	 * @param data is a Variant containing data that shall be validated.
	 * @param logger is a logger instance to which errors will be written.
	 *
	 * @return true if and only if the given Variant adheres to this Type.
	 */
	bool isValid(Variant data, Logger &logger) const{
		return build(data, logger);
	}

	/**
	 * Returns the underlying Typesystem instance.
	 *
	 * @return a Rooted reference pointing at the underlying typesystem
	 * instance.
	 */
	Rooted<Typesystem> getTypesystem()
	{
		return this->getParent().cast<Typesystem>();
	}
};

/**
 * The StringType class represents the primitive string type. There should
 * be exactly one instance of this class available in a preloaded type system.
 */
class StringType : public Type {
protected:
	/**
	 * If possible, converts the given variant to a string. Only works, if the
	 * variant contains primitive objects (integers, strings, booleans, etc.).
	 *
	 * @param data is a variant containing the data that should be checked and
	 * converted to a string.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &data, Logger &logger) const override;

public:
	/**
	 * Constructor of the StringType class. Only one instance of StringType
	 * should exist per project graph.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the type.
	 * @param system is a reference to the parent Typesystem instance.
	 */
	StringType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "string", system, true)
	{
	}

	/**
	 * Creates a variant containing an empty string.
	 *
	 * @return a variant containing an empty string.
	 */
	Variant create() const override { return Variant{""}; }
};

/**
 * The IntType class represents the primitive integer type. There should be
 * exactly one instance of this class available in a preloaded type system.
 */
class IntType : public Type {
protected:
	/**
	 * Expects the given variant to be an integer. Does not perform any type
	 * conversion.
	 *
	 * @param data is a variant containing the data that should be checked.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &data, Logger &logger) const override;

public:
	/**
	 * Constructor of the IntType class. Only one instance of IntType should
	 * exist per project graph.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the type.
	 * @param system is a reference to the parent Typesystem instance.
	 */
	IntType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "int", system, true)
	{
	}

	/**
	 * Returns a variant containing the integer value zero.
	 *
	 * @return the integer value zero.
	 */
	Variant create() const override { return Variant{0}; }
};

/**
 * The DoubleType class represents the primitive double type. There should be
 * exactly one instance of this class available in a preloaded type system.
 */
class DoubleType : public Type {
protected:
	/**
	 * Expects the given variant to be a double or an integer. Converts integers
	 * to doubles.
	 *
	 * @param data is a variant containing the data that should be checked.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &data, Logger &logger) const override;

public:
	/**
	 * Constructor of the DoubleType class. Only one instance of DoubleType
	 * should exist per project graph.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the type.
	 * @param system is a reference to the parent Typesystem instance.
	 */
	DoubleType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "double", system, true)
	{
	}

	/**
	 * Returns a variant containing the double value zero.
	 *
	 * @return the double value zero.
	 */
	Variant create() const override { return Variant{0.0}; }
};

/**
 * The BoolType class represents the primitive boolean type. There should be
 * exactly one instance of this class available in a preloaded type system.
 */
class BoolType : public Type {
protected:
	/**
	 * Expects the given variant to be a boolean. Performs no implicit type
	 * conversion.
	 *
	 * @param data is a variant containing the data that should be checked.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &data, Logger &logger) const override;

public:
	/**
	 * Constructor of the BoolType class. Only one instance of BoolType should
	 * exist per project graph.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the type.
	 * @param system is a reference to the parent Typesystem instance.
	 */
	BoolType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "bool", system, true)
	{
	}

	/**
	 * Creates a variant with the boolean value false.
	 *
	 * @return a Variant with the boolean value false.
	 */
	Variant create() const override { return Variant{false}; }
};

/**
 * The EnumType class represents a user defined enumeration type.
 */
class EnumType : public Type {
public:
	using Ordinal = Variant::intType;

private:
	/**
	 * Map containing the enumeration type values and the associated integer
	 * representation.
	 */
	const std::map<std::string, Ordinal> values;

	/**
	 * Private constructor of the EnumType class used to create a new EnumType
	 * instance from a previously created name to value map. The parameters are
	 * not checked for validity.
	 *
	 * @param mgr is the underlying Manager instance.
	 * @param name is the name of the EnumType instance. Should be a valid
	 * identifier.
	 * @param values is a vector containing the enumeration type constants.
	 */
	EnumType(Manager &mgr, std::string name, Handle<Typesystem> system,
	         std::map<std::string, Ordinal> values)
	    : Type(mgr, std::move(name), system, false), values(std::move(values))
	{
	}

protected:
	/**
	 * Converts the given variant to the corresponding enum type representation.
	 * The variant may either be a magic string containing the name of an
	 * enumeration type or an integer.
	 *
	 * @param data is a variant containing the data that should be checked.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &data, Logger &logger) const override;

public:
	/**
	 * Creates a new enum instance and validates the incomming value vector.
	 *
	 * @param mgr is the underlying Manager instance.
	 * @param name is the name of the EnumType instance. Should be a valid
	 * identifier.
	 * @param system is a reference to the parent Typesystem instance.
	 * @param values is a vector containing the enumeration type constants.
	 * The constants are checked for validity (must be a valid identifier) and
	 * uniqueness (each value must exist exactly once).
	 * @param logger is the Logger instance into which errors should be written.
	 */
	static Rooted<EnumType> createValidated(
	    Manager &mgr, std::string name, Handle<Typesystem> system,
	    const std::vector<std::string> &values, Logger &logger);

	/**
	 * Creates a Variant containing a valid representation of a variable of this
	 * EnumType instance. The variant will point at the first enumeration
	 * constant.
	 *
	 * @return a variant pointing at the first enumeration constant.
	 */
	Variant create() const override { return Variant{0}; }

	/**
	 * Returns the name of the given ordinal number. Throws a LoggableException
	 * if the ordinal number is out of range.
	 */
	std::string nameOf(Ordinal i) const;

	/**
	 * Returns the ordinal numer associated with the given enumeration constant
	 * name. Throws a LoggableException if the string does not exist.
	 */
	Ordinal valueOf(const std::string &name) const;
};

/**
 * The Attribute class describes a single attribute of a StructuredType entry.
 */
class Attribute : public Node {
private:
	/**
	 * Reference to the actual type of the attribute.
	 */
	Owned<Type> type;

	/**
	 * Initial default value passed to the constructor of the Attribute class
	 * that has not been passed through the "build" method of the type.
	 */
	Variant rawDefaultValue;

	/**
	 * Default value of the attribute.
	 */
	Variant defaultValue;

	/**
	 * Flag indicating whether this attribute is actually optional or not.
	 */
	bool optional;

	/**
	 * Reinitializes the default value from the raw default value with the
	 * current type.
	 *
	 * @param logger is the logger instance to which errors while building the
	 * default value should be passed.
	 */
	void initialize(Logger &logger);

protected:
	/**
	 * Returns true if the name of the Attribute is a valid identifier.
	 *
	 * @param logger is the logger instance to which validation errors are
	 * logged.
	 */
	bool doValidate(Logger &logger) const override;

public:
	/**
	 * Constructor of the Attribute class.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param type holds a reference to the type descriptor holding the type
	 * of the attribute.
	 * @param name is the name of the Attribute. Should be a valid identifier.
	 * @param defaultValue is the default value of the attribute and must have
	 * been passed through the build of the specified type.
	 * @param optional should be set to true if the if the default value should
	 * be used.
	 */
	Attribute(Manager &mgr, std::string name, Handle<Type> type,
	          Variant defaultValue, bool optional = true);

	/**
	 * Constructor of the Attribute class with no default value.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param type holds a reference to the type descriptor holding the type
	 * of the attribute.
	 * @param name is the name of the Attribute. Should be a valid identifier.
	 */
	Attribute(Manager &mgr, std::string name, Handle<Type> type);

	/**
	 * Constructor of the Attribute class with default value but unknown type.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the Attribute. Should be a valid identifier.
	 * @param defaultValue is the default value of the attribute and must have
	 * been passed through the build of the specified type.
	 * @param optional should be set to true if the if the default value should
	 * be used.
	 */
	Attribute(Manager &mgr, std::string name, Variant defaultValue,
	          bool optional);

	/**
	 * Sets a new default value. This makes the Attribute optional. The given
	 * default value is passed through the "build" function of the current
	 * type.
	 *
	 * @param defaultValue is the new default value.
	 * @param logger is the logger instance to which errors that occur during
	 * reinterpretion of the default value.
	 */
	void setDefaultValue(const Variant &defaultValue, Logger &logger);

	/**
	 * Returns the default value of the attribute.
	 *
	 * @return the default value of the attribute. If no default value has been
	 * given a null variant is returned (the opposite does not hold).
	 */
	Variant getDefaultValue() const { return defaultValue; }

	/**
	 * Removes any default value from the attribute, making this attribute
	 * non-optional.
	 */
	void removeDefaultValue();

	/**
	 * Returns true if the attribute is optional (a default value has been
	 * supplied by the user).
	 *
	 * @return true if the attribute is optional, false otherwise.
	 */
	bool isOptional() const { return optional; }

	/**
	 * Sets the type of the attribute to the specified value. This will
	 * reinterpret the default value that has been passed to the attribute (if
	 * available).
	 *
	 * @param type is the new type that should be used for the attribute.
	 * @param logger is the logger instance to which errors that occur during
	 * reinterpretion of the default value.
	 */
	void setType(Handle<Type> type, Logger &logger);

	/**
	 * Returns a reference to the type descriptor holding the type of the
	 * attribute.
	 *
	 * @return the underlying type of the Rooted object.
	 */
	Rooted<Type> getType() const { return type; }
};

/**
 * The StructType class represents a user defined structure.
 */
class StructType : public Type {
private:
	/**
	 * Reference to the parent structure type (or nullptr if the struct type is
	 * not derived from any other struct type).
	 */
	Owned<StructType> parentStructure;

	/**
	 * Contains the index at which the attributes declared by this StructType
	 * start.
	 */
	size_t attributeStart;

	/**
	 * Vector containing references to all attribute descriptors, including the
	 * attributes of the parent structure.
	 */
	NodeVector<Attribute> attributes;

	/**
	 * Map storing the attribute names.
	 */
	std::map<std::string, size_t> attributeNames;

	/**
	 * Resolves an attribute key string of the form "#idx" to the corresponding
	 * attribute index.
	 *
	 * @param key is the key to be parsed.
	 * @param val is the variable in which the result should be stored.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool resolveIndexKey(const std::string &key, size_t &idx) const;

	/**
	 * Resolves an attribute key strin of the form "key" to the corresponding
	 * attribute index.
	 *
	 * @param key is the key to be parsed.
	 * @param val is the variable in which the result should be stored.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool resolveIdentifierKey(const std::string &key, size_t &idx) const;

	/**
	 * Resolves the given attribute key to the corresponding array index.
	 *
	 * @param key is the key to be parsed.
	 * @param val is the variable in which the result should be stored.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool resolveKey(const std::string &key, size_t &idx) const;

	/**
	 * Inserts default values into unset attribute slots. Loggs errors if a
	 * attribute that was not explicitly set has no default value associated to
	 * it.
	 *
	 * @param data is a variant with array type that should be updated.
	 * @param set indicating which array slots that have been set explicitly.
	 * @param logger used to which error messages and warnings are logged.
	 * @return true if the operation is successful, false otherwise.
	 */
	bool insertDefaults(Variant &data, const std::vector<bool> &set,
	                    Logger &logger) const;

	/**
	 * Checks an array for validity and if possible updates its content to match
	 * the types of the structure type.
	 *
	 * @param data is the variant to be checked.
	 * @param logger used to which error messages and warnings are logged.
	 * @param trim if true, longer arrays are accepted and trimmed to the number
	 * of attributes (as needed when casting from a derived type).
	 * @return true if the operation is successful, false otherwise.
	 */
	bool buildFromArray(Variant &data, Logger &logger, bool trim) const;

	/**
	 * Checks a map and its entries for validity and if possible updates its
	 * content to match the types of the structure type.
	 *
	 * @param data is the variant to be checked.
	 * @param logger used to which error messages and warnings are logged.
	 * @param trim if true, unspecified indices are ignored. This may be needed
	 * when casting from a derived type.
	 * @return true if the operation is successful, false otherwise.
	 */
	bool buildFromMap(Variant &data, Logger &logger, bool trim) const;

	/**
	 * Checks a map or an array for validity and if possible updates its content
	 * to match the types of the structure type.
	 *
	 * @param data is the variant to be checked.
	 * @param logger used to which error messages and warnings are logged.
	 * @param trim if true, unspecified indices are ignored. This may be needed
	 * when casting from a derived type.
	 * @return true if the operation is successful, false otherwise.
	 */
	bool buildFromArrayOrMap(Variant &data, Logger &logger, bool trim) const;

	/**
	 * Rebuilds the internal index and attribute list depending on the parent
	 * structure.
	 */
	void initialize(Logger &logger);

	/**
	 * Function used internally to add and index attributes while logging
	 * exceptions.
	 *
	 * @param attribute is the attribute that should be added.
	 * @param logger is the logger instance to which
	 */
	void addAttribute(Handle<Attribute> attribute, Logger &logger,
	                  bool fromInitialize);

protected:
	/**
	 * Converts the given variant to the representation of the structure type.
	 * The variant may either be an array containing the values of the
	 * attributes in the correct order or a map containing the names of the
	 * attributes or their position in the for of a hash symbol "#" folowed by
	 * the index of the attribute. The resulting variant is an array containg
	 * the value of each attribute, extended by the default values
	 * in the correct order.
	 *
	 * @param data is a variant containing the data that should be checked.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &data, Logger &logger) const override;

	/**
	 * Checks the struct descriptor for being valid.
	 *
	 * @param logger is a reference to the logger to which error messages should
	 * be logged.
	 */
	bool doValidate(Logger &logger) const override;

public:
	/**
	 * Constructor of the StructType class, creates a new instance
	 * without performing any validity checks.
	 *
	 * @param mgr is the underlying Manager instance.
	 * @param name is the name of the EnumType instance. Should be a valid
	 * identifier.
	 * @param system is a reference to the parent Typesystem instance.
	 */
	StructType(Manager &mgr, std::string name, Handle<Typesystem> system)
	    : Type(mgr, std::move(name), system, false),
	      attributeStart(0),
	      attributes(this)
	{
	}

	/**
	 * Creates a new instance of the StructType class and checks the given
	 * parameters for validity.
	 *
	 * @param mgr is the underlying Manager instance.
	 * @param name is the name of the StructType instance. Should be a valid
	 * identifier.
	 * @param system is a reference to the parent Typesystem instance.
	 * @param parentStructure is a reference to the StructType this type is
	 * derived from, may be nullptr.
	 * @param attributes is a vector containing the struct type attributes.
	 * The attributes are checked for validity (their names must be a valid
	 * identifiers) and uniqueness (each value must exist exactly once).
	 * @param logger is the Logger instance into which errors should be written.
	 */
	static Rooted<StructType> createValidated(
	    Manager &mgr, std::string name, Handle<Typesystem> system,
	    Handle<StructType> parentStructure,
	    const NodeVector<Attribute> &attributes, Logger &logger);

	/**
	 * Returns a handle pointing at the parent type.
	 *
	 * @return a rooted handle pointing at the parent type or nullptr, if this
	 * struct type has no parent.
	 */
	Rooted<StructType> getParentStructure() const;

	/**
	 * Sets the parent structure (the structure this once derives from).
	 *
	 * @param parentStructure is the new parent structure.
	 */
	void setParentStructure(Handle<StructType> parentStructure, Logger &logger);

	/**
	 * Creates a new attribute with unknown type and adds it to the attribute
	 * list.
	 *
	 * @param name is the name of the attribute.
	 * @param defaultValue is the default value of the attribute.
	 * @param optional specifies whether the attribute is optional or not.
	 * @param logger is the logger instance to which errors while creating or
	 * adding the attribute should be logged.
	 * @return a new instance of the Attribute class.
	 */
	Rooted<Attribute> createAttribute(const std::string &name,
	                                  Variant defaultValue, bool optional,
	                                  Logger &logger);

	/**
	 * Adds an attribute. Throws an exception if the name of the attribute
	 * is not unique.
	 *
	 * @param attribute is the attribute descriptor that should be added to the
	 * internal attribute list.
	 */
	void addAttribute(Handle<Attribute> attribute, Logger &logger);

	/**
	 * Adds a complete list of attributes to the typesystem.
	 *
	 * @param attributes is the list with typesystems that should be added.
	 */
	void addAttributes(const NodeVector<Attribute> &attributes, Logger &logger);

	/**
	 * Creates a Variant containing a valid representation of a data instance of
	 * this StructType.
	 *
	 * @return a valid, empty data instance of this type.
	 */
	Variant create() const override;

	/**
	 * Function to return true if the other type either equals this type or this
	 * type is derived from the other type.
	 *
	 * @param other is the other struct type that should be checked.
	 * @return true if the other type instance points at the same instance as
	 * this type or this type is derived from the other type.
	 */
	bool derivedFrom(Handle<StructType> other) const;

	/**
	 * Casts the given type instance of a derived type to a type instance valid
	 * for this type. This operation is only valid if this type instance is a
	 * parent of the type instance that generated the data.
	 *
	 * @param data is the data that should be cast to this type. The data must
	 * have been built by a derived type of this type instance.
	 * @param logger is the Logger instance to which errors should be logged.
	 * @return true if the operation is successful, false otherwise.
	 */
	bool cast(Variant &data, Logger &logger) const;

	/**
	 * Returns a reference at the list containing all attributes, including the
	 * attributes of the parent structure.
	 *
	 * @return a const reference pointing at the attribute list.
	 */
	const NodeVector<Attribute> &getAttributes() const { return attributes; };

	/**
	 * Returns the index of the given attribute in a data array representing
	 * the StructType.
	 *
	 * @param name is the name of the attribute for which the index is
	 * requested.
	 * @return the index or -1 if the attribute does not exist.
	 */
	ssize_t indexOf(const std::string &name) const;

	/**
	 * Returns true if an attribute with the given name exists.
	 *
	 * @param name is the name of the attribute for which the index is
	 * requested.
	 * @return true if the requested attribute name exists, false otherwise.
	 */
	bool hasAttribute(const std::string &name) const;
};

/**
 * The ArrayType class represents an array with elements of a fixed inner type.
 * ArrayTypes are anonymous (they have an empty name) and always have the
 * Typesystem instance of the inner type as parent. ArrayType instances are
 * created implicitly if the user requests an array of a certain type.
 */
class ArrayType : public Type {
private:
	/**
	 * Contains the inner type of the array.
	 */
	Owned<Type> innerType;

protected:
	/**
	 * Makes sure the given variant is an array and its elements match the inner
	 * type of the Arraqy.
	 *
	 * @param data is a variant containing the array data that should be checked
	 * and passed to the inner type validation function.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &data, Logger &logger) const override;

public:
	/**
	 * Constructor of the ArrayType class.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param innerType is the type of the elements stored in the array.
	 */
	ArrayType(Manager &mgr, Handle<Type> innerType)
	    : Type(mgr, "", innerType->getTypesystem(), false),
	      innerType(acquire(innerType))
	{
	}

	/**
	 * Create a new, empty variant containing array data.
	 *
	 * @return an empty variant array.
	 */
	Variant create() const override { return Variant{Variant::arrayType{}}; }

	/**
	 * Returns a Rooted reference pointing at the inner type of the array (e.g.
	 * the type of the elements stored in the array).
	 *
	 * @return Rooted reference pointing at the innerType.
	 */
	Rooted<Type> getInnerType() { return innerType; }
};

/**
 * The UnknownType class represents a type whose type could not be resolved.
 * This class may also be used while constructing the Typesystem tree, if a
 * type has not yet been fully created. UnknownType instances are not part of a
 * typesystem, but merely placeholders which may once be replaced by an actual
 * reference to an actual Type node.
 *
 * TODO: Implement generic object for unresolved Nodes, not only types. For this
 * implement Manager.replace, which replaces all references to a certain object
 * with a new one.
 */
class UnknownType : public Type {
protected:
	/**
	 * As the UnknownType carries no type information, it does not modify the
	 * given variant and always succeeds (returns true).
	 *
	 * @return always true.
	 */
	bool doBuild(Variant &, Logger &) const override { return true; }

public:
	/**
	 * Creates a new UnknownType instance, which is a place holder for a not
	 * (yet) resolved type.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 */
	UnknownType(Manager &mgr) : Type(mgr, "unknown", nullptr, false) {}

	/**
	 * Returns a nullptr variant.
	 *
	 * @return a Variant instance with nullptr value.
	 */
	Variant create() const override { return Variant{nullptr}; }
};

/**
 * The Constant type represents a constant stored in a typesystem. A typical
 * application of a constant is e.g. to define predefined color values.
 */
class Constant : public Node {
private:
	/**
	 * Reference at the Type instance describing the type of the Constant.
	 */
	Owned<Type> type;

	/**
	 * Actual value of the constant.
	 */
	Variant value;

public:
	/**
	 * Constructor of the Constant node.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the constant.
	 * @param system is the parent typesystem.
	 * @param type is a reference at the actual type of the constant.
	 * @param value is the actual value of the constant. The value must have
	 * went through the "build" function of the type.
	 */
	Constant(Manager &mgr, std::string name, Handle<Typesystem> system,
	         Handle<Type> type, Variant value)
	    : Node(mgr, std::move(name), system),
	      type(acquire(type)),
	      value(std::move(value))
	{
	}

	/**
	 * Returns a reference pointing at the Type instance describing the type
	 * of this node.
	 *
	 * @return a Rooted handle pointing at the Type node of the constant.
	 */
	Rooted<Type> getType() { return type; }

	/**
	 * Returns a reference pointing at the value of the constant. The value must
	 * be interpreted with the help of the type of the constant.
	 *
	 * @return a const reference to the actual value of the constant.
	 */
	const Variant &getValue() { return value; }
};

/**
 * The Typesystem class represents a collection of types and constants.
 */
class Typesystem : public Node {
private:
	/**
	 * List containing all types.
	 */
	NodeVector<Type> types;

	/**
	 * List containing all constants.
	 */
	NodeVector<Constant> constants;

	/**
	 * List containing references to other referenced typesystems.
	 */
	NodeVector<Typesystem> typesystems;

protected:
	void doResolve(ResolutionState &state) override;

	bool doValidate(Logger &logger) const override;

public:
	/**
	 * Constructor of the Typesystem class.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the typesystem.
	 */
	Typesystem(Manager &mgr, std::string name)
	    : Node(mgr, std::move(name)),
	      types(this),
	      constants(this),
	      typesystems(this)
	{
	}

	/**
	 * Constructor of the Typesystem class.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param sys is a reference at the system typesystem.
	 * @param name is the name of the typesystem.
	 */
	Typesystem(Manager &mgr, Handle<SystemTypesystem> sys, std::string name)
	    : Typesystem(mgr, std::move(name))
	{
		referenceTypesystem(sys);
	}

	/**
	 * Creates a new StructType instance with the given name. Adds the new
	 * StructType as member to the typesystem.
	 *
	 * @param name is the name of the structure that should be created.
	 * @return the new StructType instance.
	 */
	Rooted<StructType> createStructType(const std::string &name);

	/**
	 * Adds a reference to the given typesystem class.
	 *
	 * @param typesystem is the typesystem that should be added to the
	 * referenced typesystems list.
	 */
	void referenceTypesystem(Handle<Typesystem> typesystem);

	/**
	 * Adds the given type to the to the type list.
	 *
	 * @param type is the Type that should be stored in this Typesystem
	 * instance.
	 */
	void addType(Handle<Type> type) { types.push_back(type); }

	/**
	 * Adds the given types to the type list.
	 *
	 * @param ts is the list of types that should be added to the typesystem.
	 */
	void addTypes(const NodeVector<Type> &ts)
	{
		types.insert(types.end(), ts.begin(), ts.end());
	}

	/**
	 * Adds the given constant to the constant list.
	 *
	 * @param constant is the constant that should be added to the typesystem.
	 */
	void addConstant(Handle<Constant> constant)
	{
		constants.push_back(constant);
	}

	/**
	 * Adds the given constants to the constant list.
	 *
	 * @param cs is the list of constants that should be added to the
	 * typesystem.
	 */
	void addConstants(const NodeVector<Constant> &cs)
	{
		constants.insert(constants.end(), cs.begin(), cs.end());
	}

	/**
	 * Returns a reference to list containing all registered types.
	 *
	 * @return NodeVector containing all registered types.
	 */
	const NodeVector<Type> &getTypes() const { return types; }

	/**
	 * Returns a reference to a list containing all registered constantants.
	 *
	 * @return NodeVector containing all registered constants.
	 */
	const NodeVector<Constant> &getConstants() const { return constants; }
};

/**
 * The SystemTypesystem class represents the typesystem defining the primitive
 * types. There should be exactly one SystemTypesystem instance in each project.
 */
class SystemTypesystem : public Typesystem {
private:
	/**
	 * Reference to the string type.
	 */
	Handle<StringType> stringType;

	/**
	 * Reference to the string type.
	 */
	Handle<IntType> intType;

	/**
	 * Reference to the double type.
	 */
	Handle<DoubleType> doubleType;

	/**
	 * Reference to the bool type.
	 */
	Handle<BoolType> boolType;

public:
	/**
	 * Creates the SystemTypesystem containing all basic types (string, int,
	 * double, bool).
	 *
	 * @param mgr is the Manager instance which manages the new Typesystem
	 * instance.
	 */
	SystemTypesystem(Manager &mgr);

	/**
	 * Returns the primitive string type.
	 *
	 * @return a reference to the primitive StringType instance.
	 */
	Rooted<StringType> getStringType() { return stringType; }

	/**
	 * Returns the primitive integer type.
	 *
	 * @return a reference to the primitive IntType instance.
	 */
	Rooted<IntType> getIntType() { return intType; }

	/**
	 * Returns the primitive double type.
	 *
	 * @return a reference to the primitive DoubleType instance.
	 */
	Rooted<DoubleType> getDoubleType() { return doubleType; }

	/**
	 * Returns the primitive boolean type.
	 *
	 * @return a reference to the primitive BoolType instance.
	 */
	Rooted<BoolType> getBoolType() { return boolType; }
};

/* RTTI type registrations */

namespace RttiTypes {
/**
 * Type information for the Type class.
 */
extern const Rtti Type;

/**
 * Type information for the StringType class.
 */
extern const Rtti StringType;

/**
 * Type information for the IntType class.
 */
extern const Rtti IntType;

/**
 * Type information for the DoubleType class.
 */
extern const Rtti DoubleType;

/**
 * Type information for the BoolType class.
 */
extern const Rtti BoolType;

/**
 * Type information for the EnumType class.
 */
extern const Rtti EnumType;

/**
 * Type information for the StructType class.
 */
extern const Rtti StructType;

/**
 * Type information for the ArrayType class.
 */
extern const Rtti ArrayType;

/**
 * Type information for the UnknownType class.
 */
extern const Rtti UnknownType;

/**
 * Type information for the Constant class.
 */
extern const Rtti Constant;

/**
 * Type information for the Attribute class.
 */
extern const Rtti Attribute;

/**
 * Type information for the Typesystem class.
 */
extern const Rtti Typesystem;

/**
 * Type information for the SystemTypesystem class.
 */
extern const Rtti SystemTypesystem;
}
}

#endif /* _OUSIA_MODEL_TYPESYSTEM_HPP_ */

