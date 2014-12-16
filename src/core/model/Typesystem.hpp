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
 * TODO: Docu
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

class Type : public Node {
protected:
	Type(Manager &mgr, std::string name, Handle<Typesystem> system,
	     bool inheritable, bool primitive)
	    : Node(mgr, std::move(name), system),
	      inheritable(inheritable),
	      primitive(primitive)
	{
	}

	virtual bool doPrepare(Variant &var, Logger &log) = 0;

public:
	/**
	 * TODO: DOC
	 */
	const bool inheritable;
	/**
	 * TODO: DOC
	 */
	const bool primitive;

	/**
	 * TODO: DOC
	 */
	virtual Variant create() = 0;

	/**
	 * TODO: DOC
	 */
	bool prepare(Variant &var, Logger &log)
	{
		try {
			return doPrepare(var, log);
		}
		catch (LoggableException ex) {
			log.log(ex);
			var = create();
			return false;
		}
	}
};

class StringType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doPrepare(Variant &var, Logger &log) override
	{
		if (!var.isPrimitive()) {
			throw LoggableException{"Expected a string or primitive input."};
		}

		if (!var.isString()) {
			log.note(std::string("Implicit type conversion from ") +
			         var.getTypeName() + " to string.");
		}
		var = Variant{var.toString().c_str()};
		return true;
	}

public:
	/**
	 * TODO: DOC
	 */
	StringType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "string", system, false, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() override { return Variant{""}; }
};

class IntType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doPrepare(Variant &var, Logger &log) override
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
	    : Type(mgr, "int", system, false, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() override { return Variant{0}; }
};

class DoubleType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doPrepare(Variant &var, Logger &log) override
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
	    : Type(mgr, "double", system, false, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() override { return Variant{0.}; }
};

class UnknownType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doPrepare(Variant &var, Logger &log) override { return true; }

public:
	/**
	 * TODO: DOC
	 */
	UnknownType(Manager &mgr, Handle<Typesystem> system)
	    : Type(mgr, "unknown", system, false, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() override { return Variant{nullptr}; }
};

class BoolType : public Type {
protected:
	/**
	 * TODO: DOC
	 */
	bool doPrepare(Variant &var, Logger &log) override
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
	    : Type(mgr, "bool", system, false, true)
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() override { return Variant{false}; }
};

class EnumerationType : public Type {
private:
	std::map<std::string, size_t> values;

protected:
	/**
	 * TODO: DOC
	 */
	bool doPrepare(Variant &var, Logger &log) override
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

	EnumerationType(Manager &mgr, std::string name, Handle<Typesystem> system,
	                std::map<std::string, size_t> values)
	    : Type(mgr, std::move(name), system, false, false),
	      values(std::move(values))
	{
	}

public:
	/**
	 * TODO: DOC
	 */
	EnumerationType(Manager &mgr, std::string name, Handle<Typesystem> system,
	                const std::vector<std::string> &values)
	    : Type(mgr, std::move(name), system, false, false)
	{
		for (size_t i = 0; i < values.size(); i++) {
			this->values.insert(std::make_pair(values[i], i));
		}
	}

	/**
	 * TODO: DOC
	 */
	static EnumerationType createValidated(
	    Manager &mgr, std::string name, Handle<Typesystem> system,
	    const std::vector<std::string> &values, Logger &logger);

	/**
	 * TODO: DOC
	 */
	Variant create() override { return Variant{0}; }
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

private:
	std::vector<AttributeDescriptor> attrs;

	StructType(Manager &mgr, std::string name, Handle<Typesystem> system,
	           std::vector<AttributeDescriptor> attrs)
	    : Type(mgr, std::move(name), system, true, false),
	      attrs(std::move(attrs))
	{
	}

public:
	// TODO
	//	static StructType createValidated(
	//	    Manager &mgr, std::string name, Handle<Typesystem> system,
	//	    Handle<StructType> parent,
	//	    const std::vector<AttributeDescriptor> &attrs, Logger &logger);

	Variant create() override { return Variant{Variant::arrayType{}}; }
};

class ArrayType : public Type {
private:
	Owned<Type> innerType;

protected:
	/**
	 * TODO: DOC
	 */
	bool doPrepare(Variant &var, Logger &log) override
	{
		if (!var.isArray()) {
			throw LoggableException("Expected array!");
		}
		bool res = true;
		for (auto &v : var.asArray()) {
			if (!innerType->prepare(v, log)) {
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
	    : Type(mgr, std::move(name), system, false, false),
	      innerType(acquire(innerType))
	{
	}

	/**
	 * TODO: DOC
	 */
	Variant create() override { return Variant{Variant::arrayType{}}; }

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
};
}
}

#endif /* _OUSIA_MODEL_TYPESYSTEM_HPP_ */

