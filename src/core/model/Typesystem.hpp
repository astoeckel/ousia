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
	 * @param system is a reference to the parent TypeSystem instance.
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
};

/**
 * The StringType class represents the primitive string type. There should
 * exactly be a single instance of this class available in a preloaded type
 * system.
 */
class StringType : public Type {
protected:
	/**
	 * If possible, converts the given variant to a string. Only works, if the
	 * variant contains primitive objects (integers, strings, booleans, etc.).
	 */
	bool doBuild(Variant &var, Logger &logger) const override;

public:
	/**
	 * Constructor of the StringType class. Only one instance of StringType
	 * should exist per project.
	 *
	 * @param mgr is the Manager instance to be used for the Node.
	 * @param name is the name of the type.
	 * @param system is a reference to the parent TypeSystem instance.
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

class IntType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doBuild(Variant &var, Logger &logger) const override
	{
		if (!var.isInt()) {
			throw LoggableException{"Expected an integer value."};
		}
		return true;
	}

public:
	/**
	 * TODO: DOC
	 */
	IntType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "int", system, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() const override { return Variant{0}; }
};

class DoubleType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doBuild(Variant &var, Logger &logger) const override
	{
		if (!var.isInt() && !var.isDouble()) {
			throw LoggableException{"Expected a double value."};
		}
		var = Variant{var.toDouble()};
		return true;
	}

public:
	/**
	 * TODO: DOC
	 */
	DoubleType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "double", system, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() const override { return Variant{0.}; }
};

class UnknownType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doBuild(Variant &var, Logger &logger) const override { return true; }

public:
	/**
	 * TODO: DOC
	 */
	UnknownType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "unknown", system, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() const override { return Variant{nullptr}; }
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
	struct AttributeDescriptor {
		const std::string name;
		const Variant defaultValue;
		const bool optional;
		const Owned<Type> type;

		AttributeDescriptor(std::string name, Variant defaultValue,
		                    bool optional, Owned<Type> type)
		    : name(name),
		      defaultValue(defaultValue),
		      optional(optional),
		      type(type)
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
				if (!attrs[a].type->build(arr[a], logger)) {
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
		std::vector<Variant> vec;
		for (auto &a : attrs) {
			auto it = map.find(a.name);
			// we use the default if nothing is set.
			if (it == map.end() || !a.type->build(it->second, logger)) {
				logger.note(std::string("Using default value for ") + a.name);
				vec.push_back(a.defaultValue);
			} else {
				vec.push_back(it->second);
			}
		}
		var = Variant(vec);
		return true;
	}

public:
	std::vector<AttributeDescriptor> attrs;

	StructType(Manager &mgr, std::string name, Handle<Typesystem> system,
	           std::vector<AttributeDescriptor> attrs)
	    : Type(mgr, std::move(name), system, false), attrs(std::move(attrs))
	{
	}
	// TODO
	//	static StructType createValidated(
	//	    Manager &mgr, std::string name, Handle<Typesystem> system,
	//	    Handle<StructType> parent,
	//	    const std::vector<AttributeDescriptor> &attrs, Logger &logger);

	Variant create() const override { return Variant{Variant::arrayType{}}; }
};

class ArrayType : public Type {
private:
	Owned<Type> innerType;

protected:
	/**
	 * TODO: DOC
	 */
	bool doBuild(Variant &var, Logger &logger) const override
	{
		if (!var.isArray()) {
			throw LoggableException("Expected array!");
		}
		bool res = true;
		for (auto &v : var.asArray()) {
			if (!innerType->build(v, logger)) {
				res = false;
			}
		}

		return res;
	}

public:
	/**
	 * TODO: DOC
	 */
	ArrayType(Manager &mgr, std::string name, Handle<Typesystem> system,
	          Handle<Type> innerType)
	    : Type(mgr, std::move(name), system, false),
	      innerType(acquire(innerType))
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() const override { return Variant{Variant::arrayType{}}; }

	Rooted<Type> getType() { return innerType; }
};

class Typesystem : public Node {
private:
	NodeVector<Type> types;

public:
	Typesystem(Manager &mgr, std::string name, Handle<Node> parent = nullptr)
	    : Node(mgr, name, parent), types(this)
	{
	}

	/**
	 * TODO: DOC
	 */
	void addType(Handle<Type> type) { types.push_back(type); }

	const NodeVector<Type> &getTypes() const { return types; }
};
}
}

#endif /* _OUSIA_MODEL_TYPESYSTEM_HPP_ */

