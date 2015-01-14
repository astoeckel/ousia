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

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ousia {

class RttiType;
class Function;
class PropertyDescriptor;

/**
 * Type describing a set of RttiType pointers.
 */
using RttiTypeSet = std::unordered_set<const RttiType *>;

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
	static std::unordered_map<std::type_index, const RttiType *> &table();

public:
	/**
	 * Registers the given pointer to the RttiType class in the RTTI table. Does
	 * not override information for already registered types.
	 *
	 * @param native is a reference at the native type information provided
	 * by the compiler.
	 * @param rtti is a pointer pointing at the type information that should be
	 * stored for this type.
	 */
	static void store(const std::type_info &native, const RttiType *rtti);

	/**
	 * Looks up the type information stored for the given native type
	 * information.
	 */
	static const RttiType &lookup(const std::type_info &native);
};

/**
 * The RttiBuilder class is used to conveniently build new instances of the Rtti
 * or the RttiType class. It follows the "Builder" pattern and allows to create
 * the properties of the RttiType class by chaining method calls. The RttiType
 * and Rtti class can be constructed from the RttiBuilder instance.
 */
class RttiBuilder {
public:
	/**
	 * Contains the human readable name of the type for which the type
	 * information is being built.
	 */
	std::string currentName;

	/**
	 * Set containing references to all parent types.
	 */
	RttiTypeSet parentTypes;

	/**
	 * Set containing references to all composite types.
	 */
	RttiTypeSet compositeTypes;

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
	 * RttiTypeSet with "unknown".
	 */
	RttiBuilder() : currentName("unknown"){};

	/**
	 * Default constructor, initializes the name of the type described by the
	 * RttiTypeSet with the given name.
	 *
	 * @param name is the initial name of the type described by the type
	 * builder.
	 */
	RttiBuilder(std::string name) : currentName(std::move(name)){};

	/**
	 * Sets the human readable name of the type information being built to the
	 * given string.
	 *
	 * @param s is the name to which the name should be set.
	 * @return a reference to the current RttiBuilder reference to allow method
	 * chaining.
	 */
	RttiBuilder &name(const std::string &s)
	{
		currentName = s;
		return *this;
	}

	/**
	 * Adds the given type descriptor as "parent" of the type information that
	 * is being built by this RttiBuilder instance.
	 *
	 * @param p is the pointer to the type descriptor that should be added.
	 * @return a reference to the current RttiBuilder reference to allow method
	 * chaining.
	 */
	RttiBuilder &parent(const RttiType *p)
	{
		parentTypes.insert(p);
		return *this;
	}

	/**
	 * Adds the given type descriptors as "parent" of the type information that
	 * is being built by this RttiBuilder instance.
	 *
	 * @param p is a
	 * @return a reference to the current RttiBuilder reference to allow method
	 * chaining.
	 */
	RttiBuilder &parent(const RttiTypeSet &p)
	{
		parentTypes.insert(p.begin(), p.end());
		return *this;
	}

	/**
	 * Marks the current type being built by this RttiBuilder instance as being
	 * a composition of the given other type.
	 *
	 * @param p is the pointer to the type descriptor that should be added as
	 * composition type.
	 * @return a reference to the current RttiBuilder reference to allow method
	 * chaining.
	 */
	RttiBuilder &composedOf(const RttiType *p)
	{
		compositeTypes.insert(p);
		return *this;
	}

	/**
	 * Marks the current type being built by this RttiBuilder instance as being
	 * a composition of the given other types.
	 *
	 * @param p is the pointer to the type descriptor that should be added as
	 * composition type.
	 * @return a reference to the current RttiBuilder reference to allow method
	 * chaining.
	 */
	RttiBuilder &composedOf(const RttiTypeSet &p)
	{
		compositeTypes.insert(p.begin(), p.end());
		return *this;
	}

	/**
	 * Registers a generic (no particular C++ type given) method for this RTTI
	 * type descriptor.
	 *
	 * @param name is the name of the method. Names must be unique for one
	 * RttiType instance. If the name is not unique, an exception is thrown.
	 * @param function is the function that should be registered.
	 * @return a reference to the current RttiBuilder reference to allow method
	 * chaining.
	 */
	RttiBuilder &genericMethod(const std::string name,
	                           std::shared_ptr<Function> function);

	/**
	 * Registers a generic (no particular C++ type given) property descriptor
	 * for this RTTI type descriptor.
	 *
	 * @param name is the name of the property. Names must be unique for one
	 * RttiType instance. If the property is not unique, an exception is thrown.
	 * @param property is the property that should be registered.
	 * @return a reference to the current RttiBuilder reference to allow method
	 * chaining.
	 */
	RttiBuilder &genericProperty(const std::string name,
	                             std::shared_ptr<PropertyDescriptor> property);
};

/**
 * The RttiType class allows for attaching data to native types that can be
 * accessed at runtime. This type information can e.g. be retrieved using the
 * "type" method of the Managed class. This system is used for attaching human
 * readable names, parent types and script engine functionality. Use the
 * RttiType class for convenient registration of type information.
 */
