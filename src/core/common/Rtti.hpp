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
 * C++ class. To do so, create a global constant of the type Rtti in the
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
 *     extern const Rtti MyT;
 * }
 * \endcode
 * In the source file:
 * \code{.cpp}
 * #include <core/common/RttiBuilder.hpp>
 *
 * // [...]
 *
 * namespace RttiTypes {
 *     const Rtti MyT = RttiBuilder<ousia::MyT>("MyT");
 * }
 * \endcode
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_RTTI_HPP_
#define _OUSIA_RTTI_HPP_

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ousia {

class Rtti;
class Function;
class PropertyDescriptor;

/**
 * Type describing a set of Rtti pointers.
 */
using RttiSet = std::unordered_set<const Rtti *>;

/**
 * Type describing a map containing methods and their name.
 */
using RttiMethodMap =
    std::unordered_map<std::string, std::shared_ptr<Function>>;

/**
 * Type describing a map containing properties and their name.
 */
using RttiPropertyMap =
    std::unordered_map<std::string, std::shared_ptr<PropertyDescriptor>>;

/**
 * Helper class used to globally store and access the runtime type information.
 */
class RttiStore {
private:
	/**
	 * Function used internally to access the static map storing all registered
	 * native types and their corresponding type information.
	 */
	static std::unordered_map<std::type_index, const Rtti *> &table();

public:
	/**
	 * Registers the given pointer to the Rtti class in the RTTI table. Does
	 * not override information for already registered types.
	 *
	 * @param native is a reference at the native type information provided
	 * by the compiler.
	 * @param rtti is a pointer pointing at the type information that should be
	 * stored for this type.
	 */
	static void store(const std::type_info &native, const Rtti *rtti);

	/**
	 * Looks up the type information stored for the given native type
	 * information.
	 */
	static const Rtti *lookup(const std::type_info &native);
};

/**
 * The RttiBuilderBase class is used to build new instances of the Rtti or the
 * Rtti class. It follows the "Builder" pattern and allows to create
 * the properties of the Rtti class by chaining method calls. The RttiType
 * class can be constructed from the RttiBuilderBase instance. Use the
 * RttiBuilder class for a more convenient, templated version that does not
 * require the native C++ type in the constructor and allows for more convenient
 * definition of methods and properties.
 */
class RttiBuilderBase {
public:
	/**
	 * Reference at the native type for which the Rtti information is currently
	 * being built by the RttiBuilder.
	 */
	const std::type_info &native;

	/**
	 * Contains the human readable name of the type for which the type
	 * information is being built.
	 */
	std::string currentName;

	/**
	 * Set containing references to all parent types.
	 */
	RttiSet parentTypes;

	/**
	 * Set containing references to all composite types.
	 */
	RttiSet compositeTypes;

	/**
	 * Map containing all methods.
	 */
	RttiMethodMap methods;

	/**
	 * Map containing all properties.
	 */
	RttiPropertyMap properties;

	/**
	 * Default constructor, initializes the name of the type described by the
	 * RttiSet with "unknown".
	 *
	 * @param native is the native C++ type information for which the type
	 * information is being built.
	 */
	RttiBuilderBase(const std::type_info &native)
	    : native(native), currentName("unknown"){};

	/**
	 * Default constructor, initializes the name of the type described by the
	 * RttiSet with the given name.
	 *
	 * @param native is the native C++ type information for which the type
	 * information is being built.
	 * @param name is the initial name of the type described by the type
	 * builder.
	 */
	RttiBuilderBase(const std::type_info &native, std::string name)
	    : native(native), currentName(std::move(name)){};

	/**
	 * Sets the human readable name of the type information being built to the
	 * given string.
	 *
	 * @param s is the name to which the name should be set.
	 * @return a reference to the current RttiBuilderBase instance to allow
	 * method chaining.
	 */
	RttiBuilderBase &name(const std::string &s)
	{
		currentName = s;
		return *this;
	}

	/**
	 * Adds the given type descriptor as "parent" of the type information that
	 * is being built by this RttiBuilderBase instance.
	 *
	 * @param p is the pointer to the type descriptor that should be added.
	 * @return a reference to the current RttiBuilderBase instance to allow
	 * method chaining.
	 */
	RttiBuilderBase &parent(const Rtti *p)
	{
		parentTypes.insert(p);
		return *this;
	}

	/**
	 * Adds the given type descriptors as "parent" of the type information that
	 * is being built by this RttiBuilderBase instance.
	 *
	 * @param p is the pointer to the type descriptor that should be added.
	 * @return a reference to the current RttiBuilderBase instance to allow
	 * method chaining.
	 */
	RttiBuilderBase &parent(const RttiSet &p)
	{
		parentTypes.insert(p.begin(), p.end());
		return *this;
	}

	/**
	 * Marks the current type being built by this RttiBuilderBase instance as
	 * being a composition of the given other type.
	 *
	 * @param p is the pointer to the type descriptor that should be added as
	 * composition type.
	 * @return a reference to the current RttiBuilderBase instance to allow
	 * method chaining.
	 */
	RttiBuilderBase &composedOf(const Rtti *p)
	{
		compositeTypes.insert(p);
		return *this;
	}

	/**
	 * Marks the current type being built by this RttiBuilderBase instance as
	 * being a composition of the given other types.
	 *
	 * @param p is the pointer to the type descriptor that should be added as
	 * composition type.
	 * @return a reference to the current RttiBuilderBase instance to allow
	 * method chaining.
	 */
	RttiBuilderBase &composedOf(const RttiSet &p)
	{
		compositeTypes.insert(p.begin(), p.end());
		return *this;
	}

	/**
	 * Registers a generic (no particular C++ type given) method for this RTTI
	 * type descriptor.
	 *
	 * @param name is the name of the method. Names must be unique for one
	 * Rtti instance. If the name is not unique, an exception is thrown.
	 * @param function is the function that should be registered.
	 * @return a reference to the current RttiBuilderBase instance to allow
	 * method chaining.
	 */
	RttiBuilderBase &genericMethod(const std::string &name,
	                               std::shared_ptr<Function> function);

	/**
	 * Registers a generic (no particular C++ type given) property descriptor
	 * for this RTTI type descriptor.
	 *
	 * @param name is the name of the property. Names must be unique for one
	 * Rtti instance. If the property is not unique, an exception is thrown.
	 * @param property is the property that should be registered.
	 * @return a reference to the current RttiBuilderBase instance to allow
	 * method chaining.
	 */
	RttiBuilderBase &genericProperty(
	    const std::string &name, std::shared_ptr<PropertyDescriptor> property);
};

/**
 * The Rtti class allows for attaching data to native types that can be
 * accessed at runtime. This type information can e.g. be retrieved using the
 * "type" method of the Managed class. This system is used for attaching human
 * readable names, parent types and script engine functionality. Use the
 * Rtti class for convenient registration of type information.
 */
class Rtti {
private:
	/**
	 * Set to true if once the parents and the composite types list have been
	 * completed (by including the parents of the original parent elements and
	 * the composite types of the original composite types).
	 */
	mutable bool initialized;

	/**
	 * Set containing references to all parent types, including their parents.
	 */
	mutable RttiSet parents;

	/**
	 * Set containing references to all types this type is a composition of,
	 * including all composite types of the original composite types.
	 */
	mutable RttiSet compositeTypes;

	/**
	 * Map used for storing all registered methods.
	 */
	mutable RttiMethodMap methods;

	/**
	 * Map used for storing all registered properties.
	 */
	mutable RttiPropertyMap properties;

	/**
	 * Adds the parent types of the original parents and the composite types of
	 * the original composite types to the internal sets for faster lookup.
	 */
	void initialize() const;

public:
	/**
	 * Human readable name associated with the type.
	 */
	const std::string name;

	/**
	 * Creates a new Rtti instance and registers it in the global type
	 * table. Use the Rtti class for more convenient registration of type
	 * information.
	 *
	 * @param builder is the builder instance containing the Rtti data.
	 */
	Rtti(const RttiBuilderBase &builder)
	    : initialized(false),
	      parents(std::move(builder.parentTypes)),
	      compositeTypes(std::move(builder.compositeTypes)),
	      methods(std::move(builder.methods)),
	      properties(std::move(builder.properties)),
	      name(std::move(builder.currentName))
	{
		RttiStore::store(builder.native, this);
	}

	/**
	 * Default constructor. Creates a Rtti instance with name "unknown"
	 * and no parents.
	 */
	Rtti() : name("unknown") {}

	/**
	 * Constructor for an empty Rtti with the given name.
	 */
	Rtti(std::string name) : name(std::move(name)) {}

	/**
	 * Returns true if this Rtti instance is the given type or has the
	 * given type as one of its parents.
	 *
	 * @param other is the other type for which the relation to this type
	 * should be checked.
	 * @return true if this type (directly or indirectly) has the given other
	 * type as parent or equals the other type.
	 */
	bool isa(const Rtti *other) const;

	/**
	 * Returns true if this Rtti instance is one of the given types.
	 *
	 * @param others is a set of other types to be checked.
	 * @return true if this type (directly or indirectly) has once of the given
	 * other types as parent or equals one of the other types.
	 */
	bool isOneOf(const RttiSet &others) const;

	/**
	 * Checks whether any type in the first set is one type in the second set.
	 *
	 * @param s1 is the first set. For each type in this set we check whether
	 * it is one of the types in s2.
	 * @param s2 is the second set.
	 * @return true if the above condition is fulfilled, false otherwise.
	 */
	static bool setIsOneOf(const RttiSet &s1, const RttiSet &s2);

	/**
	 * Calculates the intersection of two RttiSets. Only the elements of s1
	 * which are at least one element in s2 are returned.
	 *
	 * @param s1 is the first set. For each type in this set we check whether
	 * it is one of the types in s2, only those elements are returned.
	 * @param s2 is the second set.
	 * @return s1 restricted to the types in s2.
	 */
	static RttiSet setIntersection(const RttiSet &s1, const RttiSet &s2);

	/**
	 * Returns true if an instance of this type may have references to the other
	 * given type. This mechanism is used to prune impossible paths when
	 * resolving objects of a certain type by name in an object graph.
	 *
	 * @param other is the other type for which should be checked whether this
	 * type is directly or indirectly composed of it.
	 */
	bool composedOf(const Rtti *other) const;

	/**
	 * Returns all methods that are registered for this type (and the parent
	 * types, where methods with the same name as those in the parent type
	 * shadow the parent name methods).
	 *
	 * @return a mapping between method name and shared pointers of the
	 * registered function.
	 */
	const RttiMethodMap &getMethods() const;

	/**
	 * Returns all properties that are registered for this type (and the parent
	 * types, where properties with the same name as those in the parent type
	 * shadow the parent name properties).
	 *
	 * @return a mapping between property name and the shared pointers of the
	 * registered properties.
	 */
	const RttiPropertyMap &getProperties() const;

	/**
	 * Searches for a method with the given name. Returns a shared pointer to
	 * that method if found or nullptr otherwise.
	 *
	 * @param name is the name of the method that should be looked up.
	 * @return a shared pointer pointing at the method with the given name
	 */
	std::shared_ptr<Function> getMethod(const std::string &name) const;

	/**
	 * Searches for a property with the given name. Returns a shared pointer to
	 * that property if found or nullptr otherwise.
	 *
	 * @param name is the name of the property that should be looked up.
	 * @return a shared pointer pointing at the property with the given name
	 */
	std::shared_ptr<PropertyDescriptor> getProperty(
	    const std::string &name) const;

	/**
	 * Returns true if a method with the given name is registered for this type.
	 *
	 * @param name is the name of the method that should be looked up.
	 * @return true if a method with this name exists, false otherwise.
	 */
	bool hasMethod(const std::string &name) const;

	/**
	 * Returns true if a property with the given name is registered for this
	 * type.
	 *
	 * @param name is the name of the property that should be looked up.
	 * @return true if a property with this name exists, false otherwise.
	 */
	bool hasProperty(const std::string &name) const;
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
inline const Rtti *typeOf()
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
inline const Rtti *typeOf(const T &obj)
{
	return RttiStore::lookup(typeid(obj));
}

namespace RttiTypes {
/**
 * Type of no particular type.
 */
extern const Rtti None;

/**
 * Nullptr type for use by the Variant::getRtti method.
 */
extern const Rtti Nullptr;

/**
 * Bool type for use by the Variant::getRtti method.
 */
extern const Rtti Bool;

/**
 * Integer type for use by the Variant::getRtti method.
 */
extern const Rtti Int;

/**
 * Double type for use by the Variant::getRtti method.
 */
extern const Rtti Double;

/**
 * String type for use by the Variant::getRtti method.
 */
extern const Rtti String;

/**
 * Array type for use by the Variant::getRtti method.
 */
extern const Rtti Array;

/**
 * Map type for use by the Variant::getRtti method.
 */
extern const Rtti Map;

/**
 * Cardinality type for use by the Variant::getRtti method.
 */
extern const Rtti Cardinality;

/**
 * Function type for use by the Variant::getRtti method.
 */
extern const Rtti Function;
}
}

#endif /* _OUSIA_RTTI_HPP_ */

