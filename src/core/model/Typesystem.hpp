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

#include <core/Node.hpp>
#include <core/common/Exceptions.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Variant.hpp>

namespace ousia {
namespace model {

class Typesystem;

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
	 * @param var is a variant containing the data that should be checked and
	 * -- if possible and necessary -- converted to a variant adhering to the
	 * internal representation used by the Type class.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	virtual bool doBuild(Variant &var, Logger &logger) const = 0;

public:
	/**
	 * Set to true, if this type descriptor is a primitive type.
	 */
	const bool primitive;

	/**
	 * Pure virtual function which must construct a valid, default instance of
	 * the type that is being described by the typesystem.
	 */
	virtual Variant create() const = 0;

	/**
	 * Validates and completes the given variant which was read from a
	 * user-supplied source.
	 *
	 * @param var is a variant containing the data that should be checked and
	 * -- if possible and necessary -- converted to a variant adhering to the
	 * internal representation used by the Type class.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool build(Variant &var, Logger &logger) const;

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
	 * @param var is a variant containing the data that should be checked and
	 * converted to a string.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &var, Logger &logger) const override;

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
	 * @param var is a variant containing the data that should be checked.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &var, Logger &logger) const override;

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
 * The DoubleType class represents the primitive double type. There should
 * exactly be a single instance of this class available in a preloaded type 
 * system.
 */
class DoubleType : public Type {
protected:
	/**
	 * Expects the given variant to be a double or an integer. Converts integers
	 * to doubles.
	 *
	 * @param var is a variant containing the data that should be checked.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &var, Logger &logger) const override;

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

class BoolType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doBuild(Variant &var, Logger &logger) const override
	{
		if (!var.isBool()) {
			throw LoggableException("Expected boolean value!");
		}
		return true;
	}

public:
	/**
	 * TODO: DOC
	 */
	BoolType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "bool", system, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() const override { return Variant{false}; }
};

class EnumType : public Type {
private:
	std::map<std::string, size_t> values;

protected:
	/**
	 * TODO: DOC
	 */
	bool doBuild(Variant &var, Logger &logger) const override
	{
		if (var.isInt()) {
			int i = var.asInt();
			if (i < 0 || i >= (int)values.size()) {
				throw LoggableException("Value is out of range.");
			}
		} else if (var.isString()) {
		}

		return true;
	}

	EnumType(Manager &mgr, std::string name, Handle<Typesystem> system,
	         std::map<std::string, size_t> values)
	    : Type(mgr, std::move(name), system, false), values(std::move(values))
	{
	}

public:
	/**
	 * TODO: DOC
	 */
	EnumType(Manager &mgr, std::string name, Handle<Typesystem> system,
	         const std::vector<std::string> &values)
	    : Type(mgr, std::move(name), system, false)
	{
		for (size_t i = 0; i < values.size(); i++) {
			this->values.insert(std::make_pair(values[i], i));
		}
	}

	/**
	 * TODO: DOC
	 */
	static EnumType createValidated(Manager &mgr, std::string name,
	                                Handle<Typesystem> system,
	                                const std::vector<std::string> &values,
	                                Logger &logger);

	/**
	 * TODO: DOC
	 */
	Variant create() const override { return Variant{0}; }
};

class StructType : public Type {
public:
	class AttributeDescriptor : public Managed {
	public:
		const std::string name;
		const Variant defaultValue;
		const bool optional;
		const Owned<Type> type;

