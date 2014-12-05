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

#include "Exceptions.hpp"

namespace ousia {

/* Class LoggableException */

std::string LoggableException::formatMessage(const std::string &msg,
                                             const std::string &file,
                                             bool fatal, int line, int column)
{
	std::stringstream ss;
	ss << "error ";
	if (!file.empty()) {
		ss << "while processing \"" << file << "\" ";
	}
	if (line >= 0) {
		ss << "at line " << line << ", ";
		if (column >= 0) {
			ss << "column " << column << " ";
		}
	}
	ss << "with message: " << msg;
	return ss.str();
}
}

