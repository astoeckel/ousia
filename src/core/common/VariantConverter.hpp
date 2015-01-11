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

#ifndef _OUSIA_VARIANT_CONVERTER_HPP_
#define _OUSIA_VARIANT_CONVERTER_HPP_

namespace ousia {

// Forward declaration
class Logger;
class RttiType;
class Variant;
enum class VariantType : int16_t;

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
	enum class Mode { SAFE, ALL };

	/**
	 * Converts the given variant to a boolean. If the "mode" parameter is set
	 * to Mode::SAFE, only booleans can be converted to booleans. For all other
	 * types the conversion fails. If "mode" is set to Mode::ALL, nullptr
	 * values and zero numeric values are treated as "false", all other values
	 * are treated as "true".
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 */
	static bool toBool(Variant &var, Logger &logger, Mode mode = Mode::SAFE);

	/**
	 * Converts the given variant to an integer. If the "mode" parameter is set
	 * to Mode::SAFE, only integers can be converted to integers. For all other
	 * types the conversion fails. If "mode" is set to Mode::ALL, booleans are
	 * converted to 0, 1, nullptr is converted to 0, doubles are truncated,
	 * strings are parsed and truncated, arrays with one element are converted
	 * to an integer. Conversion fails for objects, functions, maps and arrays
	 * with zero or more than one entry.
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 */
	static bool toInt(Variant &var, Logger &logger, Mode mode = Mode::SAFE);

	/**
	 * Converts the given variant to a double. If the "mode" parameter is set
	 * to Mode::SAFE, only integers and doubles can be converted to doubles. For
	 * all other types the conversion fails. If "mode" is set to Mode::ALL,
	 * booleans are converted to 0.0, 1.0, nullptr is converted to 0.0, strings
	 * are parsed, arrays with one element are converted to a double.
	 * Conversion fails for objects, functions, maps and arrays with zero or
	 * more than one entry.
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 */
	static bool toDouble(Variant &var, Logger &logger, Mode mode = Mode::SAFE);

	/**
	 * Converts the given variant to a double. If the "mode" parameter is set
	 * to Mode::SAFE, all primitive types can be converted to strings. For
	 * all other types the conversion fails. If "mode" is set to Mode::ALL,
	 * maps and arrays are converted to a JSON representation, objects and
	 * functions are converted to an informative string containing their pointer
	 * and type.
	 *
	 * @param var is instance of the Variant class that should be converted to
	 * the requested type.
	 * @param logger is a reference to the logger instance into which messages
	 * should be logged.
	 * @param mode is the conversion mode. See method description for the exact
	 * effect.
	 */
	static bool toString(Variant &var, Logger &logger, Mode mode = Mode::SAFE);

	static bool convert(Variant &var, VariantType requestedType,
	                    const RttiType &rttiType, Logger &logger,
	                    Mode mode = Mode::SAFE);

	static bool convert(Variant &var, VariantType requestedType,
	                    Logger &logger, Mode mode = Mode::SAFE);
};
}

#endif /* _OUSIA_VARIANT_CONVERTER_HPP_ */

