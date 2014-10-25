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
 * The abstract Function class is most basic version of a function handle,
 * maintaining a "call" function and basic virtual functions for lifecyle
 * management.
 */
class Function {
public:
	/**
	 * Virtual clone function (e.g. used in the variant class).
	 */
	virtual Function *clone() const = 0;

	/**
	 * Virtual destructor.
	 */
	virtual ~Function() {}

	/**
	 * Abstract function which is meant to call the underlying function (be it
	 * a host or a script function) with the given arguments.
	 *
	 * @param args is a vector containing all arguments that shall be passed to
	 * the function.
	 * @return a Variant containing the return value.
	 */
	virtual Variant call(const std::vector<Variant> &args) const = 0;

	/**
	 * Calls the underlying function with no arguments.
	 *
	 * @return a Variant containing the return value.
	 */
	Variant call() const { return call({}); }

	// TODO: Use () operator instead of the call function
};

/**
 * The Argument class is used to describe the type of a function
 * argument.
 */
struct Argument {
	const VariantType type;
	const bool hasDefault;
	const Variant defaultValue;

	Argument(VariantType type) : type(type), hasDefault(false){};

	Argument(VariantType type, const Variant &defaultValue)
	    : type(type), hasDefault(true), defaultValue(defaultValue){};
};

/**
 * ArgumentValidatorError is an exception type used to represent argument
 * validator errors.
 */
class ArgumentValidatorError : public std::exception {
public:
	const int index;

	const std::string msg;

	ArgumentValidatorError(int index, const std::string &msg)
	    : index(index), msg(msg){};

	virtual const char *what() const noexcept override { return msg.c_str(); }
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
	const std::vector<Argument> signature;

	/**
	 * Argument index in the input array, at which the last error occured.
	 */
	int errorIndex = -1;

	/**
	 * Error message for the last validation error.
	 */
	std::string errorMessage;

	/**
	 * Used internally to update the errorIndex and the errorMessage fields.
	 */
	std::pair<bool, std::vector<Variant>> setError(int idx,
	                                               const std::string &msg,
	                                               std::vector<Variant> &res);

	/**
	 * Resets the error state.
	 */
	void resetError();

public:

	/**
	 * Constructor of the argument validator class.
	 *
	 * @param descriptors is a list of Arguments which should be used
	 * for the validation.
	 */
	ArgumentValidator(const std::vector<Argument> &signature)
	    : signature(signature)
	{
	}

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
	std::pair<bool, std::vector<Variant>> validate(
	    const std::vector<Variant> &args);

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
		return ArgumentValidatorError{errorIndex, errorMessage};
	}
};

/**
 * A validating function
 */
class ValidatingFunction : public Function {
private:
	/**
	 * Specifies whether the validating function should actually run or just
	 * pass the arguments through.
	 */
	bool validate;

	/**
	 * Signature for which the function should be validated.
	 */
	const std::vector<Argument> signature;

protected:
	virtual Variant validatedCall(const std::vector<Variant> &args) const = 0;

	virtual Variant call(const std::vector<Variant> &args) const override;

	using Function::call;

public:
	ValidatingFunction() : validate(false) {}

	ValidatingFunction(const std::vector<Argument> &signature)
	    : validate(true), signature(signature)
	{
	}
};

using HostFunctionCallback = Variant (*)(const std::vector<Variant> &args,
                                         void *data);
using GetterCallback = Variant (*)(void *data);
using SetterCallback = void (*)(Variant arg, void *data);

class HostFunction : public ValidatingFunction {
private:
	HostFunctionCallback callback;
	void *data;

protected:
	virtual Variant validatedCall(
	    const std::vector<Variant> &args) const override
	{
		return callback(args, data);
	}

public:
	HostFunction(HostFunctionCallback callback, std::vector<Argument> signature,
	             void *data = nullptr)
	    : ValidatingFunction(signature), callback(callback), data(data)
	{
	}

	HostFunction(HostFunctionCallback callback, void *data = nullptr)
	    : ValidatingFunction(), callback(callback), data(data)
	{
	}

	Function *clone() const override { return new HostFunction(*this); }

	using ValidatingFunction::call;
};

class Getter : public ValidatingFunction {
private:
	GetterCallback callback;
	void *data;

protected:
	virtual Variant validatedCall(
	    const std::vector<Variant> &args) const override
	{
		if (!callback) {
			// TODO: Use another exception class here
			throw "Getter not defined";
		}
		return callback(data);
	}

public:
	Getter(GetterCallback callback, void *data = nullptr)
	    : ValidatingFunction(std::vector<Argument>{}),
	      callback(callback),
	      data(data){};

	Function *clone() const override { return new Getter(*this); }

	Variant call() const { return ValidatingFunction::call(); }

	Variant operator()() const { return call(); }

	bool exists() const { return callback != nullptr; }
};

class Setter : public ValidatingFunction {
private:
	SetterCallback callback;
	void *data;

protected:
	virtual Variant validatedCall(
	    const std::vector<Variant> &args) const override
	{
		if (!callback) {
			// TODO: Use another exception class here
			throw "Setter not defined";
		}
		callback(args[0], data);
		return Variant::Null;
	}

public:
	Setter(VariantType type, SetterCallback callback, void *data = nullptr)
	    : ValidatingFunction({Argument{type}}),
	      callback(callback),
	      data(data){};

	Function *clone() const override { return new Setter(*this); }

	void call(Variant arg) const { ValidatingFunction::call({arg}); }

	void operator()(Variant arg) const { return call(arg); }

	bool exists() const { return callback != nullptr; }
};
}
}

#endif /* _OUSIA_FUNCTION_HPP_ */

