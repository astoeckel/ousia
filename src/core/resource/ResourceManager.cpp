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

#include <vector>

#include <core/common/CharReader.hpp>
#include <core/common/Exceptions.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Utils.hpp>
#include <core/model/Node.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/parser/Parser.hpp>
#include <core/Registry.hpp>

#include "ResourceManager.hpp"
#include "ResourceUtils.hpp"

namespace ousia {

/* Static helper functions */

static void logUnsopportedType(Logger &logger, Resource &resource,
                               const RttiSet &supportedTypes)
{
	// Build a list containing the expected type names
	std::vector<std::string> expected;
	for (const Rtti *supportedType : supportedTypes) {
		expected.push_back(supportedType->name);
	}

	// Log the actual error message
	logger.error(
	    std::string("Expected the file \"") + resource.getLocation() +
	    std::string("\" to define one of the following internal types ") +
	    Utils::join(expected, ", ", "{", "}"));
}

/* Class ResourceManager */

SourceId ResourceManager::allocateSourceId(const Resource &resource)
{
	// Increment the source id and make sure the values don't overflow
	SourceId sourceId = nextSourceId++;
	if (sourceId == InvalidSourceId) {
		nextSourceId = InvalidSourceId;
		throw OusiaException{"Internal resource handles depleted!"};
	}

	// Register the node and the resource with this id
	locations[resource.getLocation()] = sourceId;
	resources[sourceId] = resource;

	return sourceId;
}

void ResourceManager::storeNode(SourceId sourceId, Handle<Node> node)
{
	nodes[sourceId] = node->getUid();
}

void ResourceManager::purgeResource(SourceId sourceId)
{
	Resource res = getResource(sourceId);
	if (res.isValid()) {
		locations.erase(res.getLocation());
	}
	resources.erase(sourceId);
	nodes.erase(sourceId);
	contextReaders.erase(sourceId);
}

Rooted<Node> ResourceManager::parse(ParserContext &ctx, Resource &resource,
                                    const std::string &mimetype,
                                    const RttiSet &supportedTypes)
{
	// Try to deduce the mimetype of no mimetype was given
	std::string mime = mimetype;
	if (mime.empty()) {
		mime = ctx.registry.getMimetypeForFilename(resource.getLocation());
		if (mime.empty()) {
			ctx.logger.error(std::string("Filename \"") +
			                 resource.getLocation() +
			                 std::string(
			                     "\" has an unknown file extension. Explicitly "
			                     "specify a mimetype."));
			return nullptr;
		}
	}

	// Fetch a parser for the mimetype
	const std::pair<Parser *, RttiSet> &parserDescr =
	    ctx.registry.getParserForMimetype(mime);
	Parser *parser = parserDescr.first;

	// Make sure a parser was found
	if (!parser) {
		ctx.logger.error(std::string("Cannot parse files of type \"") + mime +
		                 std::string("\""));
		return nullptr;
	}

	// Make sure the parser returns at least one of the supported types
	if (!Rtti::setIsOneOf(parserDescr.second, supportedTypes)) {
		logUnsopportedType(ctx.logger, resource, supportedTypes);
		return nullptr;
	}

	// Allocate a new SourceId handle for this Resource
	SourceId sourceId = allocateSourceId(resource);

	// We can now try to parse the given file
	Rooted<Node> node;
	try {
		// Set the current source id in the logger instance
		ScopedLogger logger(ctx.logger, SourceLocation{sourceId});

		// Fetch the input stream and create a char reader
		std::unique_ptr<std::istream> is = resource.stream();
		CharReader reader(*is, sourceId);

		// Actually parse the input stream
		node = parser->parse(reader, ctx);
		if (node == nullptr) {
			throw LoggableException{"Internal error: Parser returned null."};
		}
	}
	catch (LoggableException ex) {
		// Remove all data associated with the allocated source id
		purgeResource(sourceId);

		// Log the exception and return nullptr
		ctx.logger.log(ex);
		return nullptr;
	}

	// Store the parsed node along with the sourceId
	storeNode(sourceId, node);

	// Return the parsed node
	return node;
}

SourceId ResourceManager::getSourceId(const std::string &location)
{
	auto it = locations.find(location);
	if (it != locations.end()) {
		return it->second;
	}
	return InvalidSourceId;
}

SourceId ResourceManager::getSourceId(const Resource &resource)
{
	if (resource.isValid()) {
		return getSourceId(resource.getLocation());
	}
	return InvalidSourceId;
}

const Resource &ResourceManager::getResource(SourceId sourceId) const
{
	auto it = resources.find(sourceId);
	if (it != resources.end()) {
		return it->second;
	}
	return NullResource;
}

Rooted<Node> ResourceManager::getNode(Manager &mgr, SourceId sourceId)
{
	auto it = nodes.find(sourceId);
	if (it != nodes.end()) {
		Managed *managed = mgr.getManaged(sourceId);
		if (managed != nullptr) {
			return dynamic_cast<Node *>(managed);
		} else {
			purgeResource(sourceId);
		}
	}
	return nullptr;
}

Rooted<Node> ResourceManager::getNode(Manager &mgr, const std::string &location)
{
	return getNode(mgr, getSourceId(location));
}

Rooted<Node> ResourceManager::getNode(Manager &mgr, const Resource &resource)
{
	return getNode(mgr, getSourceId(resource));
}

Rooted<Node> ResourceManager::link(ParserContext &ctx, const std::string &path,
                                   const std::string &mimetype,
                                   const std::string &rel,
                                   const RttiSet &supportedTypes,
                                   const Resource &relativeTo)
{
	// Try to deduce the ResourceType
	ResourceType resourceType =
	    ResourceUtils::deduceResourceType(rel, supportedTypes, ctx.logger);

	// Lookup the resource for given path and resource type
	Resource resource;
	if (!ctx.registry.locateResource(resource, path, resourceType,
	                                 relativeTo)) {
		ctx.logger.error("File \"" + path + "\" not found.");
		return nullptr;
	}

	// Try to shrink the set of supportedTypes
	RttiSet types = ResourceUtils::limitRttiTypes(supportedTypes, rel);

	// Check whether the resource has already been parsed
	Rooted<Node> node = getNode(ctx.manager, resource);
	if (node == nullptr) {
		// Node has not already been parsed, parse it now
		node = parse(ctx, resource, mimetype, supportedTypes);

		// Abort if parsing failed
		if (node == nullptr) {
			return nullptr;
		}
	}

	// Make sure the node has one of the supported types
	if (!node->type().isOneOf(supportedTypes)) {
		logUnsopportedType(ctx.logger, resource, supportedTypes);
		return nullptr;
	}

	return node;
}

Rooted<Node> ResourceManager::link(ParserContext &ctx, const std::string &path,
                                   const std::string &mimetype,
                                   const std::string &rel,
                                   const RttiSet &supportedTypes,
                                   SourceId relativeTo)
{
	// Fetch the resource corresponding to the source id, make sure it is valid
	const Resource &relativeResource = getResource(relativeTo);
	if (!relativeResource.isValid()) {
		ctx.logger.fatalError("Internal error: Invalid SourceId supplied.");
		return nullptr;
	}

	// Continue with the usual include routine
	return link(ctx, path, mimetype, rel, supportedTypes, relativeResource);
}

SourceContext ResourceManager::readContext(const SourceLocation &location,
                                           size_t maxContextLength)
{
	const Resource &resource = getResource(location.getSourceId());
	if (resource.isValid()) {
		// Fetch a char reader for the resource
		std::unique_ptr<std::istream> is = resource.stream();
		CharReader reader{*is, location.getSourceId()};

		// Return the context
		return contextReaders[location.getSourceId()].readContext(
		    reader, location, maxContextLength, resource.getLocation());
	}
	return SourceContext{};
}
}

