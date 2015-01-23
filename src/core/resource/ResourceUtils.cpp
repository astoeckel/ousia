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

#include "ResourceUtils.hpp"

namespace ousia {

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
 * Map mapping from relations to the corresponding Rtti descriptor.
 */
static const std::unordered_map<std::string, const Rtti *> RelRttiTypeMap{
    {"document", &RttiTypes::Document},
    {"domain", &RttiTypes::Domain},
    {"typesystem", &RttiTypes::Typesystem}};

/**
 * Map mapping from Rtti pointers to the corresponding ResourceType.
 */
static const std::unordered_map<const Rtti *, ResourceType> RttiResourceTypeMap{
    {&RttiTypes::Document, ResourceType::DOCUMENT},
    {&RttiTypes::Domain, ResourceType::DOMAIN_DESC},
    {&RttiTypes::Typesystem, ResourceType::TYPESYSTEM}};

ResourceType ResourceUtils::deduceResourceType(const std::string &rel,
                                               const RttiSet &supportedTypes,
                                               Logger &logger)
{
	ResourceType res;

	// Try to deduce the ResourceType from the "rel" attribute
	res = deduceResourceType(rel, logger);

	// If this did not work, try to deduce the ResourceType from the
	// supportedTypes supplied by the Parser instance.
	if (res == ResourceType::UNKNOWN) {
		res = deduceResourceType(supportedTypes, logger);
	}
	if (res == ResourceType::UNKNOWN) {
		logger.note(
		    "Ambigous resource type, consider specifying the \"rel\" "
		    "attribute");
	}
	return res;
}

ResourceType ResourceUtils::deduceResourceType(const std::string &rel,
                                               Logger &logger)
{
	std::string s = Utils::toLower(rel);
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

ResourceType ResourceUtils::deduceResourceType(const RttiSet &supportedTypes,
                                               Logger &logger)
{
	if (supportedTypes.size() == 1U) {
		auto it = RttiResourceTypeMap.find(*supportedTypes.begin());
		if (it != RttiResourceTypeMap.end()) {
			return it->second;
		}
	}
	return ResourceType::UNKNOWN;
}

const Rtti *ResourceUtils::deduceRttiType(const std::string &rel)
{
	std::string s = Utils::toLower(rel);
	if (!s.empty()) {
		auto it = RelRttiTypeMap.find(s);
		if (it != RelRttiTypeMap.end()) {
			return it->second;
		}
	}
	return &RttiTypes::Node;
}

RttiSet ResourceUtils::limitRttiTypes(const RttiSet &supportedTypes,
                                      const std::string &rel)
{
	return limitRttiTypes(supportedTypes, deduceRttiType(rel));
}

RttiSet ResourceUtils::limitRttiTypes(const RttiSet &supportedTypes,
                                      const Rtti *type)
{
	RttiSet res;
	for (const Rtti *supportedType : supportedTypes) {
		if (supportedType->isa(*type)) {
			res.insert(supportedType);
		}
	}
	return res;
}
}
