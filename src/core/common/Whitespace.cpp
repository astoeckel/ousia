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

#include "Whitespace.hpp"
#include "WhitespaceHandler.hpp"

namespace ousia {

std::string Utils::trim(const std::string &s)
{
	std::pair<size_t, size_t> bounds = trim(s, Utils::isWhitespace);
	return s.substr(bounds.first, bounds.second - bounds.first);
}

std::string Utils::collapse(const std::string &s)
{
	CollapsingWhitespaceHandler h;
	appendToWhitespaceHandler(h, s, 0);
	return h.toString();
}

}

