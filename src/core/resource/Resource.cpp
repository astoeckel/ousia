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

#include "Resource.hpp"
#include "ResourceLocator.hpp"

namespace ousia {

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

/* NullResource instance */

const Resource NullResource{};
}

