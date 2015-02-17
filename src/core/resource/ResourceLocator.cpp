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

#include <sstream>

#include "Resource.hpp"
#include "ResourceLocator.hpp"

namespace ousia {

/* Class ResourceLocator */

std::vector<std::string> ResourceLocator::autocomplete(
    const std::string &path, const ResourceType type,
    const Resource &relativeTo) const
{
	// If the locator of the given relative resource is this locator instance,
	// use the location specified in the resource, otherwise just use no
	// "relativeTo" path.
	if (&relativeTo.getLocator() == this) {
		return autocomplete(path, type, relativeTo.getLocation());
	}
	return autocomplete(path, type, "");
}

std::vector<std::string> ResourceLocator::autocomplete(
    const std::string &path, const ResourceType type,
    const std::string &relativeTo) const
{
	// Try to locate the resource for the specified type, if not found, use
	// the "UNKNOWN" type.
	std::vector<std::string> res = doAutocomplete(path, type, relativeTo);
	if (!res.empty()) {
		return res;
	}

	// Use the "UNKNOWN" type
	if (type != ResourceType::UNKNOWN) {
		return doAutocomplete(path, ResourceType::UNKNOWN, relativeTo);
	}
	return std::vector<std::string>{};
}

bool ResourceLocator::locate(Resource &resource, const std::string &path,
                             const ResourceType type,
                             const Resource &relativeTo) const
{
	// If the locator of the given relative resource is this locator instance,
	// use the location specified in the resource, otherwise just use no
	// "relativeTo" path.
	if (&relativeTo.getLocator() == this) {
		return locate(resource, path, type, relativeTo.getLocation());
	}
	return locate(resource, path, type, "");
}

bool ResourceLocator::locate(Resource &resource, const std::string &path,
                             const ResourceType type,
                             const std::string &relativeTo) const
{
	// Try to locate the resource for the specified type, if not found, use
	// the "UNKNOWN" type.
	if (doLocate(resource, path, type, relativeTo)) {
		return true;
	}
	if (type != ResourceType::UNKNOWN) {
		return doLocate(resource, path, ResourceType::UNKNOWN, relativeTo);
	}
	return false;
}

std::unique_ptr<std::istream> ResourceLocator::stream(
    const std::string &location) const
{
	return doStream(location);
}

std::vector<std::string> ResourceLocator::doAutocomplete(
    const std::string &path, const ResourceType type,
    const std::string &relativeTo) const
{
	// Default implementation, just return the path again
	return std::vector<std::string>{path};
}

/* Class StaticResourceLocator */

bool StaticResourceLocator::doLocate(Resource &resource,
                                     const std::string &path,
                                     const ResourceType type,
                                     const std::string &relativeTo) const
{
	auto it = resources.find(path);
	if (it != resources.end()) {
		resource = Resource(true, *this, type, path);
		return true;
	}
	return false;
}

std::unique_ptr<std::istream> StaticResourceLocator::doStream(
    const std::string &location) const
{
	auto it = resources.find(location);
	if (it != resources.end()) {
		return std::unique_ptr<std::istream>{new std::stringstream{it->second}};
	}
	return std::unique_ptr<std::istream>{new std::stringstream{""}};
}

void StaticResourceLocator::store(const std::string &path,
                                  const std::string &data)
{
	auto it = resources.find(path);
	if (it != resources.end()) {
		it->second = data;
	} else {
		resources.emplace(path, data);
	}
}

/* Class NullResourceLocatorImpl */

bool NullResourceLocatorImpl::doLocate(Resource &resource,
                                       const std::string &path,
                                       const ResourceType type,
                                       const std::string &relativeTo) const
{
	return false;
}
std::unique_ptr<std::istream> NullResourceLocatorImpl::doStream(
    const std::string &location) const
{
	return std::unique_ptr<std::istream>{new std::stringstream{""}};
}

const NullResourceLocatorImpl NullResourceLocator;
}

