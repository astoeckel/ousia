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

#include <iostream>

#include <string>

#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Utils.hpp>
#include <core/resource/Resource.hpp>
#include <core/resource/Resource.hpp>
#include <core/Registry.hpp>

#include "ResourceRequest.hpp"

namespace ousia {

namespace RttiTypes {
extern const Rtti Document;
extern const Rtti Domain;
extern const Rtti Node;
extern const Rtti Typesystem;
}

/**
 * Map mapping from Rtti pointers to the corresponding ResourceType.
 */
static const std::unordered_map<const Rtti *, ResourceType>
    RTTI_RESOURCE_TYPE_MAP{{&RttiTypes::Document, ResourceType::DOCUMENT},
                           {&RttiTypes::Domain, ResourceType::DOMAIN_DESC},
                           {&RttiTypes::Typesystem, ResourceType::TYPESYSTEM}};

/**
 * Function used internally to build a set with all currently supported
 * ResourceType instances.
 *
 * @param supportedTypes are all supported types.
 * @return a set containing all ResourceTypes that can be used for these
 * RTTI descriptors.
 */
static const std::unordered_set<ResourceType, Utils::EnumHash>
supportedResourceTypes(const RttiSet &supportedTypes)
{
	std::unordered_set<ResourceType, Utils::EnumHash> res;
	for (const Rtti *supportedType : supportedTypes) {
		auto it = RTTI_RESOURCE_TYPE_MAP.find(supportedType);
		if (it != RTTI_RESOURCE_TYPE_MAP.end()) {
			res.insert(it->second);
		}
	}
	return res;
}

/**
 * Converts a set of supported RTTI descriptors to a string describing the
 * corresponding ResourceTypes.
 *
 * @param supportedTypes are all supported types.
 * @return a string containing all corresponding resource types.
 */
static std::string supportedResourceTypesString(const RttiSet &supportedTypes)
{
	return Utils::join(supportedResourceTypes(supportedTypes), "\", \"", "\"",
	                   "\"");
}

/**
 * Tries to deduce the resource type from the given set of supported types.
 * Returns ResourceType::UNKNOWN if there are ambiguities.
 *
 * @param supportedTypes are all supported types.
 * @return the deduced ResourceType or ResourceType::UNKNOWN if there was an
 * ambiguity.
 */
static ResourceType deduceResourceType(const RttiSet &supportedTypes)
{
	ResourceType resourceType = ResourceType::UNKNOWN;
	for (const Rtti *supportedType : supportedTypes) {
		auto it = RTTI_RESOURCE_TYPE_MAP.find(supportedType);
		if (it != RTTI_RESOURCE_TYPE_MAP.end()) {
			// Preven ambiguity
			if (resourceType != ResourceType::UNKNOWN &&
			    resourceType != it->second) {
				resourceType = ResourceType::UNKNOWN;
				break;
			}
			resourceType = it->second;
		}
	}
	return resourceType;
}

/**
 * Function used to limit the supportedTypes to those that correspond to the
 * ResourceType.
 *
 * @param resourceType is the type of the resource type that is going to be
 * included.
 * @param supportedTypes are all supported types.
 * @return a restricted set of supportedTypes that correspond to the
 * resourceType.
 */
static RttiSet limitSupportedTypes(ResourceType resourceType,
                                   const RttiSet &supportedTypes)
{
	// Calculate the expected types
	RttiSet expectedTypes;
	for (auto entry : RTTI_RESOURCE_TYPE_MAP) {
		if (entry.second == resourceType) {
			expectedTypes.insert(entry.first);
		}
	}

	// Restrict the supported types to the expected types
	return Rtti::setIntersection(supportedTypes, expectedTypes);
}

/* Class ResourceRequest */

ResourceRequest::ResourceRequest(const std::string &path,
                                 const std::string &mimetype,
                                 const std::string &rel,
                                 const RttiSet &supportedTypes)
    : path(path),
      mimetype(mimetype),
      rel(rel),
      supportedTypes(supportedTypes),
      resourceType(ResourceType::UNKNOWN),
      parser(nullptr)
{
}

bool ResourceRequest::deduce(Registry &registry, Logger &logger)
{
	bool ok = true;

	// Make sure the given file name is not empty
	if (path.empty()) {
		logger.error("Filename may not be empty");
		return false;
	}

	// Try to deduce the mimetype if none was given
	if (mimetype.empty()) {
		mimetype = registry.getMimetypeForFilename(path);
		if (mimetype.empty()) {
			logger.error(std::string("Filename \"") + path +
			             std::string(
			                 "\" has an unknown file extension. Explicitly "
			                 "specify a mimetype."));
			ok = false;
		}
	}

	// Find a parser for the mimetype
	if (!mimetype.empty()) {
		auto parserDescr = registry.getParserForMimetype(mimetype);
		parser = parserDescr.first;
		parserTypes = parserDescr.second;

		// Make sure a valid parser was returned, and if yes, whether the
		// parser is allows to run here
		if (!parser) {
			logger.error(std::string("Cannot parse files of type \"") +
			             mimetype + std::string("\""));
			ok = false;
		} else if (!Rtti::setIsOneOf(supportedTypes, parserTypes)) {
			logger.error(std::string("Resource of type \"") + mimetype +
			             std::string("\" cannot be included here!"));
			ok = false;
		}
	}

	// Try to deduce the ResourceType from the "rel" string
	if (!rel.empty()) {
		resourceType = Resource::getResourceTypeByName(rel);
		if (resourceType == ResourceType::UNKNOWN) {
			logger.error(std::string("Unknown relation \"") + rel +
			             std::string("\", expected one of ") +
			             supportedResourceTypesString(supportedTypes));
			ok = false;
		}
	}

	// Try to deduce the ResourceType from the supportedTypes
	if (resourceType == ResourceType::UNKNOWN) {
		resourceType = deduceResourceType(supportedTypes);
	}

	// Further limit the supportedTypes to those types that correspond to the
	// specified resource type.
	if (resourceType != ResourceType::UNKNOWN) {
		supportedTypes = limitSupportedTypes(resourceType, supportedTypes);
		if (supportedTypes.empty()) {
			logger.error(std::string("Resource of type \"") + mimetype +
			             std::string("\" and relationship \"") +
			             Resource::getResourceTypeName(resourceType) +
			             std::string("\" cannot be included here"));
			ok = false;
		}
	} else if (supportedTypes.size() != 1 ||
	           *supportedTypes.begin() != &RttiTypes::Node) {
		logger.warning(std::string(
		                   "Ambiguous resource relationship, consider "
		                   "specifying one of ") +
		               supportedResourceTypesString(supportedTypes) +
		               std::string(" as \"rel\" attribute"));
	}

	return ok;
}

bool ResourceRequest::locate(Registry &registry, Logger &logger,
                             Resource &resource,
                             const Resource &relativeTo) const
{
	if (!registry.locateResource(resource, path, resourceType, relativeTo)) {
		logger.error(std::string("File not found: ") + path);
		return false;
	}
	return true;
}
}

