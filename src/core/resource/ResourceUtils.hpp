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
 * @file ResourceUtils.hpp
 *
 * Contains the ResourceUtils class which defines a set of static utility
 * functions for dealing with Resources and ResourceTypes.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_RESOURCE_UTILS_HPP_
#define _OUSIA_RESOURCE_UTILS_HPP_

#include <string>

#include "Resource.hpp"

namespace ousia {

// Forward declarations
class Rtti;
class RttiSet;

/**
 * Class containing static utility functions for dealing with Resources and
 * ResourceTypes.
 */
class ResourceUtils {
public:
	/**
	 * Function used to deduce the resource type from a given "relation" string
	 * and a set of RTTI types into which the resource should be converted by a
	 * parser.
	 *
	 * @param rel is a relation string which specifies the type of the resource.
	 * May be empty.
	 * @param supportedTypes is a set of RTTI types into which the resource
	 * should be converted by a parser. Set may be empty.
	 * @param logger is the Logger instance to which errors should be logged.
	 * @return a ResourceType specifier.
	 */
	static ResourceType deduceResourceType(const std::string &rel,
	                                       const RttiSet &supportedTypes,
	                                       Logger &logger);

	/**
	 * Function used to deduce the resource type from a given "relation" string.
	 *
	 * @param rel is a relation string which specifies the type of the resource.
	 * May be empty.
	 * @param logger is the Logger instance to which errors should be logged
	 * (e.g. if the relation string is invalid).
	 * @return a ResourceType specifier.
	 */
	static ResourceType deduceResourceType(const std::string &rel,
	                                       Logger &logger);

	/**
	 * Function used to deduce the resource type from a set of RTTI types into
	 * which the resource should be converted by a parser.
	 *
	 * @param supportedTypes is a set of RTTI types into which the resource
	 * should be converted by a parser. Set may be empty.
	 * @param logger is the Logger instance to which errors should be logged.
	 * @return a ResourceType specifier.
	 */
	static ResourceType deduceResourceType(const RttiSet &supportedTypes,
	                                       Logger &logger);

	/**
	 * Transforms the given relation string to the corresponding RttiType.
	 *
	 * @param rel is a relation string which specifies the type of the resource.
	 * May be empty.
	 * @return a pointer at the corresponding Rtti instance or a pointer at the
	 * Rtti descriptor of the Node class (the most general Node type) if the
	 * given relation type is unknown.
	 */
	static const Rtti *deduceRttiType(const std::string &rel);

	/**
	 * Reduces the number of types supported by a parser as the type of a
	 * resource to the intersection of the given supported types and the RTTI
	 * type associated with the given relation string.
	 *
	 * @param supportedTypes is a set of RTTI types into which the resource
	 * should be converted by a parser. Set may be empty.
	 * @param rel is a relation string which specifies the type of the resource.
	 * @return the supported type set limited to those types that can actually
	 * be returned according to the given relation string.
	 */
	static RttiSet limitRttiTypes(const RttiSet &supportedTypes,
	                              const std::string &rel);

	/**
	 * Reduces the number of types supported by a parser as the type of a
	 * resource to the intersection of the given supported types and the RTTI
	 * type associated with the given relation string.
	 *
	 * @param supportedTypes is a set of RTTI types into which the resource
	 * should be converted by a parser. Set may be empty.
	 * @param type is the type that is to be expected from the parser.
	 * @return the supported type set limited to those types that can actually
	 * be returned according to the given relation string (form an isa
	 * relationship with the given type).
	 */
	static RttiSet limitRttiTypes(const RttiSet &supportedTypes,
	                              const Rtti *type);
};
}

#endif /* _OUSIA_RESOURCE_UTILS_HPP_ */

