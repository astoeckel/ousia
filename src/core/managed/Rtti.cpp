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

#include "Rtti.hpp"

namespace ousia {

/* Class RttiStore */

std::unordered_map<std::type_index, const RttiBase *> &RttiStore::table()
{
	static std::unordered_map<std::type_index, const RttiBase *> table;
	return table;
}

void RttiStore::store(const std::type_info &native, const RttiBase *rtti)
{
	table().emplace(std::type_index{native}, rtti);
}

const RttiBase &RttiStore::lookup(const std::type_info &native)
{
	const auto &tbl = table();
	auto it = tbl.find(std::type_index{native});
	if (it == tbl.end()) {
		return RttiBase::None;
	} else {
		return *(it->second);
	}
}

/* Class RttiBase */

const RttiBase RttiBase::None;

bool RttiBase::isa(const RttiBase &other) const
{
	if (&other == this) {
		return true;
	}
	for (auto t : parents) {
		if (t->isa(other)) {
			return true;
		}
	}
	return false;
}


}

