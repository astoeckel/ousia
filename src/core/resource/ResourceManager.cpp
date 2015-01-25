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
#include <core/common/SourceContextReader.hpp>
#include <core/common/Utils.hpp>
#include <core/model/Node.hpp>
#include <core/model/Project.hpp>
#include <core/parser/Parser.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/Registry.hpp>

#include "ResourceManager.hpp"
#include "ResourceRequest.hpp"

namespace ousia {

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

Rooted<Node> ResourceManager::parse(Registry &registry, ParserContext &ctx,
                                    const ResourceRequest &req, ParseMode mode)
{
	// Locate the resource relative to the old resource, abort if this did not
	// work
	Resource resource;
	if (!req.locate(registry, ctx.getLogger(), resource,
	                getResource(ctx.getSourceId()))) {
		return nullptr;
	}

	// Allocate a new SourceId handle for this Resource
	SourceId sourceId = allocateSourceId(resource);

	// We can now try to parse the given file
	Rooted<Node> node;
	try {
		// Set the current source id in the logger instance
		{
			GuardedLogger logger(ctx.getLogger(), SourceLocation{sourceId});

			// Fetch the input stream and create a char reader
			std::unique_ptr<std::istream> is = resource.stream();
			CharReader reader(*is, sourceId);

			// Actually parse the input stream, distinguish the LINK and the
			// INCLUDE
			// mode
			switch (mode) {
				case ParseMode::LINK: {
					ParserScope scope;  // New empty parser scope instance
					ParserContext childCtx = ctx.clone(scope, sourceId);
					node = req.getParser()->parse(reader, childCtx);
				}
				case ParseMode::INCLUDE: {
					ParserContext childCtx = ctx.clone(sourceId);
					node = req.getParser()->parse(reader, childCtx);
				}
			}
		}
		if (node == nullptr) {
			throw LoggableException{"File \"" + resource.getLocation() +
			                        "\" cannot be parsed."};
		}
	}
	catch (LoggableException ex) {
		// Remove all data associated with the allocated source id
		purgeResource(sourceId);

		// Log the exception and return nullptr
		ctx.getLogger().log(ex);
		return nullptr;
	}

	// Store the parsed node along with the sourceId, if we are in the LINK mode
	if (mode == ParseMode::LINK) {
		storeNode(sourceId, node);
	}

	// Return the parsed node
	return node;
}

Rooted<Node> ResourceManager::link(Registry &registry, ParserContext &ctx,
                                   const std::string &path,
                                   const std::string &mimetype,
                                   const std::string &rel,
                                   const RttiSet &supportedTypes)
{
	ResourceRequest req{path, mimetype, rel, supportedTypes};
	if (req.deduce(registry, ctx.getLogger())) {
		return parse(registry, ctx, req, ParseMode::LINK);
	}
	return nullptr;
}

Rooted<Node> ResourceManager::include(Registry &registry, ParserContext &ctx,
                                      const std::string &path,
                                      const std::string &mimetype,
                                      const std::string &rel,
                                      const RttiSet &supportedTypes)
{
	ResourceRequest req{path, mimetype, rel, supportedTypes};
	if (req.deduce(registry, ctx.getLogger())) {
		return parse(registry, ctx, req, ParseMode::INCLUDE);
	}
	return nullptr;
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

SourceContext ResourceManager::readContext(const SourceLocation &location)
{
	return readContext(location, SourceContextReader::MAX_MAX_CONTEXT_LENGTH);
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
		Managed *managed = mgr.getManaged(it->second);
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
}

