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
 * resolve objects of a certain type in the object graph and to attach
 * information that should be accessible to the script engine.
 *
 * <b>Why is this needed?</b> C++ provides the <tt>typeid</tt> operator to
 * retrieve a reference at an internal table associated with type information
 * for the given class. However, there is no native way for attaching additonal
 * information to this type information table. Additional information we need to
 * store is the inheritance graph (which cannot easily be extracted from C++)
 * and information relevant for script engines (such as a list of methods and
 * properties). One could of course store information about the type within each
 * instance of this type, however when managing thousands of objects
 * this would create a significant overhead.
 *
 * <b>How to use:</b> The Rtti class allows to attach information to a certain
 * C++ class. To do so, create a global constant of the type Rtti<T> in the
 * source file associated with the type declaration, where T is the type you
 * want to register. As the type must only be registered once, you must not
 * declare the variable as "static" in the header file (this would register it
 * whever the header is included). If you want to access the global constant
 * from other Rtti definitions (as parent), create a forward declaration
 * in the header file. If you want to access the RTTI of a certain object or
 * type, use the global typeOf() function (however, don't use it
 * within global variable initializations).
 *
 * <b>Example:</b>
 * In the header file:
 * \code{.hpp}
 * // Only needed if the type needs to be accessed
 * // from other compilation units!
 * namespace RttiTypes {
 *     extern const Rtti<MyT> MyT;
 * }
 * \endcode
 * In the source file:
 * \code{.cpp}
 * const Rtti<MyT> RttiTypes::MyT{"MyT", {&RttiTypes::MyOtherT}, [...]};
 * \endcode
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_RTTI_HPP_
#define _OUSIA_RTTI_HPP_

#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
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
	 * Set to true if once the parents and the aggregated types list have been
	 * completed (by including the parents of the original parent elements and
	 * the aggregated types of the original aggregated types).
	 */
	mutable bool initialized;

	/**
	 * Set containing references to all parent types, including their parents.
	 */
	mutable std::unordered_set<const RttiBase *> parents;

	/**
	 * Set containing references to all types this type is aggregated of,
	 * including all aggregated types of the original aggregated types.
	 */
	mutable std::unordered_set<const RttiBase *> aggregatedTypes;

	/**
	 * Adds the parent types of the parents and the aggregated types of the
	 * aggregated types to the internal sets.
	 */
	void initialize() const;

public:
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
	         std::unordered_set<const RttiBase *> parents =
	             std::unordered_set<const RttiBase *>{},
	         std::unordered_set<const RttiBase *> aggregatedTypes =
	             std::unordered_set<const RttiBase *>{})
	    : initialized(false),
	      parents(std::move(parents)),
	      aggregatedTypes(aggregatedTypes),
	      name(std::move(name))
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

	/**
	 * Returns true if an instance of this type may have references to the other
	 * given type. This mechanism is used to prune impossible paths when
	 * resolving objects of a certain type by name in an object graph.
	 *
	 * @param other is the other type for which should be checked whether this
	 * type is directly or indirectly aggregated of it.
	 */
	bool contains(const RttiBase &other) const;
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
	Rtti(std::string name, const std::unordered_set<const RttiBase *> &parents =
	                           std::unordered_set<const RttiBase *>{},
	     std::unordered_set<const RttiBase *> aggregatedTypes =
	         std::unordered_set<const RttiBase *>{})
	    : RttiBase(name, typeid(T), std::move(parents),
	               std::move(aggregatedTypes))
	{
	}
};

/**
 * Function that can be used to retrieve the RTTI information of a Managed
 * object. Do not use this function in the initialization of global Rtti
 * variables, use pointers at the other global variable instead (as the
 * initialization order is not well defined).
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
 * object. Do not use this function in the initialization of global Rtti
 * variables, use pointers at the other global variable instead (as the
 * initialization order is not well defined).
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

namespace RttiTypes {
/**
 * Type of no particular type.
 */
extern const RttiBase None;
}
}

#endif /* _OUSIA_RTTI_HPP_ */

