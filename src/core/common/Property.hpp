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
 * @file Property.hpp
 *
 * Contains classes for describing properties, which allow to generically access
 * object members via Getter and Setter functions. This functionality is needed
 * for building scripting language interfaces.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PROPERTY_HPP_
#define _OUSIA_PROPERTY_HPP_

#include <memory>

#include "Exceptions.hpp"
#include "Function.hpp"
#include "Variant.hpp"

namespace ousia {

/**
 * Exception type used for signaling exceptions in the context of Properties,
 * such as calls to not set Getters or Setters.
 */
class PropertyException : public LoggableException {
public:
	using LoggableException::LoggableException;
};

// Forward declaration
class Rtti;
namespace RttiTypes {
extern const Rtti None;
}

/**
 * Structure describing the type of a property -- which consists of a "outer"
 * type (which may either be a primitive variant type such as e.g.
 * RttiTypes::Int or any other Rtti instance) and an inner type, which
 * describes the type contained within a container type such as RttiTypes::Array
 * or RttiTypes::Map.
 */
struct PropertyType {
	// Variable declaring the empty property type.
	static const PropertyType None;

	/**
	 * Outer type, may be any Rtti instance. If set to RttiTypes::None, any
	 * outer type is acceptable.
	 */
	const Rtti *type;

	/**
	 * Describes the inner type of the property used when the outer type is a
	 * container type such as RttiTypes::Array or RttiTypes::Map. If set to
	 * RttiTypes::None any inner type is acceptable.
	 */
	const Rtti *innerType;

	/**
	 * Creates a new instance of the PropertyType class with both inner and
	 * outer type being set to RttiTypes::None and thus allowing any type to be
	 * represented by this property.
	 */
	PropertyType() : type(&RttiTypes::None), innerType(&RttiTypes::None){};

	/**
	 * Creates a new instance of the PropertyType class with the given outer
	 * type.
	 *
	 * @param type is the "outer" type of the PropertyType instance which may
	 * be any Rtti instances or RttiTypes::None, in which case all types are
	 * allowed.
	 */
	PropertyType(const Rtti *type)
	    : type(type), innerType(&RttiTypes::None){};

	/**
	 * Creates a new instance of the PropertyType class with the given outer and
	 * inner type.
	 *
	 * @param type is the "outer" type of the PropertyType instance which may
	 * be any Rtti instances or RttiTypes::None, in which case all types are
	 * allowed.
	 * @param innerType is the inner type of the PropertyType instance, which is
	 * relevant if the outer type is set to a basic container type, namely
	 * RttiTypes::Array or RttiTypes::Map.
	 */
	PropertyType(const Rtti *type, const Rtti *innerType)
	    : type(type), innerType(innerType){};
};

/**
 * Represents an abstract Function with a property type attached to it. This is
 * used as base class for Getter and Setter classes.
 */
class PropertyFunction : public Function {
private:
	/**
	 * Boolean field indicating whether the function is valid or not (a callback
	 * function was provided or not
	 */
	bool valid;

protected:
	/**
	 * Constructor of the PropertyFunction class with a preset propertyType
	 * reference.
	 *
	 * @param valid specifies whether a callback function was given or not.
	 */
	PropertyFunction(bool valid)
	    : valid(valid), propertyType(nullptr){};

public:
	/**
	 * Returns the type associated with the property function.
	 */
	std::shared_ptr<PropertyType> propertyType;

	/**
	 * Returns true if a callback function was given, false otherwise.
	 */
	bool isValid() { return valid; }
};

/**
 * Abstract function type representing a Getter function. Provides validation
 * functions, yet does not have the ability to actually call a Getter. This
 * functionality is provided by the Getter template class, which provides an
 * easy to use callback mechanism.
 */
class GetterFunction : public PropertyFunction {
protected:
	/**
	 * Makes sure no arguments are given, throws an exception if any arguments
	 * are provided.
	 *
	 * @param args is an array containing the arguments.
	 */
	void validateArguments(Variant::arrayType &args) const;

	/**
	 * Makes sure the result adhers to the specified property type. Throws an
	 * exception if this is not the case.
	 *
	 * @param res is the result that should be validated.
	 */
	void validateResult(Variant &res) const;

