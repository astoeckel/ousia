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

#ifndef _OUSIA_TYPESYSTEM_HPP_
#define _OUSIA_TYPESYSTEM_HPP_

#include <string>
#include <vector>

#include "BufferedCharReader.hpp"
#include "Managed.hpp"
#include "Node.hpp"

namespace ousia {

class Typesystem;
class Type;
class StringType;

/**
 * The TypeInstance class represents an instance of a certain type variable:
 * Wheras Type only describes the type of an TypeInstance in an abstract manner,
 * TypeInstance represents an instance of that type.
 */
class TypeInstance : public Managed {
public:
	/**
	 * Reference to the underlying Type which describes this type instance.
	 */
	const Owned<Type> type;

	/**
	 * Constructor of the TypeInstance class.
	 *
	 * @param mgr is a reference to the Manager class which manages this object.
	 * @param type is a reference to the type this TypeInstance instance is an
	 * instance of.
	 */
	TypeInstance(Manager &mgr, Handle<Type> type)
	    : Managed(mgr), type(acquire(type))
	{
	}
};

/**
 * Type is an abstract describtion of a type class in the type system. The type
 * class can be used to instantiate instances of the corresponding type.
 */
class Type : public Node {
public:
	/**
	 * True, if the type cannot be extended.
	 */
	const bool isFinal;

	/**
	 * True, if the type represents a primitive type, such as an integer,
	 * doubles, enums and string.
	 */
	const bool isPrimitive;

	/**
	 * Constructor of the Type class.
	 *
	 * @param mgr is a reference at the underlying node manager.
	 * @param bool isFinal specifies whether this type is final.
	 * @param bool isPrimitive specifies whether this type is primitive.
	 * @param name specifies the internal name of the type.
	 * @param typesystem specifies the parent type system.
	 */
	Type(Manager &mgr, bool isFinal, bool isPrimitive, std::string name,
	     Handle<Typesystem> typesystem = nullptr)
	    : Node(mgr, std::move(name), typesystem),
	      isFinal(isFinal),
	      isPrimitive(isPrimitive)
	{
	}

	/**
	 * Virtual destructor.
	 */
	virtual ~Type(){};

	/**
	 * Creates a new instance of this type. All values of this type are
	 * initialized to default values.
	 *
	 * @return a new instance of this type.
	 */
	// virtual Rooted<TypeInstance> create() = 0;

	/**
	 * Parses the given string and produces a new instance of the given type.
	 *
	 * TODO: Add error handler
	 *
	 * @param str is the string which should be parsed.
	 */
	// virtual Rooted<TypeInstance> parse(BufferedCharReader &reader) = 0;
};

/**
 * Type which is used to represent a string.
 */
class StringType : public Type {
public:
	StringType(Manager &mgr, Handle<Typesystem> typesystem)
	    : Type(mgr, true, true, "string", typesystem){};
};

/**
 * Type which is used to represent an integer.
 */
class IntegerType : public Type {
public:
	IntegerType(Manager &mgr, Handle<Typesystem> typesystem)
	    : Type(mgr, true, true, "int", typesystem){};
};

/**
 * Type which is used to represent a double.
 */
class DoubleType : public Type {
public:
	DoubleType(Manager &mgr, Handle<Typesystem> typesystem)
	    : Type(mgr, true, true, "double", typesystem){};
};

/**
 * Type which represents a enum.
 */
class EnumType : public Type {
private:
	std::map<std::string, int> values;

public:
	EnumType(Manager &mgr, std::string name, Handle<Typesystem> typesystem,
	         const std::set<std::string> &names);

	/**
	 * Returns the integer value associated to the given name.
	 *
	 * @param name is the name of the value that should be looked up.
	 * @return the (non-negative) value associated to the given name or -1 if
	 * the value does not exist.
	 */
	int valueOf(const std::string &name);

	/**
	 * Returns the name corresponding to the given enum value or an empty string
	 * instead.
	 *
	 * @param value is the integer representation of an enum value that should
	 * be converted to the corresponding string.
	 * @return the string corresponding to the enum value or an empty string if
	 * no entry with such a value exists.
	 */
	std::string toString(int value);
};

/**
 * Type which represents an array.
 */
class ArrayType : public Type {
public:
	const Owned<Type> innerType;

	ArrayType(Manager &mgr, std::string name, Handle<Typesystem> typesystem,
	          Handle<Type> innerType)
	    : Type(mgr, false, true, name, typesystem),
	      innerType(acquire(innerType)){};
};

/**
 * Type which represents a structure of other types.
 */
class StructType : public Type {
private:
	std::map<std::string, Type> entries;

	StructType(Manager)
}

class Typesystem : public Node {
private:
	NodeVector<Type> types;
	ManagedMap<std::string, TypeInstance> constants;

protected:
	void doResolve(std::vector<Rooted<Managed>> &res,
	               const std::vector<std::string> &path, Filter filter,
	               void *filterData, unsigned idx,
	               VisitorSet &visited) override;

public:
	using Node::Node;

	Typesystem(Manager &mgr) : Node(mgr), types(this), constants(this) {}

	const NodeVector<Type> &getTypes() { return types; }

	const ManagedMap<std::string, TypeInstance> &getConstants()
	{
		return constants;
	}

	void addType(Handle<Type> type) { types.push_back(type); }

	void addConstant(const std::string &name, Handle<TypeInstance> instance)
	{
		constants.insert(std::make_pair(name, instance));
	}
};
}

#endif /* _OUSIA_TYPESYSTEM_HPP_ */

