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
 * @file VariantWriter.hpp
 *
 * Contains the VariantWriter class which provides serialization functions for
 * Variant types.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_VARIANT_WRITER_HPP_
#define _OUSIA_VARIANT_WRITER_HPP_

#include <string>
#include <ostream>

namespace ousia {

// Forward declaration
class Variant;

/**
 * Class which provides serialization functions for writing variants to an
 * output stream in various formats.
 */
class VariantWriter {
public:
	/**
	 * Dumps the Variant as JSON data. Note that the resulting JSON data is
	 * invalid if the Variant consists of function or object references.
	 *
	 * @param var is the variant that should be serialized.
	 * @param stream is the stream the result should be written to.
	 * @param pretty if true, the resulting value is properly indented.
	 */
	static void writeJson(const Variant &var, std::ostream &stream,
	                      bool pretty = true);

	/**
	 * Dumps the Variant as JSON data to a string.
	 *
	 * @param var is the variant that should be serialized.
	 * @param pretty if true, the resulting value is properly indented.
	 */
	static std::string writeJsonToString(const Variant &var, bool pretty = true);

};
}

#endif /* _OUSIA_VARIANT_WRITER_HPP_ */