	using PropertyFunction::PropertyFunction;

public:
	/**
	 * Returns the value of the property for the given object.
	 *
	 * @param obj is the instance for which the value of the property should be
	 * returned.
	 * @return the value retrieved from the object pointed at by obj.
	 */
	Variant get(void *obj);
};

/**
 * Class representing the getter function of a property.
 *
 * @tparam T is the type of the object on which the getter should be executed.
 */
template <class T>
class Getter : public GetterFunction {
public:
	/**
	 * Callback function type used to access the getter.
	 *
	 * @param thisRef is a reference to the object from which the value should
	 * be retrieved.
	 * @return the retrieved value. The result is checked to be of the specified
	 * type of the property.
	 */
	using Callback = Variant (*)(const T *thisRef);

private:
	/**
	 * Callback function pointer used to access the getter.
	 */
	Callback callback;

protected:
	/**
	 * Calls the callback function and validates the given arguments and the
	 * callback result.
	 *
	 * @param args is a list of arguments passed to the getter. Should be empty.
	 * @param thisRef is a reference to the object from which the value should
	 * be retrieved.
	 * @return the retrieved value. The result is checked to be of the specified
	 * type of the property.
	 */
	Variant doCall(Variant::arrayType &args, void *thisRef) const override
	{
		if (!callback) {
			throw PropertyException("Property is writeonly.");
		}

		// Make sure the input arguments are valid
		validateArguments(args);

		// Call the actual callback function and make sure the output
		// arguments
		// are valid
		Variant res = callback(static_cast<T *>(thisRef));
		validateResult(res);

		// Return the validated result
		return res;
	}

public:
	/**
	 * Creates an invalid getter instance with no callback function.
	 */
	Getter() : GetterFunction(false), callback(nullptr) {}

	/**
	 * Create a getter with the given callback function.
	 *
	 * @param callback is the underlying callback function to be used to
	 * get the value from an instance of type T.
	 */
	Getter(Callback callback)
	    : GetterFunction(callback != nullptr), callback(callback)
	{
	}
};

/**
 * Abstract function type representing a Setter function. Provides validation
 * functions, yet does not have the ability to actually call a Setter. This
 * functionality is provided by the Setter template class, which provides an
 * easy to use callback mechanism.
 */
class SetterFunction : public PropertyFunction {
protected:
	/**
	 * Makes sure exactly one argument with the specified type is given.
	 *
	 * @param args is an array containing the arguments.
	 */
	void validateArguments(Variant::arrayType &args) const;

	using PropertyFunction::PropertyFunction;

public:
	/**
	 * Sets the value of the property for the given object.
	 *
	 * @param value is the new property value that should be set.
	 * @param obj is the instance for which the value of the property should be
	 * returned.
	 */
	void set(const Variant &value, void *obj);
};

/**
 * Class representing the Setter function of a property.
 *
 * @tparam T is the type of the object on which the setter should be executed.
 */
template <class T>
class Setter : public SetterFunction {
public:
	/**
	 * Callback function type used to access the setter.
	 *
	 * @param value is the value that should be set. The value has been
	 * validated for compliance with the specified property type.
	 * @param thisRef is a reference to the object from which the value should
	 * be retrieved.
	 */
	using Callback = void (*)(const Variant &value, T *thisRef);

private:
	/**
	 * Callback function pointer used to access the setter.
	 */
	Callback callback;

protected:
	/**
	 * Calls the callback function and validates the given arguments and the
	 * callback result.
	 *
	 * @param args is a list of arguments passed to the setter. Should contain
	 * exactly one argument.
	 * @param thisRef is a reference to the object in which the value should
	 * be set.
	 * @return a variant containing nullptr.
	 */
	Variant doCall(Variant::arrayType &args, void *thisRef) const override
	{
		if (!callback) {
			throw PropertyException("Property is readonly.");
		}

		// Make sure the input argument is valid and call the callback function
		validateArguments(args);
		callback(args[0], static_cast<T *>(thisRef));
		return nullptr;
	}

public:
	/**
	 * Creates an invalid setter instance with no callback function.
	 */
	Setter() : SetterFunction(false), callback(nullptr) {}

	/**
	 * Create a setter with the given callback function.
	 *
	 * @param callback is the underlying callback function to be used to
	 * set the value of an instance of type T.
	 */
	Setter(Callback callback)
	    : SetterFunction(callback != nullptr), callback(callback)
	{
	}
};

/**
 * Class describing a generic Property of an object of a not-yet specified type.
 */
class PropertyDescriptor {
private:
	/**
	 * Description of the type of the property, consisting of an inner and an
	 * outer type.
	 */
	std::shared_ptr<PropertyType> type;

	/**
	 * Object used to read the value of the property.
	 */
	std::shared_ptr<GetterFunction> getter;

