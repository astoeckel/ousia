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

#ifndef _OUSIA_RESOURCE_LOCATOR_HPP_
#define _OUSIA_RESOURCE_LOCATOR_HPP_

#include <istream>
#include <memory>

namespace ousia {

/**
 * A ResourceLocator is a class able to locate resources in some way, usually
 * on the hard drive.
 *
 * We specify this as an abstract superclass to have an interface layer between
 * the program core and possible future extensions in terms of resource
 * locations (e.g. online resources, .zip files, etc.).
 */
class ResourceLocator {
public:
	/**
	 * This enum contains all possible types of includable resources in Ousía.
	 */
	enum class Type {
		// A Domain description
		DOMAIN,
		// An ECMA/JavaScript
		SCRIPT,
		// A Type System
		TYPESYSTEM,
		// TODO: Aren't documents and attribute descriptors missing?
		// TODO: What is the purpose of these two?
		GENERIC_MODULE,
		GENERIC_INCLUDE
	};

	/**
	 * A Location contains the location of a Resource, e.g. a file path
	 * on a hard drive. Note that the 'found' flag might be set to false
	 * indicating that a resource was not found.
	 */
	struct Location {
		const bool found;
		const ResourceLocator &locator;
		const Type type;
		/**
		 * This is a fully qualified/canonical path to the resource found or
		 * in an undefined state (possibly empty) if the 'found' flag is set
		 * to 'false'.
		 */
		const std::string location;

		Location(const bool found, const ResourceLocator &locator,
		         const Type type, const std::string location)
		    : found(found), locator(locator), type(type), location(location)
		{
		}

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
		std::unique_ptr<std::istream> stream() const
		{
			return std::move(locator.stream(location));
		}
	};

	/**
	 * The locate function uses this ResourceLocator to search for a given
	 * Resource name (path parameter). It returns a Location with the
	 * 'found' flag set accordingly.
	 *
	 * @param path is the resource name.
	 * @param relativeTo is an already resolved fully qualified name/canonical
	 *                   path that is to be used as base directory for this
	 *                   search.
	 * @param type is the type of this resource.
	 *
	 * @return A Location containing either the found location of the
	 *         Resource and the found flag set to 'true' or an empty location
	 *         and the found flag set to 'false'.
	 */
	virtual Location locate(const std::string &path,
	                        const std::string &relativeTo,
	                        const Type type) const = 0;

	/**
	 * This method returns a strem containing the data of the resource at the
	 * given location.
	 *
	 * @param location is a found location, most likely from a Location.
	 *
	 * @return a stream containing the data of the Resource at this
	 *         location. This has to be a unique_pointer because the current
	 *         C++11 compiler does not yet support move semantics for
	 *         streams.
	 */
	virtual std::unique_ptr<std::istream> stream(
	    const std::string &location) const = 0;
};
}

#endif /* _RESOURCE_LOCATOR_HPP_ */

