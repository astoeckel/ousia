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
 * @file Domain.hpp
 *
 * TODO: Docu
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_DOMAIN_HPP_
#define _OUSIA_MODEL_DOMAIN_HPP_

#include <core/ManagedContainers.hpp>
#include <core/Node.hpp>

namespace ousia {
namespace model {

class StructuredClass;
class Descriptor;

/**
 * TODO: DOC
 */
class FieldDescriptor : public Node {
public:
	/**
	 * TODO: DOC
	 */
	enum class FieldType {
		TREE,
		SUBTREE,
		PRIMITIVE
	}

	private : ManagedVector<StructuredClass> children;
	FieldType fieldType;
	Owned<Type> primitiveType;

public:
	const bool optional;

	// TODO: What about the name of default fields?
	Type(Manager &mgr, std::string name, Handle<Descriptor> parent,
	     FieldType fieldType, Handle<Type> primitiveType, bool optional)
	    : Node(mgr, std::move(name), parent),
	      fieldType(fieldType),
	      primitiveType(acquire(primitiveType)),
	      optional(optional)
	{
	}

};
}
}

#endif /* _OUSIA_MODEL_DOMAIN_HPP_ */

