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
 * @file VariantConverter.hpp
 *
 * Contains the VariantConverter class which contains code commonly used by
 * the Variant, Typesystem and Argument classes to cast Variants to other
 * Variant types.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_VARIANT_CONVERTER_HPP_
#define _OUSIA_VARIANT_CONVERTER_HPP_

namespace ousia {

// Forward declaration
class Logger;
class RttiType;
class Variant;

/**
 * The VariantConverter class is used to convert a variant to a certain
 * prespecified type. The functions ensure that the variant has the requested
 * type, even if the conversion fails.
 */
class VariantConverter {
public:
	/**
	 * Enumeration used to define the mode of conversion -- either only safe
	 * conversions (without any data loss) are performed, or all possible
	 * conversions are tried (with possible data loss).
	 */
	enum class Mode {
		/**
		 * Performs only lossless and sane conversions.
		 */
		SAFE,

		/**
		 * Performs possibly lossy and probably unintuitive conversions.
		 */
		ALL
	};

	/**
	 * Converts the given variant to a boolean. If the "mode" parameter is set
	 * to Mode::SAFE, only booleans can be converted to booleans. For all other
	 * types the conversion fails. If "mode" is set to Mode::ALL, nullptr
	 * values and zero numeric values are treated as "false", all other values
	 * are treated as "true". If the conversion fails, false is returned as
	 * default value.
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool toBool(Variant &var, Logger &logger, Mode mode = Mode::SAFE);

	/**
	 * Converts the given variant to an integer. If the "mode" parameter is set
	 * to Mode::SAFE, only integers can be converted to integers. For all other
	 * types the conversion fails. If "mode" is set to Mode::ALL, booleans are
	 * converted to 0, 1, nullptr is converted to 0, doubles are truncated,
	 * strings are parsed and truncated, arrays with one element are converted
	 * to an integer. Conversion fails for objects, functions, maps and arrays
	 * with zero or more than one entry. If the conversion fails, 0 is returned
	 * as default value.
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool toInt(Variant &var, Logger &logger, Mode mode = Mode::SAFE);

	/**
	 * Converts the given variant to a double. If the "mode" parameter is set
	 * to Mode::SAFE, only integers and doubles can be converted to doubles. For
	 * all other types the conversion fails. If "mode" is set to Mode::ALL,
	 * booleans are converted to 0.0, 1.0, nullptr is converted to 0.0, strings
	 * are parsed, arrays with one element are converted to a double.
	 * Conversion fails for objects, functions, maps and arrays with zero or
	 * more than one entry. If the conversion fails, 0.0 is returned as default
	 * value.
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool toDouble(Variant &var, Logger &logger, Mode mode = Mode::SAFE);

	/**
	 * Converts the given variant to a double. If the "mode" parameter is set
	 * to Mode::SAFE, all primitive types can be converted to strings. For
	 * all other types the conversion fails. If "mode" is set to Mode::ALL,
	 * maps and arrays are converted to a JSON representation, objects and
	 * functions are converted to an informative string containing their pointer
	 * and type. If the conversion fails, an empty string is returned as default
	 * value.
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool toString(Variant &var, Logger &logger, Mode mode = Mode::SAFE);

	/**
	 * Converts the given variant to an array with the given inner type. If the
	 * "mode" parameter is set to Mode::SAFE, the given variant must be an
	 * array, If mode is set to Mode::ALL, other variant values are encapsulated
	 * in array with one entry. In both cases, if "innerType" points at a
	 * primitive Rtti type, conversion to that type is tried (using the
	 * specified conversion mode).
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param innerType is the inner type of the array entries. Should be set to
	 * RttiTypes::None in case the inner type of the array does not matter.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool toArray(Variant &var, const RttiType &innerType, Logger &logger,
	                    Mode mode = Mode::SAFE);


	/**
	 * Converts the given variant to an map with the given inner type. The given
	 * variant must be a map. If "innerType" points at a primitive Rtti type,
	 * conversion to that type is tried with the specified conversion mode.
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param innerType is the inner type of the map entries. Should be set to
	 * RttiTypes::None in case the inner type of the map does not matter.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode used for converting the entries of the
	 * map to the inner type.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool toMap(Variant &var, const RttiType &innerType, Logger &logger,
	                    Mode mode = Mode::SAFE);

	/**
	 * Makes sure the given variant is a function. If it is not, it is replaced
	 * by a dummy function which takes no arguments and returns nullptr.
	 *
	 * @param var is the variant that should be converted to a function.
	 * @param logger is the logger to which error messages should be written.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool toFunction(Variant &var, Logger &logger);

	/**
	 * Tries conversion to the given RttiType with the given optional inner
	 * type.
	 *
	 * @param type describes the type to which the variant should be converted.
	 * This might either be a variant type such as RttiType::Bool,
	 * RttiType::Int, RttiType::Double, RttiType::String, RttiType::Array,
	 * RttiType::Map or RttiType::Function. All other types are regarded as
	 * managed object of this type. If RttiType::None is given, all types are
	 * accepted.
	 * @param innerType is used in case of maps or arrays to check the type of
	 * the elements of these containers. If RttiType::None is given, no special
	 * type is required.
	 * @param logger is a reference at the logger instance to which error
	 * messages are forwarded.
	 * @param mode is the conversion mode that is being enforced.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool convert(Variant &var, const RttiType &type,
	                    const RttiType &innerType, Logger &logger,
	                    Mode mode = Mode::SAFE);

	/**
	 * Tries conversion to the given RttiType without any enforcement regarding
	 * the inner type of container types.
	 *
	 * @param type describes the type to which the variant should be converted.
	 * This might either be a variant type such as RttiType::Bool,
	 * RttiType::Int, RttiType::Double, RttiType::String, RttiType::Array,
	 * RttiType::Map or RttiType::Function. All other types are regarded as
	 * managed object of this type. If RttiType::None is given, all types are
	 * accepted.
	 * @param logger is a reference at the logger instance to which error
	 * messages are forwarded.
	 * @param mode is the conversion mode that is being enforced.
	 * @return true if the operation was successful, false otherwise. In any
	 * case the input/output parameter "var" will have the requested type.
	 */
	static bool convert(Variant &var, const RttiType &type,
	                    Logger &logger, Mode mode = Mode::SAFE);
};
}

#endif /* _OUSIA_VARIANT_CONVERTER_HPP_ */

