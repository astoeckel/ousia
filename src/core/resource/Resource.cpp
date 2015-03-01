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

#include <unordered_map>
#include <unordered_set>

#include <core/common/Utils.hpp>

#include "Resource.hpp"
#include "ResourceLocator.hpp"

namespace ousia {

/* Helper functions for the internal maps */

static std::unordered_map<ResourceType, std::string, Utils::EnumHash> reverseMap(
    const std::unordered_map<std::string, ResourceType> &m)
{
	std::unordered_map<ResourceType, std::string, Utils::EnumHash> res;
	for (auto e : m) {
		res.emplace(e.second, e.first);
	}
	return res;
}

/* Internal maps */

static const std::unordered_map<std::string, ResourceType>
    NAME_RESOURCE_TYPE_MAP{{"document", ResourceType::DOCUMENT},
                           {"ontology", ResourceType::ONTOLOGY},
                           {"typesystem", ResourceType::TYPESYSTEM},
                           {"attributes", ResourceType::ATTRIBUTES},
                           {"stylesheet", ResourceType::STYLESHEET},
                           {"script", ResourceType::SCRIPT},
                           {"data", ResourceType::DATA}};

static const std::unordered_map<ResourceType, std::string, Utils::EnumHash>
    RESOURCE_TYPE_NAME_MAP = reverseMap(NAME_RESOURCE_TYPE_MAP);

/* Class Resource */

Resource::Resource()
    : Resource(false, NullResourceLocator, ResourceType::UNKNOWN, "")
{
}

Resource::Resource(bool valid, const ResourceLocator &locator,
                   ResourceType type, const std::string &location)
    : valid(valid), locator(&locator), type(type), location(location)
{
}

std::unique_ptr<std::istream> Resource::stream() const
{
	return locator->stream(location);
}

std::string Resource::getResourceTypeName(ResourceType resourceType)
{
	auto it = RESOURCE_TYPE_NAME_MAP.find(resourceType);
	if (it != RESOURCE_TYPE_NAME_MAP.end()) {
		return it->second;
	}
	return "unknown";
}

ResourceType Resource::getResourceTypeByName(const std::string &name)
{
	std::string normName = Utils::toLower(name);
	auto it = NAME_RESOURCE_TYPE_MAP.find(normName);
	if (it != NAME_RESOURCE_TYPE_MAP.end()) {
		return it->second;
	}
	return ResourceType::UNKNOWN;
}

/* Operators */

std::ostream &operator<<(std::ostream &os, ResourceType resourceType)
{
	return os << Resource::getResourceTypeName(resourceType);
}

/* NullResource instance */

const Resource NullResource{};
}

