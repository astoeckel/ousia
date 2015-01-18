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
 * @file RttiBuilder.hpp
 *
 * Defines a more convenient version of the RttiBuilder.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_RTTI_BUILDER_HPP_
#define _OUSIA_RTTI_BUILDER_HPP_

#include "Argument.hpp"
#include "Rtti.hpp"
#include "Function.hpp"
#include "Property.hpp"

namespace ousia {

/**
 * The RttiBuilder class is a more convenient version of the RttiBuilderBase
 * class which allows simple definition of new methods and properties.
 *
 * @tparam T is the C++ class for which the type is being built.
 */
template <class T>
class RttiBuilder : public RttiBuilderBase {
public:
	/**
	 * Default constructor, initializes the name of the type described by the
	 * RttiTypeSet with "unknown".
	 */
	RttiBuilder() : RttiBuilderBase(typeid(T)){};

	/**
	 * Default constructor, initializes the name of the type described by the
	 * RttiTypeSet with the given name.
	 *
	 * @param name is the initial name of the type described by the type
	 * builder.
	 */
	RttiBuilder(std::string name) : RttiBuilderBase(typeid(T), name){};

	/**
	 * Sets the human readable name of the type information being built to the
	 * given string.
	 *
	 * @param s is the name to which the name should be set.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &name(const std::string &s)
	{
		RttiBuilderBase::name(s);
		return *this;
	}

	/**
	 * Adds the given type descriptor as "parent" of the type information that
	 * is being built by this RttiBuilder instance.
	 *
	 * @param p is the pointer to the type descriptor that should be added.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &parent(const RttiType *p)
	{
		RttiBuilderBase::parent(p);
		return *this;
	}

	/**
	 * Adds the given type descriptors as "parent" of the type information that
	 * is being built by this RttiBuilder instance.
	 *
	 * @param p is the pointer to the type descriptor that should be added.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &parent(const RttiTypeSet &p)
	{
		RttiBuilderBase::parent(p);
		return *this;
	}

	/**
	 * Marks the current type being built by this RttiBuilder instance as being
	 * a composition of the given other type.
	 *
	 * @param p is the pointer to the type descriptor that should be added as
	 * composition type.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &composedOf(const RttiType *p)
	{
		RttiBuilderBase::composedOf(p);
		return *this;
	}

	/**
	 * Marks the current type being built by this RttiBuilder instance as being
	 * a composition of the given other types.
	 *
	 * @param p is the pointer to the type descriptor that should be added as
	 * composition type.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &composedOf(const RttiTypeSet &p)
	{
		RttiBuilderBase::composedOf(p);
		return *this;
	}

	/**
	 * Registers a generic (no particular C++ type given) method for this RTTI
	 * type descriptor.
	 *
	 * @param name is the name of the method. Names must be unique for one
	 * RttiType instance. If the name is not unique, an exception is thrown.
	 * @param function is the function that should be registered.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &genericMethod(const std::string &name,
	                               std::shared_ptr<Function> function)
	{
		RttiBuilderBase::genericMethod(name, function);
		return *this;
	}

	/**
	 * Registers a generic (no particular C++ type given) property descriptor
	 * for this RTTI type descriptor.
	 *
	 * @param name is the name of the property. Names must be unique for one
	 * RttiType instance. If the property is not unique, an exception is thrown.
	 * @param property is the property that should be registered.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &genericProperty(
	    const std::string &name, std::shared_ptr<PropertyDescriptor> property)
	{
		RttiBuilderBase::genericProperty(name, property);
		return *this;
	}

	/**
	 * Registers a method for this RTTI type descriptor.
	 *
	 * @param name is the name of the method. Names must be unique for one
	 * RttiType instance. If the name is not unique, an exception is thrown.
	 * @param method is the function that should be registered.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &method(const std::string name, const Method<T> &method)
	{
		return genericMethod(name, std::make_shared<Method<T>>(method));
	}

	/**
	 * Registers a method for this RTTI type descriptor.
	 *
	 * @param name is the name of the method. Names must be unique for one
	 * RttiType instance. If the name is not unique, an exception is thrown.
	 * @param method is the function that should be registered.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &method(const std::string name,
	                       const typename Method<T>::Callback &method)
	{
		return genericMethod(name, std::make_shared<Method<T>>(method));
	}

	/**
	 * Registers a property for this RTTI type descriptor.
	 *
	 * @param name is the name of the property. Names must be unique for one
	 * RttiType instance. If the property is not unique, an exception is thrown.
	 * @param property is the property that should be registered.
	 * @return a reference to the current RttiBuilder to allow method chaining.
	 */
	RttiBuilder<T> &property(const std::string name,
	                         const Property<T> &property)
	{
		RttiBuilderBase::genericProperty(
		    name, std::make_shared<Property<T>>(property));
		return *this;
	}
};
}

#endif /* _OUSIA_RTTI_BUILDER_HPP_ */

