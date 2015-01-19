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

#ifndef _OUSIA_REGISTRY_HPP_
#define _OUSIA_REGISTRY_HPP_

#include <map>
#include <vector>

#include <core/resource/Resource.hpp>

namespace ousia {

// TODO: Add support for ScriptEngine type

namespace parser {
class Parser;
}
class ResourceLocator;

class Registry {
private:
	std::vector<parser::Parser *> parsers;
	std::map<std::string, parser::Parser *> parserMimetypes;

	std::vector<ResourceLocator *> locators;

public:
	void registerParser(parser::Parser &parser);

	parser::Parser *getParserForMimetype(const std::string &mimetype) const;

	void registerResourceLocator(ResourceLocator &locator);

	bool locateResource(Resource &resource, const std::string &path,
	                    ResourceType type = ResourceType::UNKNOWN,
	                    const Resource &relativeTo = NullResource) const;
};
}

#endif /* _OUSIA_REGISTRY_HPP_ */

