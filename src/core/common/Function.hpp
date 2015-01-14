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
 * @file Function.hpp
 *
 * Contains the definition of a Function class used to describe both methods and
 * functions in the host code and functions in the script code.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_FUNCTION_HPP_
#define _OUSIA_FUNCTION_HPP_

#include <cassert>

#include "Argument.hpp"
#include "Variant.hpp"

namespace ousia {

/**
 * The Function interface defines all the methods needed to represent a
 * generic function. Function objects can be called using the "call" function in
 * which an array of Variant is supplied to the function and a Variant is
 * returned to the caller. The actual function that is being represented by an
 * instance of the Function class may either be a C++ function or a function
 * residing in some script.
 */
class Function {
protected:
	/**
	 * Protecte default constructor -- prevents the Function class from being
	 * created. Use one of the child classes instead.
	 */
	Function(){};

	/**
	 * Abstract function which is meant to call the underlying function (be it
	 * a host or a script function) with the given arguments.
	 *
	 * @param args is a vector containing all arguments that shall be passed to
	 * the function.
	 * @return a Variant containing the return value.
	 */
	virtual Variant doCall(Variant::arrayType &args, void *thisRef) const = 0;

public:

	/**
	 * Virtual destructor of the Function class.
	 */
	virtual ~Function(){};

	/**
	 * Calls the function.
	 *
	 * @param args is an array of variants that should be passed to the
	 * function. Note that the arguments might be modified, e.g. by a validation
	 * process or the called function itself.
	 * @param thisRef is a user-defined reference which may be pointing at the
	 * object the function should be working on.
	 * @return a Variant containing the result of the function call.
	 */
	Variant call(Variant::arrayType &args, void *thisRef = nullptr) const
	{
		return doCall(args, thisRef);
	}

	/**
	 * Calls the function.
	 *
	 * @param args is an array of variants that should be passed to the
	 * function.
	 * @param thisRef is a user-defined reference which may be pointing at the
	 * object the function should be working on.
	 * @return a Variant containing the result of the function call.
	 */
	Variant call(const Variant::arrayType &args = Variant::arrayType{},
	             void *thisRef = nullptr) const
	{
		Variant::arrayType argsCopy = args;
		return doCall(argsCopy, thisRef);
	}
};

/**
 * Function doing nothing. Instances of this class are used as default values
 * for instances of the Function class.
 */
class FunctionStub : public Function {
protected:
	Variant doCall(Variant::arrayType &, void *) const override
	{
		return nullptr;
	}

public:
	/**
	 * Constructor of the FunctionStub class.
	 */
	FunctionStub() {}
};

/**
 * Function class providing factilities for the validation of arguments.
 */
class ValidatingFunction : public Function {
private:
	/**
	 * List describing a valid set to arguments.
	 */
	Arguments arguments;

protected:
	/**
	 * Default constructor. Disables validation, all arguments are allowed.
	 */
	ValidatingFunction() : arguments(Arguments::None){};

	/**
	 * Default constructor. Disables validation, all arguments are allowed.
	 */
	ValidatingFunction(Arguments arguments)
	    : arguments(std::move(arguments)) {};

	/**
	 * Function which cares about validating a set of arguments.
	 *
	 * @param args is an array containing the arguments that should be
	 * validated.
	 * @return the reference to the array.
	 */
	Variant::arrayType &validate(Variant::arrayType &args) const;
};

/**
 * The Method class refers to a method in the C++ code, belonging to an object
 * of a certain type T.
 *
 * @tparam T is the type of the method that should be called.
 */
template <class T>
class Method : public ValidatingFunction {
public:
	/**
	 * Type of the Callback function that is being called by the "call"
	 * function.
	 *
	 * @param args contains the input arguments that were passed to the
	 * function.
	 * @param thisRef is a pointer pointing at an instance of type T.
	 * @return the return value of the function as Variant instance.
	 */
	using Callback = Variant (*)(Variant::arrayType &args, T *thisRef);

private:
	/**
	 * Pointer at the actual C++ method being called.
	 */
	const Callback method;

protected:
	/**
	 * Calls the underlying method.
	 *
	 * @param args is a vector containing all arguments that should be passed
	 * to the method.
	 * @return a Variant containing the return value.
	 */
	Variant doCall(Variant::arrayType &args, void *thisRef) const override
	{
		return method(validate(args), static_cast<T *>(thisRef));
	}

public:
	/**
	 * Constructor of the Method class with a description of the arguments that
	 * are to be passed to the callback method.
	 *
	 * @param arguments is a type description restricting the arguments that are
	 * being passed to the callback function.
	 * @param method is the actual callback function that is being called once
	 * the method is executed. The arguments passed to the method are validated
	 * using the given argument descriptor.
	 */
	Method(Arguments arguments, Callback method)
	    : ValidatingFunction(arguments), method(method){};

	/**
	 * Constructor of the Method class.
	 *
	 * @param method is a pointer at the C++ function that should be called.
	 */
	Method(Callback method) : method(method){};
};
}

#endif /* _OUSIA_FUNCTION_HPP_ */

