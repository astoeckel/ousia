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

#ifndef _OUSIA_FILE_LOCATOR_HPP_
#define _OUSIA_FILE_LOCATOR_HPP_

#include <core/resource/Resource.hpp>
#include <core/resource/ResourceLocator.hpp>

#include <map>
#include <set>
#include <vector>

namespace ousia {

/**
 * A ResourceLocator is a class able to locate resources in some way, usually
 * on the hard drive.
 *
 * We specify this as an abstract superclass to have an interface layer between
 * the program core and possible future extensions in terms of resource
 * locations (e.g. online resources, .zip files, etc.).
 */
class FileLocator : public ResourceLocator {
public:
	/**
	 * Type alias representing a Res
	 */
	using SearchPaths = std::map<ResourceType, std::vector<std::string>>;

private:
	/**
	 * Internal variable containing all stored search paths.
	 */
	SearchPaths searchPaths;

protected:
	bool doLocate(Resource &resource, const std::string &path,
	              const ResourceType type,
	              const std::string &relativeTo) const override;

	std::unique_ptr<std::istream> doStream(
	    const std::string &location) const override;

public:
	FileLocator() : searchPaths() {}

	/**
	 * Adds a search paths for the given types.
	 *
	 * @param path is a fully qualified/canonical path to a directory.
	 * @param types is a set of Resource Types. The FileLocator will look for
	 *              resources of the specified types at the given path in the
	 *              future.
	 */
	void addSearchPath(const std::string &path, std::set<ResourceType> types);

	/**
	 * Adds a search path. Implicitly adds the search path for the "unknown"
	 *
	 * @param path is a fully qualified/canonical path to a directory.
	 * @param types is a set of Resource Types. The FileLocator will look for
	 *              resources of the specified types at the given path in the
	 *              future.
	 */
	void addSearchPath(const std::string &path);

	/**
	 * Returns the backing map containing all search paths for a given type.
	 * This is read-only.
	 */
	const SearchPaths &getSearchPaths() const { return searchPaths; }
};
}

#endif /* _OUSIA_FILE_LOCATOR_HPP_ */