class RttiType {
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
	mutable RttiTypeSet parents;

	/**
	 * Set containing references to all types this type is a composition of,
	 * including all composite types of the original composite types.
	 */
	mutable RttiTypeSet compositeTypes;

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

protected:
	/**
	 * Creates a new RttiType instance and registers it in the global type
	 * table. Use the Rtti and the RttiBuilder class for more convenient
	 * registration of type information.
	 *
	 * @param name is the name of the type.
	 * @param native is a reference at the native type information provided by
	 * the compiler.
	 * @param parents is a list of parent types.
	 * @param compositeTypes is a list of types of which instances of this type
	 * are composited (consist of).
	 */
	RttiType(std::string name, const std::type_info &native,
	         RttiTypeSet parents = RttiTypeSet{},
	         RttiTypeSet compositeTypes = RttiTypeSet{},
	         RttiMethodMap methods = RttiMethodMap{},
	         RttiPropertyMap properties = RttiPropertyMap{})
	    : initialized(false),
	      parents(std::move(parents)),
	      compositeTypes(compositeTypes),
	      methods(std::move(methods)),
	      properties(std::move(properties)),
	      name(std::move(name))
	{
		RttiStore::store(native, this);
	}

	/**
	 * Creates a new RttiType instance and registers it in the global type
	 * table. Use the Rtti class for more convenient registration of type
	 * information.
	 *
	 * @param builder is the builder instance containing the Rtti data.
	 */
	RttiType(const std::type_info &native, const RttiBuilder &builder)
	    : initialized(false),
	      parents(std::move(builder.parentTypes)),
	      compositeTypes(std::move(builder.compositeTypes)),
	      methods(std::move(builder.methods)),
	      properties(std::move(builder.properties)),
	      name(std::move(builder.currentName))
	{
		RttiStore::store(native, this);
	}

public:
	/**
	 * Human readable name associated with the type.
	 */
	const std::string name;

	/**
	 * Default constructor. Creates a Rtti instance with name "unknown"
	 * and no parents.
	 */
	RttiType() : name("unknown") {}

	/**
	 * Constructor for an empty RttiType with the given name.
	 */
	RttiType(std::string name) : name(std::move(name)) {}

	/**
	 * Returns true if this Rtti instance is the given type or has the
	 * given type as one of its parents.
	 *
	 * @param other is the other type for which the relation to this type
	 * should be checked.
	 */
	bool isa(const RttiType &other) const;

	/**
	 * Returns true if an instance of this type may have references to the other
	 * given type. This mechanism is used to prune impossible paths when
	 * resolving objects of a certain type by name in an object graph.
	 *
	 * @param other is the other type for which should be checked whether this
	 * type is directly or indirectly composed of it.
	 */
	bool composedOf(const RttiType &other) const;

	/**
	 * Returns all methods that are registered for this type (and the parent
	 * types, where methods with the same name as those in the parent type
	 * shadow the parent name methods).
	 *
	 * @return a mapping between method name and shared pointers of the
	 * registered function.
	 */
	const RttiMethodMap& getMethods() const;

	/**
	 * Returns all properties that are registered for this type (and the parent
	 * types, where properties with the same name as those in the parent type
	 * shadow the parent name properties).
	 *
	 * @return a mapping between property name and the shared pointers of the
	 * registered properties.
	 */
	const RttiPropertyMap& getProperties() const;

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
	std::shared_ptr<PropertyDescriptor> getProperty(const std::string &name) const;

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
class Rtti : public RttiType {
public:
	/**
	 * Creates a new Rtti instance and registers it in the global type table.
	 *
	 * @param name is the name of the type.
	 */
	Rtti(std::string name) : RttiType(name, typeid(T)) {}

	/**
	 * Creates a new Rtti instance from the data stored in the given builder
	 * instance and registers it in the global type table.
	 *
	 * @param builder is the RttiBuilder instance containing the data from which
	 * the Rtti information should be copied.
	 */
	Rtti(const RttiBuilder &builder) : RttiType(typeid(T), builder){};
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
inline const RttiType &typeOf()
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
inline const RttiType &typeOf(const T &obj)
{
	return RttiStore::lookup(typeid(obj));
}

namespace RttiTypes {
/**
 * Type of no particular type.
 */
extern const RttiType None;

/**
 * Nullptr type for use by the Variant::getRttiType method.
 */
extern const RttiType Nullptr;

/**
 * Bool type for use by the Variant::getRttiType method.
 */
extern const RttiType Bool;

/**
 * Integer type for use by the Variant::getRttiType method.
 */
extern const RttiType Int;

/**
 * Double type for use by the Variant::getRttiType method.
 */
extern const RttiType Double;

/**
 * String type for use by the Variant::getRttiType method.
 */
extern const RttiType String;

/**
 * Array type for use by the Variant::getRttiType method.
 */
extern const RttiType Array;

/**
 * Map type for use by the Variant::getRttiType method.
 */
extern const RttiType Map;

/**
 * Function type for use by the Variant::getRttiType method.
 */
extern const RttiType Function;
}
}

#endif /* _OUSIA_RTTI_HPP_ */

