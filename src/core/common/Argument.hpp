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
 * @file Argument.hpp
 *
 * Contains the declaration of the Argument and Arguments classes which are used
 * to define the list of Arguments that can be passed to a Method or the set
 * of attributes that are attached to an XML node or similar.
 *
 * The Argument and Arguments classes have some ressemblance to the Attribute
 * and StructType types, however the classes defined here have been built to
 * represent types which are known at compile time, whereas Attribute and
 * StructType represent types defined at runtime by the user.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_ARGUMENT_HPP_
#define _OUSIA_ARGUMENT_HPP_

#include <initializer_list>
#include <vector>
#include <unordered_map>

#include "Variant.hpp"

namespace ousia {

// Forward declaration
class Logger;
class RttiType;

/**
 * The Argument class represents a single argument that can be passed to a
 * function.
 */
class Argument {
private:
	/**
	 * Type that should be returned by the Variant rttiType function.
	 */
	const RttiType &type;

	/**
	 * Describes the inner type of the variant -- e.g. the type of the elements
	 * inside an array. Normally set to RttiType::None.
	 */
	const RttiType &innerType;

	/**
	 * Private constructor used for manually setting all internal data fields.
	 *
	 * @param name is the name of the Argument.
	 * @param variantType is the variant type of the argument that is to be
	 * expected.
	 * @param rttiType is the rttiType of the argument. Only used it the type
	 * of the variant is an object.
	 * @param defaultValue is the default value to be used.
	 * @param hasDefault indicates whether the defaultValue actually should be
	 * used.
	 */
	Argument(std::string name, const RttiType &type, const RttiType &innerType,
	         Variant defaultValue, bool hasDefault);

	/**
	 * Private constructor used to build an argument describing a primitive type
	 * with default value.
	 *
	 * @param name is the name of the Argument.
	 * @param variantType is the variant type of the argument that is to be
	 * expected.
	 * @param defaultValue is the default value to be used.
	 */
	Argument(std::string name, const RttiType &type, Variant defaultValue);

	/**
	 * Private constructor used to build an argument describing a primitive type
	 * without default value.
	 *
	 * @param name is the name of the Argument.
	 * @param variantType is the variant type of the argument that is to be
	 * expected.
	 */
	Argument(std::string name, const RttiType &type);

public:
	/**
	 * Contains the name of the argument. Used for logging and in case the
	 * arguments are presented as map.
	 */
	const std::string name;

	/**
	 * Default value. Note that a value of nullptr does not indicate that no
	 * default value has been set. Use the "hasDefault" flag for this purpose.
	 * Nullptr is a valid value for objects.
	 */
	const Variant defaultValue;

	/**
	 * True if a default value is set, false otherwise.
	 */
	const bool hasDefault;

	/**
	 * Named constructor for an argument with any type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @return a new Argument instance.
	 */
	static Argument Any(std::string name);

	/**
	 * Named constructor for an argument with any type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument Any(std::string name, Variant defaultValue);

	/**
	 * Named constructor for a boolean argument with no default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @return a new Argument instance.
	 */
	static Argument Bool(std::string name);

	/**
	 * Named constructor for a boolean argument with default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument Bool(std::string name, Variant::boolType defaultValue);

	/**
	 * Named constructor for an integer argument with no default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @return a new Argument instance.
	 */
	static Argument Int(std::string name);

	/**
	 * Named constructor for an integer argument with default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument Int(std::string name, Variant::intType defaultValue);

	/**
	 * Named constructor for a double argument with no default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @return a new Argument instance.
	 */
	static Argument Double(std::string name);

	/**
	 * Named constructor for a double argument with default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument Double(std::string name, Variant::doubleType defaultValue);

	/**
	 * Named constructor for a string argument with no default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @return a new Argument instance.
	 */
	static Argument String(std::string name);

	/**
	 * Named constructor for a string argument with default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument String(std::string name,
	                       const Variant::stringType &defaultValue);

	/**
	 * Named constructor for an object argument with no default value. Object
	 * arguments always point at an instance of the Managed class. The concrete
	 * Object type must be specified in the "type" argument.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param type is the RttiType of acceptable objects. All objects where the
	 * "isa" function returns true for the given type are be accepted.
	 * @return a new Argument instance.
	 */
	static Argument Object(std::string name, const RttiType &type);

	/**
	 * Named constructor for an object argument with default value. The default
	 * value can only be nullptr. Object arguments always point at an instance
	 * of the Managed class. The concrete Object type must be specified in the
	 * "type" argument.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param type is the RttiType of acceptable objects. All objects where the
	 * "isa" function returns true for the given type are be accepted.
	 * @return a new Argument instance.
	 */
	static Argument Object(std::string name, const RttiType &type,
	                       std::nullptr_t defaultValue);

	/**
	 * Named constructor for a function argument with no default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @return a new Argument instance.
	 */
	static Argument Function(std::string name);