		AttributeDescriptor(Manager &mgr, std::string name,
		                    Variant defaultValue, bool optional,
		                    Handle<Type> type)
		    : Managed(mgr),
		      name(name),
		      defaultValue(defaultValue),
		      optional(optional),
		      type(acquire(type))
		{
		}
	};

protected:
	/**
	 * TODO: DOC
	 */
	bool doBuild(Variant &var, Logger &logger) const override
	{
		// If we already have an array, we just check that.
		if (var.isArray()) {
			auto arr = var.asArray();
			for (size_t a = 0; a < attrs.size(); a++) {
				if (!attrs[a]->type->build(arr[a], logger)) {
					return false;
				}
			}
			return true;
		}
		// Otherwise we expect a map.
		if (!var.isMap()) {
			throw LoggableException("Expected map!");
		}
		auto &map = var.asMap();
		// We transform the map into an array with the correct values at the
		// correct places.
		Variant::arrayType vec;
		for (auto &a : attrs) {
			auto it = map.find(a->name);
			// we use the default if nothing is set.
			if (it == map.end() || !a->type->build(it->second, logger)) {
				logger.note(std::string("Using default value for ") + a->name);
				vec.push_back(a->defaultValue);
			} else {
				vec.push_back(it->second);
			}
		}
		var = Variant(vec);
		return true;
	}

public:
	const ManagedVector<AttributeDescriptor> attrs;

	StructType(Manager &mgr, std::string name, Handle<Typesystem> system,
	           ManagedVector<AttributeDescriptor> attrs)
	    : Type(mgr, std::move(name), system, false),
	      attrs(this, std::move(attrs))
	{
	}
	// TODO
	//	static StructType createValidated(
	//	    Manager &mgr, std::string name, Handle<Typesystem> system,
	//	    Handle<StructType> parent,
	//	    const std::vector<AttributeDescriptor> &attrs, Logger &logger);

	Variant create() const override { return Variant{Variant::arrayType{}}; }
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
	 * @param var is a variant containing the array data that should be checked
	 * and passed to the inner type validation function.
	 * @param logger is the Logger instance into which errors should be written.
	 * @return true if the conversion was successful, false otherwise.
	 */
	bool doBuild(Variant &var, Logger &logger) const override;

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
	 * @param name is the name of the unresolved type which may be used later
	 * to perform a complete resolution.
	 */
	UnknownType(Manager &mgr, std::string name)
	    : Type(mgr, std::move(name), nullptr, false)
	{
	}

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

public:
	/**
	 * Constructor of the Typesystem class.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the typesystem.
	 */
	Typesystem(Manager &mgr, std::string name)
	    : Node(mgr, name), types(this), constants(this)
	{
	}

	/**
	 * Adds the given type to the to the type list.
	 *
	 * @param type is the Type that should be stored in this Typesystem
	 * instance.
	 */
	void addType(Handle<Type> type) { types.push_back(type); }

	/**
	 * Adds the given constant to the constant list.
	 */
	void addConstant(Handle<Constant> constant)
	{
		constants.push_back(constant);
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
}

/* RTTI type registrations */

namespace RttiTypes {
/**
 * Type information for the Type class.
 */
extern const Rtti<model::Type> Type;

/**
 * Type information for the StringType class.
 */
extern const Rtti<model::StringType> StringType;

/**
 * Type information for the IntType class.
 */
extern const Rtti<model::IntType> IntType;

/**
 * Type information for the DoubleType class.
 */
extern const Rtti<model::DoubleType> DoubleType;

/**
 * Type information for the BoolType class.
 */
extern const Rtti<model::BoolType> BoolType;

/**
 * Type information for the EnumType class.
 */
extern const Rtti<model::EnumType> EnumType;

/**
 * Type information for the StructType class.
 */
extern const Rtti<model::StructType> StructType;

/**
 * Type information for the ArrayType class.
 */
extern const Rtti<model::ArrayType> ArrayType;

/**
 * Type information for the UnknownType class.
 */
extern const Rtti<model::UnknownType> UnknownType;

/**
 * Type information for the Constant class.
 */
extern const Rtti<model::Constant> Constant;

/**
 * Type information for the Typesystem class.
 */
extern const Rtti<model::Typesystem> Typesystem;
}
}

#endif /* _OUSIA_MODEL_TYPESYSTEM_HPP_ */

