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

/**
 * @file SpecialPaths.hpp
 *
 * Access to platform specific special paths.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_SPECIAL_PATHS_HPP_
#define _OUSIA_SPECIAL_PATHS_HPP_

#include <string>

namespace ousia {

/**
 * Class containing static functions providing access to special paths.
 */
class SpecialPaths {
public:
	/**
	 * Returns the home directory of the current user or an empty string if the
	 * functionality is not available.
	 *
	 * @return path to the home director or empty string on failure.
	 */
	static std::string getHomeDir();

	/**
	 * Returns the global application data directory (e.g. /usr/share/ousia on
	 * unix).
	 *
	 * @return path to the installation data directory, empty string on failure.
	 */
	static std::string getGlobalDataDir();

	/**
	 * Returns the local application data directory (e.g.
	 * /home/usre/.local/share/ousia on unix).
	 *
	 * @return path to the local data directory, empty string on failure.
	 */
	static std::string getLocalDataDir();

	/**
	 * Returns the path to the application data when running a debug build.
	 *
	 * @return path to the application data when running a debug build. Returns
	 * an empty string otherwise.
	 */
	static std::string getDebugDataDir();

	/**
	 * Returns the path to the test data when running a debug build with enabled
	 * tests.
	 *
	 * @return path to the test data when running a debug build, Returns an
	 * empty string otherwise.
	 */
	static std::string getDebugTestdataDir();
};
}

#endif /* _OUSIA_SPECIAL_PATHS_HPP_ */

