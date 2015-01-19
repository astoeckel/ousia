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

#include "FileLocator.hpp"

#include <boost/filesystem.hpp>

#include <fstream>

namespace ousia {

void FileLocator::addSearchPath(const std::string &path,
                                std::set<ResourceType> types)
{
	// Canonicalize the given path
	std::string canonicalPath =
	    boost::filesystem::canonical(path).generic_string();

	// Insert the path for all given types.
	for (auto &type : types) {
		auto it = searchPaths.find(type);
		if (it != searchPaths.end()) {
			it->second.push_back(canonicalPath);
		} else {
			searchPaths.insert({type, {canonicalPath}});
		}
	}
}

bool FileLocator::doLocate(Resource &resource, const std::string &path,
                           const ResourceType type,
                           const std::string &relativeTo) const
{
	boost::filesystem::path base(relativeTo);
	if (boost::filesystem::exists(base)) {
		// Look if 'relativeTo' is a directory already.
		if (!boost::filesystem::is_directory(base)) {
			// If not we use the parent directory.
			base = base.parent_path();
		}

		// Use the / operator to append the path.
		base /= path;

		// If we already found a fitting resource there, use that.
		if (boost::filesystem::exists(base)) {
			std::string location =
			    boost::filesystem::canonical(base).generic_string();
			resource = Resource(true, *this, type, location);
			return true;
		}
	}

	// Otherwise look in the search paths.
	auto it = searchPaths.find(type);
	if (it != searchPaths.end()) {
		for (boost::filesystem::path p : it->second) {
			p /= path;
			if (boost::filesystem::exists(p)) {
				std::string location =
				    boost::filesystem::canonical(p).generic_string();
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
