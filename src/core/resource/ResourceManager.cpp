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

#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Utils.hpp>
#include <core/model/Node.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/Registry.hpp>

#include "Resource.hpp"
#include "ResourceManager.hpp"

namespace ousia {

/* Deduction of the ResourceType */

namespace RttiTypes {
extern const Rtti Document;
extern const Rtti Domain;
extern const Rtti Node;
extern const Rtti Typesystem;
}

/**
 * Map mapping from relations (the "rel" attribute in includes) to the
 * corresponding ResourceType.
 */
static const std::unordered_map<std::string, ResourceType> RelResourceTypeMap{
    {"document", ResourceType::DOCUMENT},
    {"domain", ResourceType::DOMAIN_DESC},
    {"typesystem", ResourceType::TYPESYSTEM}};

/**
 * Map mapping from Rtti pointers to the corresponding ResourceType.
 */
static const std::unordered_map<Rtti *, ResourceType> RttiResourceTypeMap{
    {&RttiTypes::Document, ResourceType::DOCUMENT},
    {&RttiTypes::Domain, ResourceType::DOMAIN_DESC},
    {&RttiTypes::Typesystem, ResourceType::TYPESYSTEM}};

static ResourceType relToResourceType(const std::string &rel, Logger &logger)
{
	std::string s = Utils::toLowercase(rel);
	if (!s.empty()) {
		auto it = RelResourceTypeMap.find(s);
		if (it != RelResourceTypeMap.end()) {
			return it->second;
		} else {
			logger.error(std::string("Unknown relation \"") + rel +
			             std::string("\""));
		}
	}
	return ResourceType::UNKNOWN;
}

static ResourceType supportedTypesToResourceType(const RttiSet &supportedTypes)
{
	if (supportedTypes.size() == 1U) {
		auto it = RttiResourceTypeMap.find(supportedTypes[0]);
		if (it != RelResourceTypeMap.end()) {
			return it->second;
		}
	}
	return ResourceType::UNKNOWN;
}

static ResourceType deduceResourceType(const std::string &rel,
                                       const RttiSet &supportedTypes,
                                       Logger &logger)
{
	ResourceType res;

	// Try to deduce the ResourceType from the "rel" attribute
	res = relToResourceType(rel, logger);

	// If this did not work, try to deduce the ResourceType from the
	// supportedTypes supplied by the Parser instance.
	if (res == ResourceType::UNKNOWN) {
		res = supportedTypesToResourceType(supportedTypes);
	}
	if (res == ResourceType::UNKNOWN) {
		logger.note(
		    "Ambigous resource type, consider specifying the \"rel\" "
		    "attribute");
	}
	return res;
}

/* Functions for reducing the set of supported types */

/**
 * Map mapping from relations (the "rel" attribute in includes) to the
 * corresponding RttiType
 */
static const std::unordered_map<std::string, Rtti *> RelRttiTypeMap{
    {"document", &RttiTypes::DOCUMENT},
    {"domain", &RttiTypes::DOMAIN},
    {"typesystem", &RttiTypes::TYPESYSTEM}};

static Rtti *relToRttiType(const std::string &rel)
{
	std::string s = Utils::toLowercase(rel);
	if (!s.empty()) {
		auto it = RelRttiTypeMap.find(s);
		if (it != RelRttiTypeMap.end()) {
			return it->second;
		}
	}
	return &ResourceType::Node;
}

static RttiType shrinkSupportedTypes(const RttiSet &supportedTypes,
                                     const std::string &rel)
{
	RttiSet types;
	RttiType *type = relToRttiType(rel);
	for (RttiType *supportedType : supportedTypes) {
		if (supportedType->isa(type)) {
			types.insert(supportedType);
		}
	}
	return types;
}

/* Class ResourceManager */

Rooted<Node> ResourceManager::link(ParserContext &ctx, Resource &resource,
                                   const std::string &mimetype,
                                   const RttiSet &supportedTypes)
{
	
}

Rooted<Node> ResourceManager::link(ParserContext &ctx, const std::string &path,
                                   const std::string &mimetype,
                                   const std::string &rel,
                                   const RttiSet &supportedTypes,
                                   const Resource &relativeTo)
{
	// Try to deduce the ResourceType
	ResourceType resourceType =
	    deduceResourceType(rel, supportedTypes, ctx.logger);

	// Lookup the resource for given path and resource type
	Resource resource;
	if (!ctx.registry.locateResource(resource, path, resourceType,
	                                 relativeTo)) {
		ctx.logger.error("File \"" + path + "\" not found.");
		return nullptr;
	}

	// Try to shrink the set of supportedTypes
	RttiSet types = shrinkSupportedTypes(supportedTypes, rel);

	// Check whether the resource has already been parsed
	Rooted<Node> node = nullptr;
	auto it = locations.find(res.getLocation());
	if (it != locations.end()) {
		node = 
	}
	 = link(ctx, resource, mimetype, types);

	// Try to deduce the mimetype
	std::string mime = mimetype;
	if (mime.empty()) {
		// Fetch the file extension
		std::string ext = Utils::extractFileExtension(path);
		if (ext.empty()) {
			ctx.logger.error(
			    std::string("Specified filename \"") + path +
			    std::string(
			        "\" has no extension and no mimetype (\"type\" "
			        "attribute) was given instead."));
			return nullptr;
		}

		// Fetch the mimetype for the extension
		mime = ctx.registry.getMimetypeForExtension(ext);
		if (mime.empty()) {
			ctx.logger.error(std::string("Unknown file extension \"") + ext +
			                 std::string("\""));
			return nullptr;
		}
	}

	// Fetch a parser for the mimetype
	const std::pair<Parser *, RttiSet> parser =
	    ctx.registry.getParserForMimetype(mime);

	// Make sure a parser was found
	if (!parser->first) {
		ctx.logger.error(std::string("Cannot parse files of type \"") + mime +
		                 std::string("\""));
	}

	// Make sure the parser returns one of the supported types
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
	return include(ctx, path, mimetype, rel, supportedTypes, relativeResource);
}

const Resource &getResource(SourceId sourceId) const
{
	if (sourceId < resources.size()) {
		return resources[sourceId];
	}
	return NullResource;
}

SourceContext ResourceManager::buildContext(const SourceLocation &location)
{
	SourceContext res;

	// TODO

	return res;
}
};
}

#endif /* _OUSIA_RESOURCE_MANAGER_HPP_ */

