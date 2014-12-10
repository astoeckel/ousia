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

#include <core/Logger.hpp>

#include <core/parser/Parser.hpp>

namespace ousia {

using namespace parser;

/* Class Registry */

void Registry::registerParser(parser::Parser *parser)
{
	parsers.push_back(parser);
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

void Registry::registerResourceLocator(ResourceLocator *locator)
{
	locators.push_back(locator);
}

ResourceLocator::Location Registry::locateResource(
    const std::string &path, const std::string &relativeTo,
    ResourceLocator::Type type) const
{
	ResourceLocator::Location *last;
	for (auto &locator : locators) {
		ResourceLocator::Location loc = locator->locate(path, relativeTo, type);
		if (loc.found) {
			return loc;
		}
		last = &loc;
	}
	return *last;
}
}

