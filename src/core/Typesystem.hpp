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

class TypeInstance : public Managed {
public:
	const Owned<Type> type;

	TypeInstance(Manager &mgr, Handle<Type> type)
	    : Managed(mgr), type(acquire(type))
	{
	}
};

class Type : public Node {
public:
	using Node::Node;

	virtual bool isFinal() const { return true; }

	virtual bool isPrimitive() const { return true; }

	virtual Rooted<TypeInstance> create() = 0;

	virtual Rooted<TypeInstance> parse(const std::string &str) = 0;
};

class Typesystem : public Node {
private:
	const std::vector<Owned<Type>> types;
	const std::vector<Owned<TypeInstance>> constants;

protected:
	void doResolve(std::vector<Rooted<Node>> &res,
	               const std::vector<std::string> &path, Filter filter,
	               void *filterData, unsigned idx,
	               VisitorSet &visited) override;

public:
	using Node::Node;

	const &std::vector<Owned<Type>> getTypes() { return types; }

	const &std::vector<Owned<TypeInstance>> getConstants() { return constants; }

	void addType(Handle<Type> type) {
		types.push_back(acquire(type));
	}

	void addConstant(Handle<TypeInstance> ) {
		
	}
};
}

#endif /* _OUSIA_TYPESYSTEM_HPP_ */

