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

#include "Variant.hpp"

namespace ousia {

/**
 * The Function interface defines all the methods needed to represent a
 * generic function. Function objects can be called using the "call" function in
 * which an array of Variant is supplied to the function and a Variant is
 * returned to the caller.
 */
class Function {
protected:
	Function(){};

public:
	Function(const Function &) = delete;
	Function(Function &&) = delete;
	virtual ~Function(){};

	/**
	 * Abstract function which is meant to call the underlying function (be it
	 * a host or a script function) with the given arguments.
	 *
	 * @param args is a vector containing all arguments that shall be passed to
	 * the function.
	 * @return a Variant containing the return value.
	 */
	virtual Variant call(const Variant::arrayType &args = Variant::arrayType{},
	                     void *thisRef = nullptr) const = 0;
};

/**
 * Function doing nothing. Instances of this class are used as default values
 * for instances of the Function class.
 */
class FunctionStub : public Function {
public:
	/**
	 * Constructor of the FunctionStub class.
	 */
	FunctionStub() {}

	Variant call(const Variant::arrayType &, void *) const override
	{
		return nullptr;
	}
};

/**
 * The Method class refers to a method in the C++ code, belonging to an object
 * of a certain type T.
 *
 * @tparam T is the type of the method that should be called.
 */
template <class T>
class Method : public Function {
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
	using Callback = Variant (*)(const Variant::arrayType &args, T *thisRef);

private:
	/**
	 * Pointer at the actual C++ method being called.
	 */
	const Callback method;

public:
	/**
	 * Constructor of the Method class.
	 *
	 * @param method is a pointer at the C++ function that should be called.
	 */
	Method(Callback method) : method(method){};

	/**
	 * Calls the underlying method.
	 *
	 * @param args is a vector containing all arguments that should be passed
	 * to the method.
	 * @return a Variant containing the return value.
	 */
	Variant call(const Variant::arrayType &args = Variant::arrayType{},
	             void *thisRef = nullptr) const override
	{
		// Call the method
		return method(args, static_cast<T *>(thisRef));
	}
};
}

#endif /* _OUSIA_FUNCTION_HPP_ */

