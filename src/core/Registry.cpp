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

#include <core/common/Exceptions.hpp>
#include <core/common/Utils.hpp>
#include <core/parser/Parser.hpp>
#include <core/resource/Resource.hpp>
#include <core/resource/ResourceLocator.hpp>

#include "Registry.hpp"

namespace ousia {

/* Class Registry */

void Registry::registerParser(const std::set<std::string> &mimetypes,
                              const RttiSet &types, Parser *parser)
{
	for (const std::string &mimetype : mimetypes) {
		// Make sure no other parser was given for this mimetype
		auto it = parsers.find(mimetype);
		if (it != parsers.end()) {
			throw OusiaException{std::string{"Parser for mimetype "} +
			                     mimetype +
			                     std::string{" already registered."}};
		}

		// Store a reference at the parser and a copy of the given RttiSet
		parsers[mimetype] = std::pair<Parser *, RttiSet>{parser, types};
	}
}

static const std::pair<Parser *, RttiSet> NullParser{nullptr, RttiSet{}};

const std::pair<Parser *, RttiSet> &Registry::getParserForMimetype(
    const std::string &mimetype) const
{
	const auto it = parsers.find(mimetype);
	if (it != parsers.end()) {
		return it->second;
	}
	return NullParser;
}

void Registry::registerExtension(const std::string &extension,
                                 const std::string &mimetype)
{
	// Always use extensions in lower case
	std::string ext = Utils::toLower(extension);

	// Make sure the extension is unique
	auto it = extensions.find(ext);
	if (it != extensions.end()) {
		throw OusiaException{std::string{"Extension "} + extension +
		                     std::string{" already registered."}};
	}

	// Register the mimetype
	extensions[ext] = mimetype;
}

std::string Registry::getMimetypeForExtension(
    const std::string &extension) const
{
	// Always use extensions in lower case
	std::string ext = Utils::toLower(extension);

	// Try to find the extension
	auto it = extensions.find(ext);
	if (it != extensions.end()) {
		return it->second;
	}
	return std::string{};
}

void Registry::registerResourceLocator(ResourceLocator *locator)
{
	locators.push_back(locator);
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

	return false;
}
}

