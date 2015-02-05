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
#include <cctype>
#include <limits>
#include <string>

#include "Utils.hpp"

namespace ousia {

std::string Utils::trim(const std::string &s)
{
	std::pair<size_t, size_t> bounds = trim(s, Utils::isWhitespace);
	return s.substr(bounds.first, bounds.second - bounds.first);
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

bool Utils::hasNonWhitepaceChar(const std::string &s)
{
	for (char c : s) {
		if (!isWhitespace(c)) {
			return true;
		}
	}
	return false;
}

std::vector<std::string> Utils::split(const std::string &s, char delim)
{
	std::vector<std::string> res;
	const size_t totalLen = s.size();
	size_t start = 0;
	size_t len = 0;
	for (size_t i = 0; i <= totalLen; i++) {
		if (i == totalLen || s[i] == delim) {
			res.push_back(s.substr(start, len));
			start = i + 1;
			len = 0;
		} else {
			len++;
		}
	}
	return res;
}

std::string Utils::toLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	return s;
}

std::string Utils::extractFileExtension(const std::string &filename)
{
	size_t n = 0;
	for (ssize_t i = filename.size() - 1; i >= 0; i--) {
		if (filename[i] == '/' || filename[i] == '\\') {
			return std::string{};
		}
		if (filename[i] == '.') {
			return toLower(filename.substr(i + 1, n));
		}
		n++;
	}
	return std::string{};
}
}

