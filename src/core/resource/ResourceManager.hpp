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
#include <core/common/Rtti.hpp>
#include <core/common/SourceContextReader.hpp>
#include <core/model/Node.hpp>

#include "Resource.hpp"

namespace ousia {

// Forward declarations
class Parser;
class ParserContext;
class ResourceRequest;
extern const Resource NullResource;

/**
 * The ResourceManager class is responsible for keepking track of all included
 * resources. It retrieves CharReader instances for not-yet parsed resources
 * and returns references for those resources that already have been parsed.
 */
class ResourceManager {
public:
	/**
	 * Used internally to set the mode of the Parse function.
	 */
	enum class ParseMode { IMPORT, INCLUDE };

private:
	/**
	 * Next SourceId to be used.
	 */
	SourceId nextSourceId = 0;

	/**
	 * Map between Resource locations and the corresponding SourceId.
	 */
	std::unordered_map<std::string, SourceId> locations;

	/**
	 * Map used for mapping SourceId instances to the underlying resource.
	 */
	std::unordered_map<SourceId, Resource> resources;

	/**
	 * Map between a SourceId and the corresponding (if available) parsed node
	 * uid (this resembles weak references to the Node instance).
	 */
	std::unordered_map<SourceId, ManagedUid> nodes;

	/**
	 * The set of SourceIds for which resources are currently being parsed.
	 */
	std::unordered_set<SourceId> currentlyParsing;

	/**
	 * Map containing SourceContextReader instances which are -- as their name
	 * suggests -- used to produce SourceContext structures describing the
	 * source code at a given SourceLocation.
	 */
	std::unordered_map<SourceId, SourceContextReader> contextReaders;

	/**
	 * Allocates a new SourceId for the given resource.
	 *
	 * @param resource is the Resource that should be associated with the newly
	 * allocated SourceId.
	 * @return a new SourceId describing the given resource.
	 */
	SourceId allocateSourceId(const Resource &resource);

	/**
	 * Registers the parsed node for this node id.
	 *
	 * @param sourceId is SourceId instance of the resource.
	 * @param node is the node that was parsed from that resource.
	 */
	void storeNode(SourceId sourceId, Handle<Node> node);

	/**
	 * Removes a resource from the internal stores.
	 *
	 * @param sourceId is the id of the file that should be removed.
	 */
	void purgeResource(SourceId sourceId);

	/**
	 * Used internally to parse the given resource. Can either operate in the
	 * "import" or the "include" mode. In the latter case the ParserScope
	 * instance inside the ParserContext is exchanged with an empty one.
	 *
	 * @param ctx is the context that should be passed to the parser.
	 * @param mimetype is the mimetype of the resource that should be parsed
	 * (may be empty, in which case the mimetype is deduced from the file
	 * extension)
	 * @param rel is a "relation string" supplied by the user which specifies
	 * the relationship of the specified resource. May be empty, in which case
	 * the relation is deduced from the supported types and the types of the
	 * parser for the given mimetype.
	 * @param supportedTypes contains the types of the returned Node the caller
	 * can deal with. Note that only the types the parser claims to return are
	 * checked, not the actual result.
	 * @param mode describes whether the file should be included or imported.
	 * @return the parsed nodes or an empty list if something went wrong.
	 */
	ManagedVector<Node> parse(ParserContext &ctx, const std::string &path,
	                       const std::string &mimetype, const std::string &rel,
	                       const RttiSet &supportedTypes, ParseMode mode);

public:
	/**
	 * Resolves the reference to the file specified by the given path and -- if
	 * this has not already happened -- parses the file. The parser that is
	 * called is provided a new ParserContext instance with an empty ParserScope
	 * which allows the Node instance returned by the parser to be cached. Logs
	 * any problem in the logger instance of the given ParserContext.
	 *
	 * @param ctx is the context from which the Logger instance will be looked
	 * up. The sourceId specified in the context instance will be used as
	 * relative location for looking up the new resource. The registry specified
	 * in the ParserContext is used to lookup the parser instances and to
	 * locate the resources.
	 * @param mimetype is the mimetype of the resource that should be parsed
	 * (may be empty, in which case the mimetype is deduced from the file
	 * extension)
	 * @param rel is a "relation string" supplied by the user which specifies
	 * the relationship of the specified resource. May be empty, in which case
	 * the relation is deduced from the supported types and the types of the
	 * parser for the given mimetype.
	 * @param supportedTypes contains the types of the returned Node the caller
	 * can deal with. Note that only the types the parser claims to return are
	 * checked, not the actual result.
	 * @return the parsed node or nullptr if something went wrong.
	 */
	Rooted<Node> import(ParserContext &ctx, const std::string &path,
	                    const std::string &mimetype, const std::string &rel,
	                    const RttiSet &supportedTypes);

