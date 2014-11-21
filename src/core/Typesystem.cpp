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

#include <unordered_map>
#include <sstream>

#include "CodeTokenizer.hpp"
#include "Typesystem.hpp"

namespace ousia {

/* Class StringInstance */


/**
 * Contains a map from escape characters and their corresponding code point.
 */
static const std::unordered_map<char, char> ESCAPE_CHARACTERS_TO_CODEPOINT{
	{'n', '\n'},
	{'r', '\r'},
	{'t', '\t'},
	{'b', '\b'},
	{'f', '\f'},
	{'v', '\v'}
};

static const std::unordered_map<char, char> CODEPOINT_TO_ESCAPE_CHARACTER{
	{'\n', 'n'},
	{'\r', 'r'},
	{'\t', 't'},
	{'\b', 'b'},
	{'\f', 'f'},
	{'\v', 'v'}
};

static const char MIN_CONTROL_CHARACTER = 0x37;

std::string StringInstance::toString()
{
	std::stringstream ss;
	ss << '"';
	for (char c: value) {
		if (c == '"') {
			ss << '\\';
		} else if (c == '\\') {
			ss << "\\\\";
		} else if (c <= MIN_CONTROL_CHARACTER) {
			const auto it = CODEPOINT_TO_ESCAPE_CHARACTER.find(c);
			if (it != CODEPOINT_TO_ESCAPE_CHARACTER.cend()) {
				ss << '\\' << it->second;
			} else {
				ss << c;
			}
		}
		ss << c;
	}
	ss << '"';
	return ss.str();
}

/* Class StringType */

Rooted<TypeInstance> StringType::create()
{
	return new StringInstance(getManager(), this, "");
}

Rooted<TypeInstance> StringType::parse(const std::string &str)
{
	/*std::stringstream ss;
	int state = 0;*/
	return nullptr;
}

/* Class Typesystem */

void Typesystem::doResolve(std::vector<Rooted<Managed>> &res,
	               const std::vector<std::string> &path, Filter filter,
	               void *filterData, unsigned idx,
	               VisitorSet &visited)
{
	// Try to resolve the given name as a type
	for (auto n: types) {
		n->resolve(res, path, filter, filterData, idx, visited, nullptr);
	}

	// Try to resolve the given name as a constant
	const auto it = constants.find(path[idx]);
	if (it != constants.cend()) {
		if (filter && filter(it->second, filterData)) {
			res.push_back(it->second);
		}
	}
}

}