	/**
	 * Named constructor for a function argument with default value.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument Function(std::string name,
	                         Variant::functionType defaultValue);

	/**
	 * Named constructor for an integer argument with no default and no specific
	 * inner type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @return a new Argument instance.
	 */
	static Argument Array(std::string name);

	/**
	 * Named constructor for an array argument with default value and no
	 * specific inner type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument Array(std::string name,
	                      const Variant::arrayType &defaultValue);

	/**
	 * Named constructor for an array argument of objects of the given RTTI
	 * type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param innerType is the inner type of the array. All array elements are
	 * forced to be of this type.
	 * @return a new Argument instance.
	 */
	static Argument Array(std::string name, const RttiType &innerType);

	/**
	 * Named constructor for an array argument of objects of the given RTTI
	 * type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param innerType is the inner type of the array. All array elements are
	 * forced to be of this type.
	 * @return a new Argument instance.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 */
	static Argument Array(std::string name, const RttiType &innerType,
	                      const Variant::arrayType &defaultValue);

	/**
	 * Named constructor for a map argument with no default value and no
	 * specific inner type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @return a new Argument instance.
	 */
	static Argument Map(std::string name);

	/**
	 * Named constructor for a map argument with default value and no specific
	 * inner type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument Map(std::string name, const Variant::mapType &defaultValue);

	/**
	 * Named constructor for a map argument with no default value and a given
	 * inner type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param innerType is the inner type of the map. All map entries are forced
	 * to be of this type.
	 * @return a new Argument instance.
	 */
	static Argument Map(std::string name, const RttiType &innerType);

	/**
	 * Named constructor for a map argument with default value and a given inner
	 * type.
	 *
	 * @param name is the name of the argument as used for error messages and in
	 * case the arguments are given as a map.
	 * @param innerType is the inner type of the map. All map entries are forced
	 * to be of this type.
	 * @param defaultValue is the default value to be used in case this argument
	 * is not supplied.
	 * @return a new Argument instance.
	 */
	static Argument Map(std::string name, const RttiType &innerType,
	                    const Variant::mapType &defaultValue);

	/**
	 * Makes sure the given variant is in the requested format and returns true
	 * if the variant was valid. Logs any error to the given logger instance.
	 * In case the validation was not successful, but the Argument instance was
	 * given an default value, the variant is set to that default value. If no
	 * default value was given, the variant is set to a valid value of the
	 * requested type.
	 *
	 * @param var is the variant that should be verified and transformed to
	 * match the argument specification.
	 * @param logger is the logger instance to which errors should be written.
	 * @return true if the given variant was valid, false otherwise.
	 */
	bool validate(Variant &var, Logger &logger);
};

/**
 * The Arguments class represents a list of Argument instances and allows to
 * either compare an array or a map of Variant instances against this argument
 * list.
 */
class Arguments {
private:
	/**
	 * List storing all arguments this instance consists of.
	 */
	std::vector<Argument> arguments;

	/**
	 * Map containing all used argument names.
	 */
	std::unordered_map<std::string, size_t> names;

public:
	/**
	 * Constructor of the Arguments class from a list of Argument instances.
	 *
	 * @param arguments is a list of Argument instances with which the Arguments
	 * instance should be initialized.
	 */
	Arguments(std::initializer_list<Argument> arguments);

	/**
	 * Checks whether the content of the given variant array matches the
	 * argument list stored in this Arguments instance. Any ommited default
	 * arguments are added to the array.
	 *
	 * @param arr is the variant array that should be validated. The array is
	 * extended by all missing default values. The resulting array is ensured to
	 * be of the correct length and all entries to be of the correct type, even
	 * if validation errors occured (to facilitate graceful degradation).
	 * @param logger is the logger instance to which error messages or warnings
	 * will be written.
	 * @return true if the operation was successful, false if an error occured.
	 */
	bool validateArray(Variant::arrayType &arr, Logger &logger);

	/**
	 * Checks whether the content of the given variant map matches the
	 * argument list stored in this Arguments instance. Any ommited default
	 * arguments are added to the map.
	 *
	 * @param map is the variant map that should be validated. The map is
	 * extended by all missing default values. The resulting map is ensured to
	 * be of the correct length and all entries to be of the correct type, even
	 * if validation errors occured (to facilitate graceful degradation).
	 * @param logger is the logger instance to which error messages or warnings
	 * will be written.
	 * @param ignoreUnknown if set to true, unknown map entries are ignored
	 * (a note is issued). This behaviour can be usefull if forward
	 * compatibility must be achieved (such as for XML based formats).
	 * @return true if the operation was successful, false if an error occured.
	 */
	bool validateMap(Variant::mapType &map, Logger &logger,
	                 bool ignoreUnknown = false);
};
}

#endif /* _OUSIA_ARGUMENT_HPP_ */

