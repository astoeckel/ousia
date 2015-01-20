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

#include <cstdlib>
#include <boost/filesystem.hpp>

#include <config.h>

#include "SpecialPaths.hpp"

namespace fs = boost::filesystem;

namespace ousia {

std::string SpecialPaths::getHomeDir()
{
	char *home = getenv("HOME");
	if (home) {
		return std::string{home};
	}
	return std::string{};
}

std::string SpecialPaths::getGlobalDataDir()
{
	return OUSIA_INSTALL_DATA_DIR;
}

std::string SpecialPaths::getLocalDataDir()
{
	std::string home = getHomeDir();
	if (!home.empty()) {
		fs::path homePath{home};
		return (homePath / ".local/share/ousia").generic_string();
	}
	return std::string{};
}

std::string SpecialPaths::getDebugDataDir()
{
	fs::path debug{OUSIA_DEBUG_DIR};
	return (debug / "data").generic_string();
}

std::string SpecialPaths::getDebugTestdataDir()
{
	std::string debug{OUSIA_DEBUG_DIR};
	if (debug.empty()) {
		return "./testdata";
	} else {
		return (fs::path{debug} / "testdata").generic_string();
	}
}

}