	/**
	 * Object used to write values of the property. The setter may be invalid
	 * in which case the property is read only.
	 */
	std::shared_ptr<SetterFunction> setter;

protected:
	/**
	 * Base constructor of the PropertyDescriptor class, called by all other
	 * constructors.
	 *
	 * @param type describes the type of the PropertyDescriptor.
	 * @param getter is the getter function used for reading the property. The
	 * getter function must be valid, writeonly properties are not supported.
	 * @param setter is the setter function used for writing the property. The
	 * setter function may be invalid, in which case the property is readonly.
	 */
	PropertyDescriptor(const PropertyType &type,
	                   std::shared_ptr<GetterFunction> getter,
	                   std::shared_ptr<SetterFunction> setter);

public:
	/**
	 * Returns true if this is a read only property.
	 *
	 * @return true if no (valid) setter was given in the constructor, false
	 * otherwise.
	 */
	bool isReadonly() const { return !(setter->isValid()); }

	/**
	 * Returns the type described by the property.
	 *
	 * @return the PropertyType instance describing the type of this property.
	 */
	const PropertyType *getType() const { return type.get(); }

	/**
	 * Returns the value of the property for the given object.
	 *
	 * @param obj is the instance for which the value of the property should be
	 * returned.
	 * @return the value retrieved from the object pointed at by obj.
	 */
	Variant get(void *obj) const { return getter->get(obj); }

	/**
	 * Sets the value of the property for the given object.
	 *
	 * @param value is the new property value that should be set.
	 * @param obj is the object for which the property should be updated.
	 */
	void set(const Variant &value, void *obj) const { setter->set(value, obj); }
};

/**
 * Class representing a Property of an object of type T. Provides convenient
 * constructors for the construction of the underlying PropertyDescriptor.
 *
 * @tparam T is the type with that field that should be accessed through this
 * property.
 */
template <class T>
class Property : public PropertyDescriptor {
public:
	/**
	 * Constructor of the Property class, creates a property with no type
	 * restrictions.
	 *
	 * @param getter is a Getter for accessing the described property for
	 * objects of type T.
	 * @param setter is a Setter for writing the described property for objects
	 * of type T.
	 */
	Property(const Getter<T> &getter, const Setter<T> &setter = Setter<T>{})
	    : PropertyDescriptor(
	          PropertyType{},
	          std::make_shared<Getter<T>>(getter),
	          std::make_shared<Setter<T>>(setter))
	{
	}

	/**
	 * Constructor of the Property class.
	 *
	 * @param type is the type of the field that can be accessed by the
	 * property. This may either be a primitive variant type such as e.g.
	 * RttiTypes::Int or any other Rtti instance
	 * @param getter is a Getter for accessing the described property for
	 * objects of type T.
	 * @param setter is a Setter for writing the described property for objects
	 * of type T.
	 */
	Property(const Rtti *type, const Getter<T> &getter,
	         const Setter<T> &setter = Setter<T>{})
	    : PropertyDescriptor(
	          PropertyType{type},
	          std::make_shared<Getter<T>>(getter),
	          std::make_shared<Setter<T>>(setter))
	{
	}

	/**
	 * Constructor of the Property class.
	 *
	 * @param type is the type of the field that can be accessed by the
	 * property. This may either be a primitive variant type such as e.g.
	 * RttiTypes::Int or any other Rtti instance.
	 * @param innerType is only relevant if type is set to either
	 * RttiTypes::Array or RttiTypes::Map. In this case the innerType describes
	 * the type of the elements stored inside these containers.
	 * @param getter is a Getter for accessing the described property for
	 * objects of type T.
	 * @param setter is a Setter for writing the described property for objects
	 * of type T.
	 */
	Property(const Rtti *type, const Rtti *innerType,
	         const Getter<T> &getter, const Setter<T> &setter = Setter<T>{})
	    : PropertyDescriptor(
	          PropertyType{type, innerType},
	          std::make_shared<Getter<T>>(getter),
	          std::make_shared<Setter<T>>(setter))
	{
	}

	/**
	 * Returns the value of the property for the given object.
	 *
	 * @param obj is the instance for which the value of the property should be
	 * returned.
	 * @return the value retrieved from the object pointed at by obj.
	 */
	Variant get(T *obj) { return PropertyDescriptor::get(obj); }

	/**
	 * Sets the value of the property for the given object.
	 *
	 * @param value is the new property value that should be set.
	 * @param obj is the object for which the property should be updated.
	 */
	void set(const Variant &value, T *obj)
	{
		PropertyDescriptor::set(value, obj);
	}
};
}

#endif /* _OUSIA_PROPERTY_HPP_ */

