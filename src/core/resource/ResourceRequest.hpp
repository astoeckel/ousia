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
 * @file ResourceRequest.hpp
 *
 * Defines the ResourceRequest class used by the ResourceManager to deduce as
 * much information as possible about a resource that was included by the user.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_RESOURCE_REQUEST_HPP_
#define _OUSIA_RESOURCE_REQUEST_HPP_

#include <string>

#include <core/common/Rtti.hpp>
#include <core/resource/Resource.hpp>

namespace ousia {

// Forward declarations
class Logger;
class Parser;
class Registry;

/**
 * The ResourceRequest class contains user provided data about a Resource that
 * should be opened and parsed. The ResourceRequest class can then be used to
 * deduce missing information about the resource and finally to locate the
 * Resource in the filesystem and to find a parser that is capable of parsing
 * the Resource.
 */
class ResourceRequest {
private:
	/**
	 * Requested path of the file that should be included.
	 */
	std::string path;

	/**
	 * Mimetype of the resource that should be parsed.
	 */
	std::string mimetype;

	/**
	 * Relation string specifing the relationship of the resource within the
	 * document it is included in.
	 */
	std::string rel;

	/**
	 * Specifies the types of the Node that may result from the resource once it
	 * has been parsed.
	 */
	RttiSet supportedTypes;

	/**
	 * Types the parser is expected to return.
	 */
	RttiSet parserTypes;

	/**
	 * The resource relative to which this resource is to be located.
	 */
	Resource relativeTo;

	/**
	 * ResourceType as deduced from the user provided values.
	 */
	ResourceType resourceType;

	/**
	 * Pointer at the Parser instance that may be used to parse the resource.
	 */
	Parser *parser;

public:
	/**
	 * Constructor of the ResourceRequest class. Takes the user provided data
	 * about the resource request.
	 *
	 * @param path is the requested path of the file that should be included.
	 * @param mimetype is the mimetype of the resource that should be parsed
	 * (may be empty, in which case the mimetype is deduced from the file
	 * extension)
	 * @param rel is a "relation string" supplied by the user which specifies
	 * the relationship of the specified resource within the document it is
	 * included in.
	 * @param supportedTypes specifies the types of the Node that may result
	 * from the resource once it has been parsed. This value is not directly
	 * provided by the user, but by the calling code.
	 * @param relativeTo is another resource relative to which the Resource
	 * should be looked up.
	 */
	ResourceRequest(const std::string &path, const std::string &mimetype,
	                const std::string &rel, const RttiSet &supportedTypes,
	                const Resource &relativeTo = NullResource);

	/**
	 * Tries to deduce all possible information and produces log messages for
	 * the user.
	 *
	 * @param registry from which registered parsers, mimetypes and file
	 * extensions are looked up.
	 * @param logger is the logger instance to which errors or warnings are
	 * logged.
	 * @return true if a parser has been found that could potentially be used to
	 * parse the file.
	 */
	bool deduce(Registry &registry, Logger &logger);

	/**
	 * Tries to locate the specified resource.
	 *
	 * @param registry from which registered parsers, mimetypes and file
	 * extensions are looked up.
	 * @param logger is the logger instance to which errors or warnings are
	 * logged.
	 * @param resource is the Resource descriptor that should be filled with the
	 * actual location.
	 * @return true if a resource was found, false otherwise. Equivalent to
	 * the value of resource.isValid().
	 */
	bool locate(Registry &registry, Logger &logger, Resource &resource) const;

	/**
	 * Returns the requested path of the file that should be included.
	 *
	 * @param path given by the user (not the location of an actually found
	 * resource).
	 */
	const std::string &getPath() const { return path; }

	/**
	 * Returns the mimetype of the resource that should be parsed.
	 *
	 * @return the deduced mimetype.
	 */
	const std::string &getMimetype() const { return mimetype; }

	/**
	 * Returns the relation string which specifies the relationship of the
	 * resource within the document it is included in.
	 *
	 * @return the deduced relation string.
	 */
	const std::string &getRel() const { return rel; }

	/**
	 * Returns the types of the Node that may result from the resource once it
	 * has been parsed. Restricted to the types that may actually returned by
	 * the parser.
	 *
	 * @return the deduced supported types.
	 */
	const RttiSet &getSupportedTypes() const { return supportedTypes; }

	/**
	 * Returns the ResourceType as deduced from the user provided values.
	 *
	 * @return deduced ResourceType or ResourceType::UNKNOWN if none could be
	 * deduced.
	 */
	ResourceType getResourceType() const { return resourceType; }

	/**
	 * Returns the parser that was deduced according to the given resource
	 * descriptors.
	 *
	 * @return the pointer at the parser instance or nullptr if none was found.
	 */
	Parser *getParser() const { return parser; }

	/**
	 * Returns the types the parser may return or an empty set if no parser was
	 * found.
	 *
	 * @return the types the parser may return.
	 */
	RttiSet getParserTypes() const { return parserTypes; }
};
}

#endif /* _OUSIA_RESOURCE_REQUEST_HPP_ */

