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
//#define FILELOCATOR_DEBUG_PRINT
#endif

#ifdef FILELOCATOR_DEBUG_PRINT
#include <iostream>
#endif

#include <algorithm>
#include <fstream>

#include <boost/filesystem.hpp>

#include <core/common/Utils.hpp>

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
	// Skip empty or non-existant paths
	if (path.empty() || !fs::exists(path) || !fs::is_directory(path)) {
		return;
	}

	// Canonicalize the given path, check whether it exists
	std::string canonicalPath = fs::canonical(path).generic_string();

#ifdef FILELOCATOR_DEBUG_PRINT
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
	if (relativeTo.empty() || !fs::exists(relativeTo) ||
	    !fs::is_directory(relativeTo)) {
		return;
	}

	// Add the search paths
	fs::path base{relativeTo};
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
#endif
	// also add working directory.
	addSearchPath(".", {ResourceType::UNKNOWN, ResourceType::DOMAIN_DESC,
	                    ResourceType::TYPESYSTEM, ResourceType::DOCUMENT,
	                    ResourceType::ATTRIBUTES, ResourceType::STYLESHEET,
	                    ResourceType::SCRIPT, ResourceType::DATA});
}

void FileLocator::addUnittestSearchPath(const std::string &subdir,
                                        ResourceType type)
{
	addSearchPath((fs::path{SpecialPaths::getDebugTestdataDir()} / subdir)
	                  .generic_string(),
	              type);
}

template <typename CallbackType>
static bool iteratePaths(const FileLocator::SearchPaths &searchPaths,
                         const std::string &path, const ResourceType type,
                         const std::string &relativeTo, CallbackType callback)
{
#ifdef FILELOCATOR_DEBUG_PRINT
	std::cout << "FileLocator: Searching for \"" << path << "\"" << std::endl;
#endif

	// Divide the given path into the directory and the filename
	fs::path p{path};
	fs::path dir = p.parent_path();
	std::string filename = p.filename().generic_string();

	// Check whether the given resource has an absolute path -- if yes, call the
	// callback function and do not try any search paths
	if (dir.is_absolute()) {
		return callback(dir, filename);
	}

	// If the path starts with "./" or "../" only perform relative lookups!
	if (path.substr(0, 2) != "./" && path.substr(0, 3) != "../") {
		// Look in the search paths, search backwards, last defined search
		// paths have a higher precedence
		auto it = searchPaths.find(type);
		if (it != searchPaths.end()) {
			const auto &paths = it->second;
			for (auto it = paths.rbegin(); it != paths.rend(); it++) {
#ifdef FILELOCATOR_DEBUG_PRINT
				std::cout << "FileLocator: Entering " << *it << std::endl;
#endif
				// Concatenate the searchpath with the given directory
				fs::path curDir = fs::path(*it) / dir;
				if (callback(curDir, filename)) {
					return true;
				}
			}
		}
	}

	// Perform relative lookups
	if (!relativeTo.empty()) {
		fs::path curDir(relativeTo);
		if (fs::exists(curDir)) {
			// Look if 'relativeTo' is a directory already.
			if (!fs::is_directory(curDir)) {
				// If not we use the parent directory.
				curDir = curDir.parent_path();
			}

			// Append the directory to the base path and try to resolve this
			// pair
			curDir = curDir / dir;

			// If we already found a fitting resource there, use that.
			if (callback(curDir, filename)) {
				return true;
			}
		}
	}
	return false;
}

bool FileLocator::doLocate(Resource &resource, const std::string &path,
                           const ResourceType type,
                           const std::string &relativeTo) const
{
	return iteratePaths(
	    searchPaths, path, type, relativeTo,
	    [&](const fs::path &dir, const std::string &filename) -> bool {
		    // Combine directory and filename
		    fs::path p = dir / filename;

		    // Check whether p exists
		    if (fs::exists(p) && fs::is_regular_file(p)) {
			    std::string location = fs::canonical(p).generic_string();
#ifdef FILELOCATOR_DEBUG_PRINT
			    std::cout << "FileLocator: Found at " << location << std::endl;
#endif
			    resource = Resource(true, *this, type, location);
			    return true;
		    }
		    return false;
		});
}

std::vector<std::string> FileLocator::doAutocomplete(
    const std::string &path, const ResourceType type,
    const std::string &relativeTo) const
{
	std::vector<std::string> res;
	iteratePaths(searchPaths, path, type, relativeTo,
	             [&](const fs::path &dir, const std::string &filename) -> bool {
		// Make sure the given directory actually is a directory
		if (!fs::is_directory(dir)) {
			return false;
		}

		// Iterate over the directory content
		fs::directory_iterator end;
		for (fs::directory_iterator it(dir); it != end; it++) {
			const std::string fn = it->path().filename().generic_string();
			if (!fn.empty() && fn[fn.size() - 1] != '~' &&
			    Utils::startsWith(fn, filename)) {
				res.push_back(it->path().generic_string());
			}
		}
		return !res.empty();
	});
	return res;
}

std::unique_ptr<std::istream> FileLocator::doStream(
    const std::string &location) const
{
	return std::unique_ptr<std::istream>{new std::ifstream(location)};
}
}
