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
 * @file ResourceManager.hpp
 *
 * Defines the ResourceManager class which is responsible for keeping track of
 * already included resources and to retrieve CharReader instance for not-yet
 * parsed resources.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_RESOURCE_MANAGER_HPP_
#define _OUSIA_RESOURCE_MANAGER_HPP_

#include <string>
#include <unordered_map>

#include <core/common/Location.hpp>
#include <core/managed/Managed.hpp>

namespace ousia {

// Forward declarations
class Node;
class ParserContext;
class Resource;
class RttiSet;
extern const Resource NullResource;

/**
 * The ResourceManager class is responsible for keepking track of all included
 * resources. It retrieves CharReader instances for not-yet parsed resources
 * and returns references for those resources that already have been parsed.
 */
class ResourceManager {
private:
	/**
	 * Vector used for mapping SourceId instances to the underlying resource.
	 */
	std::vector<Resource> resources;

	/**
	 * Index pointing at the next free entry in the resources list.
	 */
	SourceId nextFreeSourceId = 0;

	/**
	 * Map between Resource locations and the corresponding SourceId.
	 */
	std::unordered_map<std::string, SourceId> locations;

	/**
	 * Map between a SourceId and the corresponding (if available) parsed node
	 * uid (this resembles weak references to the Node instance).
	 */
	std::unordered_map<SourceId, ManagedUid> nodes;

	/**
	 * Cache used for translating byte offsets to line numbers. Maps from a
	 * SourceId onto a list of (sorted) SourceOffsets. The index in the list
	 * corresponds to the line number.
	 */
	std::unordered_map<SourceId, std::vector<SourceOffset>> lineNumberCache;


	Rooted<Node> getCachedNode(SourceId id);

	Rooted<Node> getCachedNode(const std::string &location);

	SourceId getSourceId(const std::string &location);

	/**
	 * Used internally to either parse a resource or retrieve it from the
	 * internal cache of already parsed resources.
	 */
	Rooted<Node> link(ParserContext &ctx, Resource &resource, const std::string &mimetype,
	                     const RttiSet &supportedTypes);


public:
	/**
	 * Resolves the reference to the file specified by the given path and -- if
	 * this has not already happened -- parses the file. Logs any problem in
	 * the logger instance of the given ParserContext.
	 *
	 * @param ctx is the context from the Registry and the Logger instance will
	 * be looked up.
	 * @param path is the path to the file that should be included.
	 * @param mimetype is the mimetype the file was included with. If no
	 * mimetype is given, the path must have an extension that is known by
	 */
	Rooted<Node> link(ParserContext &ctx, const std::string &path,
	                     const std::string &mimetype = "",
	                     const std::string &rel = "",
	                     const RttiSet &supportedTypes = RttiSet{},
	                     const Resource &relativeTo = NullResource);

	/**
	 * Resolves the reference to the file specified by the given path and -- if
	 * this has not already happened -- parses the file. Logs any problem in
	 * the logger instance of the given ParserContext.
	 */
	Rooted<Node> link(ParserContext &ctx, const std::string &path,
	                     const std::string &mimetype, const std::string &rel,
	                     const RttiSet &supportedTypes, SourceId relativeTo);

	/**
	 * Returns a Resource instance for the given SourceId.
	 *
	 * @param sourceId is the id of the Resource instance that should be
	 * returned.
	 * @return the Resource instance corresponding to the given sourceId. If the
	 * sourceId is invalid, the returned Resource will be invalid (a reference
	 * at NullResource).
	 */
	const Resource &getResource(SourceId sourceId) const;

	/**
	 * Creates and returns a SourceContext structure containing information
	 * about the given SourceLocation (such as line and column number). Throws
	 * a LoggableException if an irrecoverable error occurs while looking up the
	 * context (such as a no longer existing resource).
	 *
	 * @param location is the SourceLocation for which context information
	 * should be retrieved. This method is used by the Logger class to print
	 * pretty messages.
	 * @return a valid SourceContext if a valid SourceLocation was given or an
	 * invalid SourceContext if the location is invalid.
	 */
	SourceContext buildContext(const SourceLocation &location);
};
}

#endif /* _OUSIA_RESOURCE_MANAGER_HPP_ */