	/**
	 * Resolves the reference to the file specified by the given path and parses
	 * the file using the provided context. As the result of the "include"
	 * function depends on the ParserScope inside the provided ParserContext
	 * instance, the resource has to be parsed every time this function is
	 * called. This contasts the behaviour of the "import" function, which
	 * creates a new ParserScope and thus guarantees reproducible results. Logs
	 * any problem in the logger instance of the given ParserContext.
	 *
	 * @param ctx is the context from which the Logger instance will be looked
	 * up. The sourceId specified in the context instance will be used as
	 * relative location for looking up the new resource. The registry specified
	 * in the ParserContext is used to lookup the parser instances and to
	 * locate the resources.
	 * @param path is the requested path of the file that should be included.
	 * @param mimetype is the mimetype of the resource that should be parsed
	 * (may be empty, in which case the mimetype is deduced from the file
	 * extension)
	 * @param rel is a "relation string" supplied by the user which specifies
	 * the relationship of the specified resource. May be empty, in which case
	 * the relation is deduced from the supported types and the types of the
	 * parser for the given mimetype.
	 * @param supportedTypes contains the types of the returned Node the caller
	 * can deal with. Note that only the types the parser claims to return are
	 * checked, not the actual result.
	 * @return the parsed nodes or an empty list if something went wrong.
	 */
	ManagedVector<Node> include(ParserContext &ctx, const std::string &path,
	                         const std::string &mimetype,
	                         const std::string &rel,
	                         const RttiSet &supportedTypes);

	/**
	 * Creates and returns a SourceContext structure containing information
	 * about the given SourceLocation (such as line and column number). Throws
	 * a LoggableException if an irrecoverable error occurs while looking up the
	 * context (such as a no longer existing resource).
	 *
	 * @param location is the SourceLocation for which context information
	 * should be retrieved. This method is used by the Logger class to print
	 * pretty messages.
	 * @param maxContextLength is the maximum length in character of context
	 * that should be extracted.
	 * @return a valid SourceContext if a valid SourceLocation was given or an
	 * invalid SourceContext if the location is invalid.
	 */
	SourceContext readContext(const SourceLocation &location,
	                          size_t maxContextLength);
	/**
	 * Creates and returns a SourceContext structure containing information
	 * about the given SourceLocation (such as line and column number). Throws
	 * a LoggableException if an irrecoverable error occurs while looking up the
	 * context (such as a no longer existing resource). Does not limit the
	 * context length.
	 *
	 * @param location is the SourceLocation for which context information
	 * should be retrieved. This method is used by the Logger class to print
	 * pretty messages.
	 * @return a valid SourceContext if a valid SourceLocation was given or an
	 * invalid SourceContext if the location is invalid.
	 */
	SourceContext readContext(const SourceLocation &location);

	/**
	 * Returns the sourceId for the given location string.
	 *
	 * @param location is the location string for which the resource id should
	 * be returned.
	 * @return the SourceId that can be used to identify the Resource, or
	 * InvalidSourceId if the specified location is not loaded.
	 */
	SourceId getSourceId(const std::string &location);

	/**
	 * Returns the sourceId for the given Resource.
	 *
	 * @param resource is the Resource for which the sourceId should be
	 * returned.
	 * @return the SourceId that can be used to identify the Resource, or
	 * InvalidSourceId if the specified resource is not loaded or invalid.
	 */
	SourceId getSourceId(const Resource &resource);

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
	 * Returns the node that is associated with the given SourceId or nullptr if
	 * the Node no longer exists or the supplied SourceId is invalid.
	 *
	 * @param mgr is the Manager instance that should be used to resolve the
	 * internal weak reference to the Node instance.
	 * @param sourceId is the id of the resource for which the parsed Node
	 * instance should be returned.
	 * @return the Node instance corresponding to the given sourceId.
	 */
	Rooted<Node> getNode(Manager &mgr, SourceId sourceId);

	/**
	 * Returns the node that is associated with the given location or nullptr if
	 * the Node no longer exists or the supplied location was never parsed.
	 *
	 * @param mgr is the Manager instance that should be used to resolve the
	 * internal weak reference to the Node instance.
	 * @param location is the location from which the node was parsed.
	 * @return the Node instance corresponding to the given location.
	 */
	Rooted<Node> getNode(Manager &mgr, const std::string &location);

	/**
	 * Returns the node that is associated with the given resource or nullptr if
	 * the Node no longer exists or the supplied resource was never parsed.
	 *
	 * @param mgr is the Manager instance that should be used to resolve the
	 * internal weak reference to the Node instance.
	 * @param resource is the resource from which the node was parsed.
	 * @return the Node instance corresponding to the given resource.
	 */
	Rooted<Node> getNode(Manager &mgr, const Resource &resource);

	/**
	 * Returns a SourceContextCallback that can be passed to a logger instance.
	 * Remeber to reset the SourceContextCallback after the Project instance has
	 * been freed.
	 *
	 * @return a SourceContextCallback that is coupled to this Project instance.
	 */
	SourceContextCallback getSourceContextCallback();
};
}

#endif /* _OUSIA_RESOURCE_MANAGER_HPP_ */

