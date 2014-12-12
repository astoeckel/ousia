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

#include <algorithm>
#include <limits>

#include "Utils.hpp"

namespace ousia {

std::string Utils::trim(const std::string &s)
{
	size_t firstNonWhitespace = std::numeric_limits<size_t>::max();
	size_t lastNonWhitespace = 0;
	for (size_t i = 0; i < s.size(); i++) {
		if (!isWhitespace(s[i])) {
			firstNonWhitespace = std::min(i, firstNonWhitespace);
			lastNonWhitespace = std::max(i, lastNonWhitespace);
		}
	}

	if (firstNonWhitespace < lastNonWhitespace) {
		return s.substr(firstNonWhitespace,
		                lastNonWhitespace - firstNonWhitespace + 1);
	}
	return std::string{};
}

bool Utils::isIdentifier(const std::string &name)
{
	bool first = true;
	for (char c : name) {
		if (first && !(isAlphabetic(c) || c == '_')) {
			return false;
		}
		if (!first && !(isAlphanumeric(c) || c == '_' || c == '-')) {
			return false;
		}
		first = false;
	}
	return true;
}
}

