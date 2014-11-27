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

/* Class EnumType */

EnumType::EnumType(Manager &mgr, const std::set<std::string> &names) :
	Type(mgr, false, true)
{
	int value = 0;
	for (const auto &name: names) {
		values.insert(std::make_pair(name, value++))
	}
}

int EnumType::valueOf(const std::string &name)
{
	auto it = values.find(name);
	if (it != values.end()) {
		return it->second;
	}
	return -1;
}

std::string EnumType::toString(int value) {
	for (const auto &p : values) {
		if (p->second == value) {
			return p->first;
		}
	}
	return std::string{};
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

