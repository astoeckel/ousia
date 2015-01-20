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

#ifndef NDEBUG
#include <iostream>
#endif

#include <algorithm>
#include <fstream>

#include <boost/filesystem.hpp>

#include "FileLocator.hpp"
#include "SpecialPaths.hpp"

namespace fs = boost::filesystem;

namespace ousia {

void FileLocator::addPath(const std::string &path,
                          std::vector<std::string> &paths)
{
	auto it = std::find(paths.begin(), paths.end(), path);
	if (it == paths.end()) {
		paths.push_back(path);
	}
}

void FileLocator::addSearchPath(const std::string &path,
                                std::set<ResourceType> types)
{
	if (path.empty() || !fs::exists(path)) {
		return;
	}

	// Canonicalize the given path, check whether it exists
	std::string canonicalPath = fs::canonical(path).generic_string();

#ifndef NDEBUG
	std::cout << "FileLocator: Adding search path " << canonicalPath
	          << std::endl;
#endif

	// Insert the path for all given types.
	for (auto &type : types) {
		auto it = searchPaths.find(type);
		if (it != searchPaths.end()) {
			addPath(canonicalPath, it->second);
		} else {
			searchPaths.insert({type, {canonicalPath}});
		}
	}
}

void FileLocator::addSearchPath(const std::string &path, ResourceType type)
{
	addSearchPath(path, std::set<ResourceType>{type});
}

void FileLocator::addDefaultSearchPaths(const std::string &relativeTo)
{
	// Abort if the base directory is empty
	if (relativeTo.empty()) {
		return;
	}

	// Abort if the base directory does not exist or is not a directory
	fs::path base(relativeTo);
	if (!fs::exists(base) || !fs::is_directory(base)) {
		return;
	}

	// Add the search paths
	addSearchPath(base.generic_string(), ResourceType::UNKNOWN);
	addSearchPath((base / "domain").generic_string(),
	              ResourceType::DOMAIN_DESC);
	addSearchPath((base / "typesystem").generic_string(),
	              ResourceType::TYPESYSTEM);
}

void FileLocator::addDefaultSearchPaths()
{
	addDefaultSearchPaths(SpecialPaths::getGlobalDataDir());
	addDefaultSearchPaths(SpecialPaths::getLocalDataDir());
#ifndef NDEBUG
	addDefaultSearchPaths(SpecialPaths::getDebugDataDir());
	addDefaultSearchPaths(SpecialPaths::getDebugTestdataDir());
#endif
}

bool FileLocator::doLocate(Resource &resource, const std::string &path,
                           const ResourceType type,
                           const std::string &relativeTo) const
{
	if (!relativeTo.empty()) {
		fs::path base(relativeTo);
		if (fs::exists(base)) {
			// Look if 'relativeTo' is a directory already.
			if (!fs::is_directory(base)) {
				// If not we use the parent directory.
				base = base.parent_path();
			}

			// Use the / operator to append the path.
			base /= path;

			// If we already found a fitting resource there, use that.
			if (fs::exists(base)) {
				std::string location = fs::canonical(base).generic_string();
#ifndef NDEBUG
				std::cout << "FileLocator: Found \"" << path << "\" at "
				          << location << std::endl;
#endif
				resource = Resource(true, *this, type, location);
				return true;
			}
		}
	}

	// Otherwise look in the search paths, search backwards, last defined search
	// paths have a higher precedence
	auto it = searchPaths.find(type);
	if (it != searchPaths.end()) {
		const auto &paths = it->second;
		for (auto it = paths.rbegin(); it != paths.rend(); it++) {
#ifndef NDEBUG
			std::cout << "FileLocator: Searching for \"" << path << "\" in "
			          << *it << std::endl;
#endif
			fs::path p{*it};
			p /= path;
			if (fs::exists(p)) {
				std::string location = fs::canonical(p).generic_string();
#ifndef NDEBUG
				std::cout << "FileLocator: Found \"" << path << "\" at "
				          << location << std::endl;
#endif
				resource = Resource(true, *this, type, location);
				return true;
			}
		}
	}
	return false;
}

std::unique_ptr<std::istream> FileLocator::doStream(
    const std::string &location) const
{
	std::unique_ptr<std::istream> ifs{new std::ifstream(location)};
	return std::move(ifs);
}
}
