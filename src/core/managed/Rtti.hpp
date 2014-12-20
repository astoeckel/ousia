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
 * @file Rtti.hpp
 *
 * Classes used for storing runtime type information (RTTI). RTTI is used to
 * lookup objects in the object graph of a certain type and to attach
 * information that should be accessible to the script engine.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MANAGED_RTTI_HPP_
#define _OUSIA_MANAGED_RTTI_HPP_

#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ousia {

class RttiBase;

/**
 * Helper class used to globally store and access the runtime type information.
 */
class RttiStore {
private:
	/**
	 * Function used internally to access the static map storing all registered
	 * native types and their corresponding type information.
	 */
	static std::unordered_map<std::type_index, const RttiBase *> &table();

public:
	/**
	 * Registers the given pointer to the RttiBase class in the RTTI table. Does
	 * not override information for already registered types.
	 *
	 * @param native is a reference at the native type information provided
	 * by the compiler.
	 * @param rtti is a pointer pointing at the type information that should be
	 * stored for this type.
	 */
	static void store(const std::type_info &native, const RttiBase *rtti);

	/**
	 * Looks up the type information stored for the given native type
	 * information.
	 */
	static const RttiBase &lookup(const std::type_info &native);
};

/**
 * The Rtti class allows for attaching data to native types that can be accessed
 * at runtime. This type information can e.g. be retrieved using the "type"
 * method of the Managed class. This system is used for attaching human readable
 * names, parent types and script engine functionality. Use the Rtti class for
 * convenient registration of type information.
 */
class RttiBase {
private:
	/**
	 * Set containing references to the parent types.
	 */
	const std::vector<const RttiBase *> parents;

public:
	/**
	 * Rtti of no particular type.
	 */
	static const RttiBase None;

	/**
	 * Human readable name associated with the type.
	 */
	const std::string name;

	/**
	 * Default constructor. Creates a Rtti instance with name "unknown"
	 * and no parents.
	 */
	RttiBase() : name("unknown") {}

	/**
	 * Creates a new RttiBase instance and registers it in the global type
	 * table. Use the Rtti class for more convinient registration of type
	 * information.
	 *
	 * @param name is the name of the type.
	 * @param native is a reference at the native type information provided by
	 * the compiler.
	 * @param parents is a list of parent types.
	 */
	RttiBase(std::string name, const std::type_info &native,
	         std::vector<const RttiBase *> parents =
	             std::vector<const RttiBase *>{})
	    : parents(std::move(parents)), name(std::move(name))
	{
		RttiStore::store(native, this);
	}

	/**
	 * Returns true if this Rtti instance is the given type or has the
	 * given type as one of its parents.
	 *
	 * @param other is the other type for which the relation to this type
	 * should be checked.
	 */
	bool isa(const RttiBase &other) const;
};

/**
 * The Rtti class allows for attaching data to native types that can be accessed
 * at runtime. This type information can e.g. be retrieved using the "type"
 * method of the Managed class. This system is used for attaching human
 * readable names, parent types and script engine functionality.
 *
 * @tparam T is the class for which the type information should be registered.
 */
template <class T>
class Rtti : public RttiBase {
public:
	/**
	 * Creates a new RttiBase instance and registers it in the global type
	 * table.
	 *
	 * @param name is the name of the type.
	 * @param parents is a list of parent types.
	 */
	Rtti(std::string name, const std::vector<const RttiBase *> &parents =
	                           std::vector<const RttiBase *>{})
	    : RttiBase(name, typeid(T), parents)
	{
	}
};

/**
 * Function that can be used to retrieve the RTTI information of a Managed
 * object.
 *
 * @tparam T is the C++ type for which the type information should be returned.
 */
template <typename T>
inline const RttiBase &typeOf()
{
	return RttiStore::lookup(typeid(T));
}

/**
 * Function that can be used to retrieve the RTTI information of a Managed
 * object.
 *
 * @tparam T is the C++ type for which the type information should be returned.
 * @param obj is a dummy object for which the type information should be
 * returned.
 */
template <typename T>
inline const RttiBase &typeOf(const T &obj)
{
	return RttiStore::lookup(typeid(obj));
}
}

#endif /* _OUSIA_MANAGED_RTTI_HPP_ */

