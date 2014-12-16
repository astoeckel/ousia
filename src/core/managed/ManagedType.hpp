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

#ifndef _OUSIA_MANAGED_TYPE_HPP_
#define _OUSIA_MANAGED_TYPE_HPP_

#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

namespace ousia {

/**
 * The ManagedType is used to register type information that can be retrieved
 * using the "type" method of the Managed class.
 */
class ManagedType {
private:
	/**
	 * Used internally to store all registered native types and their
	 * corresponding type information.
	 */
	static std::unordered_map<std::type_index, ManagedType *>& table() {
		static std::unordered_map<std::type_index, ManagedType *> table;
		return table;
	}

	/**
	 * Name of the type -- for messages and debug output.
	 */
	const std::string name;

	/**
	 * Set containing references to the parent types.
	 */
	const std::unordered_set<ManagedType *> parents;

public:
	/**
	 * ManagedType of no particular type.
	 */
	static const ManagedType None;

	/**
	 * Returns the ManagedType for the given type_info structure.
	 */
	static const ManagedType &typeOf(const std::type_info &nativeType);

	/**
	 * Default constructor. Creates a ManagedType instance with name "unknown"
	 * and no parents.
	 */
	ManagedType() : name("unknown") {}

	/**
	 * Creates a new ManagedType instance and registers it in the global type
	 * table.
	 *
	 * @param name is the name of the type.
	 * @param nativeType is the underlying C++ class the type should be attached
	 * to.
	 */
	ManagedType(std::string name, const std::type_info &nativeType)
	    : name(std::move(name))
	{
		table().emplace(std::make_pair(std::type_index{nativeType}, this));
	}

	/**
	 * Creates a new ManagedType instance and registers it in the global type
	 * table.
	 *
	 * @param name is the name of the type.
	 * @param nativeType is the underlying C++ class the type should be attached
	 * to.
	 * @param parents is a list of parent types.
	 */
	ManagedType(std::string name, const std::type_info &nativeType,
	            std::unordered_set<ManagedType *> parents)
	    : name(std::move(name)), parents(parents)
	{
		table().emplace(std::make_pair(std::type_index{nativeType}, this));
	}

	/**
	 * Returns the name of this type.
	 */
	std::string getName() const { return name; }

	/**
	 * Returns true if this ManagedType instance is the given type or has the
	 *given
	 * type as one of its parents.
	 *
	 * @param other is the other type for which the relation to this type
	 * should be checked.
	 */
	bool isa(const ManagedType &other) const;
};
}

#endif /* _OUSIA_MANAGED_TYPE_HPP_ */

