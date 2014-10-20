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

#ifndef _OUSIA_FUNCTION_HPP_
#define _OUSIA_FUNCTION_HPP_

#include <exception>
#include <utility>
#include <vector>

#include "Variant.hpp"

namespace ousia {
namespace script {

/**
 * The abstract Function class is most basic version of a function handle --
 * just a virtual "call" function which calls the underlying code.
 */
class Function {

public:

	/**
	 * Abstract function which is meant to call the underlying function (be it
	 * a host or a script function) with the given arguments.
	 *
	 * @param args is a vector containing all arguments that shall be passed to
	 * the function.
	 * @return a Variant containing the return value.
	 */
	virtual Variant call(const std::vector<Variant> &args) const = 0;

};

/**
 * The ArgumentDescriptor class is used to describe the type of a function
 * argument.
 */
struct ArgumentDescriptor {

	const VariantType type;
	const bool hasDefault;
	const Variant defaultValue;

	ArgumentDescriptor(VariantType type) :
		type(type), hasDefault(false) {};

	ArgumentDescriptor(VariantType type, const Variant &defaultValue) :
		type(type), hasDefault(true), defaultValue(defaultValue) {};

};

/**
 * ArgumentValidatorError is an exception type used to represent argument
 * validator errors.
 */
class ArgumentValidatorError : std::exception {

public:

	const int index;

	const std::string msg;

	ArgumentValidatorError(int index, const std::string &msg) :
		index(index), msg(msg) {};

	virtual const char* what() const noexcept override
	{
		return msg.c_str();
	}

};

/**
 * The ArgumentValidator class is responsible for checking whether the given
 * arguments passed to a function match the description.
 */
class ArgumentValidator {

private:
	/**
	 * List containing the argument descriptors.
	 */
	const std::vector<ArgumentDescriptor> descriptors;

	/**
	 * Argument index in the input array, at which the last error occured.
	 */
	int errorIndex = -1;

	/**
	 * Error message for the last validation error.
	 */
	std::string errorMessage;

public:

	/**
	 * Constructor of the argument validator class.
	 *
	 * @param descriptors is a list of ArgumentDescriptors which should be used
	 * for the validation.
	 */
	ArgumentValidator(const std::vector<ArgumentDescriptor> &descriptors) :
		descriptors(descriptors) {}

	/**
	 * Validates and augments the given argument list (e.g. adds the default
	 * values).
	 *
	 * @param args contains the input arguments.
	 * @return a pair, where the first element specifies whether the arguments
	 * were validated sucessfully and the second argument contains the augmented
	 * list of arguments. If false is returned, use the error function to get
	 * more information about the error.
	 */
	std::pair<bool, std::vector<Variant>> validate(const std::vector<Variant> &args);

	/**
	 * Returns an ArgumentValidatorError instance containing the argument index
	 * in the input array, at which the error occured and an explaining error
	 * message. As ArgumentValidatorError is derived from std::exception,
	 * the result of this function is throwable.
	 *
	 * @return an ArgumentValidatorError instance containing information about
	 * the last error. If no error occurred, the message will be empty and
	 * the argument index will be set to -1.
	 */
	ArgumentValidatorError error()
	{
		return ArgumentValidatorError(errorIndex, errorMessage);
	}

};

/**
 * The HostFunction class represents a function that resides in the script host.
 */

}
}

#endif /* _OUSIA_FUNCTION_HPP_ */

