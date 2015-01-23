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
 * @file Registry.hpp
 *
 * Class used for registering plugin classes. The Registry is one of the central
 * classes needed for parsing and transforming an Ousía document.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_REGISTRY_HPP_
#define _OUSIA_REGISTRY_HPP_

#include <map>
#include <set>
#include <vector>

#include <core/common/Rtti.hpp>
#include <core/resource/Resource.hpp>

namespace ousia {

// Forward declarations
class Parser;
class ResourceLocator;

/**
 * The Registry class is the central class which is used to store references to
 * all available plugins.
 */
class Registry {
private:
	/**
	 * Mapping between parser mimetypes and pairs of parsers and their supported
	 * Rtti types.
	 */
	std::map<std::string, std::pair<Parser *, RttiSet>> parsers;

	/**
	 * Map from file extensions to registered mimetypes.
	 */
	std::map<std::string, std::string> extensions;

	/**
	 * List containing all registered ResourceLocator instances.
	 */
	std::vector<ResourceLocator *> locators;

public:
	/**
	 * Registers a new parser instance for the given set of mimetypes. Throws
	 * an exception if a parser is already registered for one of the mimetypes.
	 *
	 * @param mimetypes is a set of mimetypes for which the Parser should be
	 * registered.
	 * @param types is a set of node the parser is known to return. This
	 * information is needed in order to prevent inclusion of the wrong Node
	 * types
	 * @param parser is the parser instance that is registered for the given
	 * mimetypes.
	 */
	void registerParser(const std::set<std::string> &mimetypes,
	                    const RttiSet &types, Parser *parser);

	/**
	 * Returns a pointer pointing at a Parser that was registered for handling
	 * the given mimetype.
	 *
	 * @param mimetype is the mimetype for which a Parser instance should be
	 * looked up.
	 * @return a pair containing a pointer at the parser and the RttiTypes
	 * supported by the parser. The pointer is set to a nullptr if no such
	 * parser could be found.
	 */
	const std::pair<Parser *, RttiSet> &getParserForMimetype(
	    const std::string &mimetype) const;

	/**
	 * Registers a file extension with a certain mimetype. Throws an exception
	 * if a mimetype is already registered for this extension.
	 *
	 * @param extension is the file extension for which the mimetype should be
	 * registered. The extension has to be provided without a leading dot. The
	 * extensions are handled case insensitive.
	 * @param mimetype is the mimetype that should be registered for the
	 * extension.
	 */
	void registerExtension(const std::string &extension,
	                       const std::string &mimetype);

	/**
	 * Registers mimetypes for some default extensions.
	 */
	void registerDefaultExtensions();

	/**
	 * Returns the mimetype for the given extension.
	 *
	 * @param extension is the file extension for which the mimetype should be
	 * looked up. The extension has to be provided without a leading dot. The
	 * extensions are handled case insensitive.
	 * @return the registered mimetype or an empty string of none was found.
	 */
	std::string getMimetypeForExtension(const std::string &extension) const;

	/**
	 * Tries to deduce the mimetype from the given filename.
	 *
	 * @param filename is the filename from which the mimetype should be
	 * deduced.
	 * @return the mimetype or an empty string if no filename could be deduced.
	 */
	std::string getMimetypeForFilename(const std::string &filename) const;

	/**
	 * Registers a ResourceLocator instance that should be used for locating
	 * resources. Two registered ResourceLocator should not be capable of
	 * accessing Resources at the same location. If this happens, the resource
	 * locator that was registered first has precedence.
	 *
	 * @param locator is the ResourceLocator instance that should be registered.
	 */
	void registerResourceLocator(ResourceLocator *locator);

	/**
	 * Locates a resource using the registered ResourceLocator instances.
	 *
	 * @param resource is the resource descriptor to which the result will be
	 * written.
	 * @param path is the path under which the resource should be looked up.
	 * @param type is the ResourceType. Specifying a resource type may help to
	 * locate the resource.
	 * @param relativeTo is another resource relative to which the resource may
	 * be looked up.
	 */
	bool locateResource(Resource &resource, const std::string &path,
	                    ResourceType type = ResourceType::UNKNOWN,
	                    const Resource &relativeTo = NullResource) const;
};
}

#endif /* _OUSIA_REGISTRY_HPP_ */

