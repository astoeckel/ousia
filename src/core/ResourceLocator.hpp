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
	enum class ResourceType {
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
	 * A ResourceLocation contains the location of a Resource, e.g. a file path
	 * on a hard drive. Note that the 'found' flag might be set to false
	 * indicating that a resource was not found.
	 */
	struct ResourceLocation {
		const bool found;
		const ResourceLocator &locator;
		const ResourceType type;
		const std::string location;

		ResourceLocation(const bool found, const ResourceLocator &locator,
		                 const ResourceType type, const std::string location)
		    : found(found), locator(locator), type(type), location(location)
		{
		}

		/**
		 * This calls the 'stream' method of the underlying ResourceLocator that
		 * found this location and returns a stream containing the data of the
		 * Resource at this location.
		 *
		 * @param stream is an inputstream that gets the data of the Resource at
		 *               this location.
		 */
		void stream(std::istream &input) const
		{
			return locator.stream(location, input);
		}
	};

	/**
	 * The locate function uses this ResourceLocator to search for a given
	 * Resource name (path parameter). It returns a ResourceLocation with the
	 * 'found' flag set accordingly.
	 *
	 * @param path is the resource name.
	 * @param relativeTo TODO: What is the meaning of this parameter?
	 * @param type is the type of this resource.
	 *
	 * @return A ResourceLocation containing either the found location of the
	 *         Resource and the found flag set to 'true' or an empty location
	 *         and the found flag set to 'false'.
	 */
	virtual ResourceLocation locate(const std::string &path,
	                                const std::string &relativeTo,
	                                const ResourceType type) const;

	/**
	 * This method returns a strem containing the data of the resource at the
	 * given location.
	 *
	 * @param location is a found location, most likely from a ResourceLocation.
	 * @param stream is an inputstream that gets the data of the Resource at
	 *               this location.
	 */
	virtual void stream(const std::string &location,
	                     std::istream &input) const;
};
}

#endif /* _RESOURCE_LOCATOR_HPP_ */

