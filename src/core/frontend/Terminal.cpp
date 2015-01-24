/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#include "Terminal.hpp"

namespace ousia {

std::string Terminal::color(int color, bool bright) const
{
	if (!useColor) {
		return std::string{};
	}
	std::stringstream ss;
	ss << "\x1b[";
	if (bright) {
		ss << "1;";
	}
	ss << color << "m";
	return ss.str();
}

std::string Terminal::bright() const
{
	if (!useColor) {
		return std::string{};
	}
	return "\x1b[1m";
}

std::string Terminal::italic() const
{
	if (!useColor) {
		return std::string{};
	}
	return "\x1b[3m";
}

std::string Terminal::underline() const
{
	if (!useColor) {
		return std::string{};
	}
	return "\x1b[4m";
}

std::string Terminal::reset() const
{
	if (!useColor) {
		return std::string{};
	}
	return "\x1b[0m";
}

}

