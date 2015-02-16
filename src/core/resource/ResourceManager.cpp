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

template <class T>
class GuardedSetInsertion {
private:
	std::unordered_set<T> &set;
	T value;
	bool success;

public:
	GuardedSetInsertion(std::unordered_set<T> &set, T value)
	    : set(set), value(value)
	{
		success = set.insert(value).second;
	}

	~GuardedSetInsertion() { set.erase(value); }

	bool isSuccess() { return success; }
};

NodeVector<Node> ResourceManager::parse(
    ParserContext &ctx, const std::string &path, const std::string &mimetype,
    const std::string &rel, const RttiSet &supportedTypes, ParseMode mode)
{
	// Some references used for convenience
	Registry &registry = ctx.getRegistry();
	Logger &logger = ctx.getLogger();
	ParserScope &scope = ctx.getScope();
	Resource relativeTo = getResource(ctx.getSourceId());

	// Locate the resource relative to the old resource, abort if this did not
	// work
	ResourceRequest req{path, mimetype, rel, supportedTypes, relativeTo};
	Resource resource;
	if (!req.deduce(registry, logger) ||
	    !req.locate(registry, logger, resource)) {
		return NodeVector<Node>{};
	}

	// initialize the output vector.
	NodeVector<Node> parsedNodes;

	// Allocate a new SourceId handle for this Resource
	bool newResource = false;
	SourceId sourceId = getSourceId(resource);
	if (sourceId == InvalidSourceId) {
		newResource = true;
		sourceId = allocateSourceId(resource);
	}
	// check for cycles.
	GuardedSetInsertion<SourceId> cycleDetection{currentlyParsing, sourceId};
	if (!cycleDetection.isSuccess()) {
		throw LoggableException{std::string("Detected cyclic parse of ") +
		                        resource.getLocation()};
	}

	if (!newResource && mode == ParseMode::IMPORT) {
		// if a already imported resource should be imported we just use the
		// cached node.
		parsedNodes.push_back(getNode(ctx.getManager(), sourceId));
	} else {
		// We can now try to parse the given file

		// Set the current source id in the logger instance. Note that this
		// modifies the logger instance -- the GuardedLogger is just used to
		// make sure the default location is popped from the stack again.
		GuardedLogger guardedLogger(logger, SourceLocation{sourceId});

		try {
			// Fetch the input stream and create a char reader
			std::unique_ptr<std::istream> is = resource.stream();
			CharReader reader(*is, sourceId);

			// Actually parse the input stream, distinguish the IMPORT and the
			// INCLUDE mode
			switch (mode) {
				case ParseMode::IMPORT: {
					// Create a new, empty parser scope instance and a new
					// parser
					// context with this instance in place
					ParserScope innerScope;
					ParserContext childCtx = ctx.clone(innerScope, sourceId);

					// Run the parser
					req.getParser()->parse(reader, childCtx);

					// Make sure the scope has been unwound and perform all
					// deferred resolutions
					innerScope.checkUnwound(logger);
					innerScope.performDeferredResolution(logger);

					// Fetch the nodes that were parsed by this parser instance
					// and
					// validate them
					parsedNodes = innerScope.getTopLevelNodes();
					for (auto parsedNode : parsedNodes) {
						parsedNode->validate(logger);
					}

					// Make sure the number of elements is exactly one -- we can
					// only store one element per top-level node.
					if (parsedNodes.empty()) {
						throw LoggableException{"Module is empty."};
					}
					if (parsedNodes.size() > 1) {
						throw LoggableException{
						    std::string(
						        "Expected exactly one top-level node but "
						        "got ") +
						    std::to_string(parsedNodes.size())};
					}

					// Store the parsed node along with the sourceId
					storeNode(sourceId, parsedNodes[0]);

					break;
				}
				case ParseMode::INCLUDE: {
					// Fork the scope instance and create a new parser context
					// with this instance in place
					ParserScope forkedScope = scope.fork();
					ParserContext childCtx = ctx.clone(forkedScope, sourceId);

					// Run the parser
					req.getParser()->parse(reader, childCtx);

					// Join the forked scope with the outer scope
					scope.join(forkedScope, logger);

					// Fetch the nodes that were parsed by this parser instance
					parsedNodes = forkedScope.getTopLevelNodes();

					break;
				}
			}
		}
		catch (LoggableException ex) {
			// Log the exception and return nullptr
			logger.log(ex);
			return NodeVector<Node>{};
		}
	}

	// Make sure the parsed nodes fulfill the "supportedTypes" constraint,
	// remove nodes that do not the result
	for (auto it = parsedNodes.begin(); it != parsedNodes.end();) {
		const Rtti *type = (*it)->type();
		if (!type->isOneOf(supportedTypes)) {
			logger.error(std::string("Node of internal type ") + type->name +
			                 std::string(" not supported here"),
			             **it);
			it = parsedNodes.erase(it);
		} else {
			it++;
		}
	}

	return parsedNodes;
}

Rooted<Node> ResourceManager::import(ParserContext &ctx,
                                     const std::string &path,
                                     const std::string &mimetype,
                                     const std::string &rel,
                                     const RttiSet &supportedTypes)
{
	NodeVector<Node> res =
	    parse(ctx, path, mimetype, rel, supportedTypes, ParseMode::IMPORT);
	if (res.size() == 1U) {
		return res[0];
	}
	return nullptr;
}

NodeVector<Node> ResourceManager::include(ParserContext &ctx,
                                          const std::string &path,
                                          const std::string &mimetype,
                                          const std::string &rel,
                                          const RttiSet &supportedTypes)
{
	return parse(ctx, path, mimetype, rel, supportedTypes, ParseMode::INCLUDE);
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

SourceContextCallback ResourceManager::getSourceContextCallback()
{
	return [this](const SourceLocation &location) {
		return this->readContext(location);
	};
}
}

