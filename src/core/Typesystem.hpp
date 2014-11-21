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

#include "Managed.hpp"
#include "Node.hpp"

namespace ousia {

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

	virtual std::string toString() = 0;
};

class StringInstance : public TypeInstance {
public:
	std::string value;

	StringInstance(Manager &mgr, Handle<StringType> type, std::string value)
	    : TypeInstance(mgr, type), value(std::move(value))
	{
	}

	std::string toString() override;
};

/**
 * Type is an abstract describtion of a type class in the type system. The type
 * class can be used to instantiate instances of the corresponding type.
 */
class Type : public Node {
public:
	using Node::Node;

	/**
	 * Returns true, if the type cannot be extended.
	 *
	 * @return true if the type is final, false otherwise.
	 */
	virtual bool isFinal() const { return true; }

	/**
	 * Returns true, if the type is a primitive type (not a composite).
	 *
	 * @return true for types such as integers, doubles, enums and strings,
	 * false otherwise.
	 */
	virtual bool isPrimitive() const { return true; }

	/**
	 * Creates a new instance of this type. All values of this type are
	 * initialized to default values.
	 *
	 * @return a new instance of this type.
	 */
	virtual Rooted<TypeInstance> create() = 0;

	/**
	 * Parses the given string and produces a new instance of the given type.
	 *
	 * TODO: Add error handler
	 *
	 * @param str is the string which should be parsed.
	 */
	virtual Rooted<TypeInstance> parse(const std::string &str) = 0;
};

class StringType : public Type {
public:
	using Type::Type;

	Rooted<TypeInstance> create() override;

	Rooted<TypeInstance> parse(const std::string &str) override;
};

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

