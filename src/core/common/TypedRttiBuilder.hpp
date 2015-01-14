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
 * @file TypedRttiBuilder.hpp
 *
 * Defines a more convenient version of the RttiBuilder.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_TYPED_RTTI_BUILDER_HPP_
#define _OUSIA_TYPED_RTTI_BUILDER_HPP_

#include "Argument.hpp"
#include "Rtti.hpp"
#include "Function.hpp"
#include "Property.hpp"

namespace ousia {

/**
 * The TypedRttiBuilder class is a more convenient version of the RttiBuilder
 * class which allows simple definition of new methods and properties.
 *
 * @tparam T is the C++ class for which the type is being built.
 */
template <class T>
class TypedRttiBuilder : public RttiBuilder {
public:
	using RttiBuilder::RttiBuilder;

	/**
	 * Sets the human readable name of the type information being built to the
	 * given string.
	 *
	 * @param s is the name to which the name should be set.
	 * @return a reference to the current TypedRttiBuilder reference to allow
	 * method chaining.
	 */
	TypedRttiBuilder<T> &name(const std::string &s)
	{
		RttiBuilder::name(s);
		return *this;
	}

	/**
	 * Adds the given type descriptor as "parent" of the type information that
	 * is being built by this RttiBuilder instance.
	 *
	 * @param p is the pointer to the type descriptor that should be added.
	 * @return a reference to the current TypedRttiBuilder reference to allow
	 * method chaining.
	 */
	TypedRttiBuilder<T> &parent(const RttiType *p)
	{
		RttiBuilder::parent(p);
		return *this;
	}

	/**
	 * Adds the given type descriptors as "parent" of the type information that
	 * is being built by this RttiBuilder instance.
	 *
	 * @param p is a
	 * @return a reference to the current TypedRttiBuilder reference to allow
	 * method chaining.
	 */
	TypedRttiBuilder<T> &parent(const RttiTypeSet &p)
	{
		RttiBuilder::parent(p);
		return *this;
	}

	/**
	 * Marks the current type being built by this RttiBuilder instance as being
	 * a composition of the given other type.
	 *
	 * @param p is the pointer to the type descriptor that should be added as
	 * composition type.
	 * @return a reference to the current TypedRttiBuilder reference to allow
	 * method chaining.
	 */
	TypedRttiBuilder<T> &composedOf(const RttiType *p)
	{
		RttiBuilder::composedOf(p);
		return *this;
	}

	/**
	 * Marks the current type being built by this RttiBuilder instance as being
	 * a composition of the given other types.
	 *
	 * @param p is the pointer to the type descriptor that should be added as
	 * composition type.
	 * @return a reference to the current TypedRttiBuilder reference to allow
	 * method chaining.
	 */
	TypedRttiBuilder<T> &composedOf(const RttiTypeSet &p)
	{
		RttiBuilder::composedOf(p);
		return *this;
	}

	/**
	 * Registers a method for this RTTI type descriptor.
	 *
	 * @param name is the name of the method. Names must be unique for one
	 * RttiType instance. If the name is not unique, an exception is thrown.
	 * @param method is the function that should be registered.
	 * @return a reference to the current TypedRttiBuilder reference to allow
	 * method chaining.
	 */
	TypedRttiBuilder<T> &method(const std::string name, const Method<T> &method)
	{
		RttiBuilder::genericMethod(name, std::make_shared<Method<T>>(method));
		return *this;
	}

	/**
	 * Registers a method for this RTTI type descriptor.
	 *
	 * @param name is the name of the method. Names must be unique for one
	 * RttiType instance. If the name is not unique, an exception is thrown.
	 * @param method is the function that should be registered.
	 * @return a reference to the current TypedRttiBuilder reference to allow
	 * method chaining.
	 */
	TypedRttiBuilder<T> &method(const std::string name,
	                            const typename Method<T>::Callback &method)
	{
		RttiBuilder::genericMethod(name, std::make_shared<Method<T>>(method));
		return *this;
	}

	/**
	 * Registers a property for this RTTI type descriptor.
	 *
	 * @param name is the name of the property. Names must be unique for one
	 * RttiType instance. If the property is not unique, an exception is thrown.
	 * @param property is the property that should be registered.
	 * @return a reference to the current TypedRttiBuilder reference to allow
	 * method chaining.
	 */
	TypedRttiBuilder<T> &property(const std::string name,
	                              const Property<T> &property)
	{
		RttiBuilder::genericProperty(name,
		                             std::make_shared<Property<T>>(property));
		return *this;
	}
};
}

#endif /* _OUSIA_TYPED_RTTI_BUILDER_HPP_ */

