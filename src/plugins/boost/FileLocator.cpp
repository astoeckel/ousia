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

#include <fstream>

namespace ousia {

void FileLocator::addSearchPath(const boost::filesystem::path &path,
                                std::set<ResourceLocator::Type> types)
{
	for (auto &type : types) {
		// retrieve the path vector for the given type.
		auto it = searchPaths.find(type);
		if (it != searchPaths.end()) {
			it->second.push_back(path);
		} else {
			std::vector<boost::filesystem::path> v{path};
			searchPaths.insert({type, v});
		}
	}
}

ResourceLocator::Location FileLocator::locate(const std::string &path,
                                              const std::string &relativeTo,
                                              const Type type) const
{
	boost::filesystem::path base(relativeTo);
	if (boost::filesystem::exists(base)) {
		// look if 'relativeTo' is a directory already.
		if (!boost::filesystem::is_directory(base)) {
			// if not we use the parent directory.
			base = base.parent_path();
		}
		// use the / operator to append the path.
		base /= path;
		// if we already found a fitting resource there, use that.
		if (boost::filesystem::exists(base)) {
			std::string location =
			    boost::filesystem::canonical(base).generic_string();
			return ResourceLocator::Location(true, *this, type, location);
		}
	}
	// otherwise look in the search paths.
	auto it = searchPaths.find(type);
	if (it != searchPaths.end()) {
		for (boost::filesystem::path p : it->second) {
			p /= path;
			if (boost::filesystem::exists(p)) {
				std::string location =
				    boost::filesystem::canonical(p).generic_string();
				return ResourceLocator::Location(true, *this, type, location);
			}
		}
	}
	// if we find the resource in none of the search paths we return a location.
	// with the found flag set to false.
	ResourceLocator::Location l(false, *this, type, "");
	return l;
}

std::unique_ptr<std::istream> FileLocator::stream(
    const std::string &location) const
{
	std::unique_ptr<std::istream> ifs{new std::ifstream(location)};
	return std::move(ifs);
}
}
