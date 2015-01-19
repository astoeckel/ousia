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

#include "Resource.hpp"

namespace ousia {

// Forward declaration
class ResourceLocator;

/**
 * A ResourceLocator is a class able to locate resources in some way, usually
 * on the hard drive.
 *
 * We specify this as an abstract superclass to have an interface layer between
 * the program core and possible future extensions in terms of resource
 * locations (e.g. online resources, .zip files, etc.).
 */
class ResourceLocator {
protected:
	/**
	 * The locate function uses this ResourceLocator to search for a given
	 * Resource name (path parameter). It returns a Location with the
	 * 'valid' flag set accordingly.
	 *
	 * @param resource reference to a Resource the will be set the the found
	 * location. The content of the variable will be only valid if the return
	 * value of this function is set to true.
	 * @param path is the resource name.
	 * @param relativeTo is an already resolved fully qualified name/canonical
	 * path that is to be used as base directory for this search. It is
	 * guaranteed that the path was produced by this locator instance. Otherwise
	 * this argument is set to an empty string.
	 * @param type is the type of this resource.
	 * @return true if a resource could be found, false otherwise.
	 */
	virtual bool doLocate(Resource &resource, const std::string &path,
	                      const ResourceType type,
	                      const std::string &relativeTo) const = 0;

	/**
	 * This method returns a stream containing the data of the resource at the
	 * given location.
	 *
	 * @param location is a found location, most likely from a Location.
	 *
	 * @return a stream containing the data of the Resource at this
	 *         location. This has to be a unique_pointer because the current
	 *         C++11 compiler does not yet support move semantics for
	 *         streams.
	 */
	virtual std::unique_ptr<std::istream> doStream(
	    const std::string &location) const = 0;

public:
	/**
	 * Virtual destructor of the ResourceLocator interface.
	 */
	virtual ~ResourceLocator() {}

	/**
	 * The locate function uses this ResourceLocator to search for a given
	 * Resource name (path parameter).
	 *
	 * @param resource reference to a Resource the will be set the the found
	 * location. The content of the variable will be only valid if the return
	 * value of this function is set to true.
	 * @param path is the resource name.
	 * @param relativeTo is an already resolved Resource.
	 * @param type is the type of this resource.
	 * @return true if a resource could be found, false otherwise.
	 */
	bool locate(Resource &resource, const std::string &path,
	            const ResourceType type = ResourceType::UNKNOWN,
	            const Resource &relativeTo = NullResource) const;

	/**
	 * The locate function uses this ResourceLocator to search for a given
	 * Resource name (path parameter).
	 *
	 * @param resource reference to a Resource the will be set the the found
	 * location. The content of the variable will be only valid if the return
	 * value of this function is set to true.
	 * @param path is the resource name.
	 * @param type is the type of this resource.
	 * @param relativeTo is the location of an already resolved resource
	 * relative to which this resource should be located.
	 * @return true if a resource could be found, false otherwise.
	 */
	bool locate(Resource &resource, const std::string &path,
	            const ResourceType type,
	            const std::string &relativeTo) const;

	/**
	 * This method returns a stream containing the data of the resource at the
	 * given location.
	 *
	 * @param location is a found location, most likely from a Location.
	 *
	 * @return a stream containing the data of the Resource at this
	 *         location. This has to be a unique_pointer because the current
	 *         C++11 compiler does not yet support move semantics for
	 *         streams.
	 */
	std::unique_ptr<std::istream> stream(const std::string &location) const;	
};

/**
 * The StaticResourceLocator class stores a set of predefined resources in
 * memory and allows to return these.
 */
class StaticResourceLocator : public ResourceLocator {
private:
	/**
	 * Map containing the paths and the corresponding stored data.
	 */
	std::map<std::string, std::string> resources;

protected:
	/**
	 * Always returns false.
	 */
	bool doLocate(Resource &resource, const std::string &path,
	              const ResourceType type,
	              const std::string &relativeTo) const override;

	/**
	 * Returns an input stream containing an empty string.
	 */
	std::unique_ptr<std::istream> doStream(
	    const std::string &location) const override;

public:
	/**
	 * Stores static (string) data for the given path.
	 *
	 * @param path is the path the resource should be stored with.
	 * @param data is the data that should be stored for that path.
	 */
	void store(const std::string &path, const std::string &data);
};

/**
 * Implementation of the NullResourceLocator - contains a default implementation
 * of the ResourceLocator class that does nothing. This class is e.g. used in
 * the default constructor of the resource class. A instance of the class is
 * provided as "NullResourceLocator".
 */
class NullResourceLocatorImpl : public ResourceLocator {
protected:
	/**
	 * Always returns false.
	 */
	bool doLocate(Resource &resource, const std::string &path,
	              const ResourceType type,
	              const std::string &relativeTo) const override;

	/**
	 * Returns an input stream containing an empty string.
	 */
	std::unique_ptr<std::istream> doStream(
	    const std::string &location) const override;
};

/**
 * The NullResourceLocator is used as a fallback for invalid Resources.
 */
extern const NullResourceLocatorImpl NullResourceLocator;
}

#endif /* _RESOURCE_LOCATOR_HPP_ */

