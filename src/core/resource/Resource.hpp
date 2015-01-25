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
 * @file Resource.hpp
 *
 * Contains the Resource class, representing an external resource as well as
 * further types used for describing resources.
 *
 * @author Benjamin Paaßen (bpassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_RESOURCE_HPP_
#define _OUSIA_RESOURCE_HPP_

#include <map>
#include <memory>
#include <ostream>
#include <string>

namespace ousia {

// Forward declaration
class ResourceLocator;

/**
 * This enum contains all possible types of includable resources in Ousía.
 */
enum class ResourceType {
	/**
     * Unknown type.
     */
	UNKNOWN,

	/**
     * The resource contains a domain description.
     */
	DOMAIN_DESC,

	/**
     * The resource contains a typesystem description.
     */
	TYPESYSTEM,

	/**
     * The resource contains a simple document.
     */
	DOCUMENT,

	/**
     * The resource contains style attributes.
     */
	ATTRIBUTES,

	/**
     * The resource is a stylesheet.
     */
	STYLESHEET,

	/**
     * The resource contains script (note that the actual scripting language is
     * not specified by the resource type).
     */
	SCRIPT,

	/**
     * Generic data, such as e.g. images.
     */
	DATA
};

/**
 * A Location contains the location of a Resource, e.g. a file path
 * on a hard drive. Note that the 'found' flag might be set to false
 * indicating that a resource was not found.
 */
class Resource {
private:
	/**
	 * Specifies whether the resource points at a valid location or not.
	 */
	bool valid;

	/**
	 * Reference pointing at the location
	 */
	ResourceLocator const *locator;

	/**
	 * Requested type of the resource.
	 */
	ResourceType type;

	/**
	 * This is a fully qualified/canonical path to the resource found or
	 * in an undefined state (possibly empty) if the 'valid' flag is set
	 * to 'false'.
	 */
	std::string location;

public:
	/**
	 * Default constructor of the Resource class, represents an invalid
	 * resource.
	 */
	Resource();

	/**
	 * Constructor of the Resource class.
	 *
	 * @param valid specifies whether the Resource is valid or not.
	 * @param locator specifies the resource locator that was used to locate the
	 * resource.
	 */
	Resource(bool valid, const ResourceLocator &locator,
	         ResourceType type, const std::string &location);

	/**
	 * This calls the 'stream' method of the underlying ResourceLocator that
	 * found this location and returns a stream containing the data of the
	 * Resource at this location.
	 *
	 * @return a stream containing the data of the Resource at this
	 *         location. This has to be a unique_pointer because the current
	 *         C++11 compiler does not yet support move semantics for
	 *         streams.
	 */
	std::unique_ptr<std::istream> stream() const;

	/**
	 * Returns whether this resource is valid or not.
	 *
	 * @return true if the resource is valid, false otherwise.
	 */
	bool isValid() const { return valid; }

	/**
	 * Returns a reference pointing at the locator that was used to locate this
	 * resource.
	 *
	 * @return the locator used for locating this resource.
	 */
	const ResourceLocator &getLocator() const { return *locator; }

	/**
	 * Returns the type of the resource that was requested when the resource was
	 * located.
	 *
	 * @return the type of the resource.
	 */
	ResourceType getType() const { return type; }

	/**
	 * Returns a canonical location that can be used in a hash map to identify
	 * a resource.
	 */
	const std::string &getLocation() const { return location; }

	/**
	 * Returns the name of the given resource type.
	 *
	 * @param resourceType is the ResourceType of which the human readable name
	 * should be returned.
	 * @return the human readable name of the ResourceType.
	 */
	static std::string getResourceTypeName(ResourceType resourceType);

	/**
	 * Returns a resourceType by its name or ResourceType::UNKNOWN if the name
	 * is invalid.
	 *
	 * @param name is the name of the resource type. The name is converted to
	 * lowercase.
	 */
	static ResourceType getResourceTypeByName(const std::string &name);
};

/**
 * Operator used for streaming the name of ResourceType instances.
 *
 * @param os is the output stream.
 * @param resourceType is the type that should be serialized.
 * @return the output stream.
 */
std::ostream& operator<<(std::ostream &os, ResourceType resourceType);

/**
 * Invalid resource instance.
 */
extern const Resource NullResource;
}

#endif /* _OUSIA_RESOURCE_HPP_ */

