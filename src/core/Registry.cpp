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

#include <core/parser/Parser.hpp>
#include <core/resource/Resource.hpp>
#include <core/resource/ResourceLocator.hpp>

#include "Registry.hpp"

namespace ousia {

using namespace parser;

/* Class Registry */

void Registry::registerParser(parser::Parser &parser)
{
	parsers.push_back(&parser);
	for (const auto &mime : parser->mimetypes()) {
		//TODO: This does not allow for multiple parsers with the same mimetype.
		// Is that how its supposed to be?
		parserMimetypes.insert(std::make_pair(mime, parser));
	}
}

Parser *Registry::getParserForMimetype(const std::string &mimetype) const
{
	const auto it = parserMimetypes.find(mimetype);
	if (it != parserMimetypes.end()) {
		return it->second;
	}
	return nullptr;
}

void Registry::registerResourceLocator(ResourceLocator &locator)
{
	locators.push_back(&locator);
}

bool Registry::locateResource(Resource &resource, const std::string &path,
                              ResourceType type,
                              const Resource &relativeTo) const
{
	// Try the locator of the given "relativeTo" resource first
	if (relativeTo.isValid()) {
		if (relativeTo.getLocator().locate(resource, path, type, relativeTo)) {
			return true;
		}
	}

	// Iterate over all registered locators and try to resolve the given path
	for (auto &locator : locators) {
		if (locator->locate(resource, path, type, relativeTo)) {
			return true;
		}
	}

	// If this did not work out, retry but use the UNKNOWN type.
	if (type != ResourceType::UNKNOWN) {
		for (auto &locator : locators) {
			if (locator->locate(resource, path, ResourceType::UNKNOWN,
			                    relativeTo)) {
				return true;
			}
		}
	}

	return false;
}
}

