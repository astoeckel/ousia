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
		return RttiTypes::None;
	} else {
		return *(it->second);
	}
}

/* Class RttiBase */

void RttiBase::initialize() const
{
	// Only run this function exactly once -- directly set the initialized flag
	// to prevent unwanted recursion
	if (!initialized) {
		initialized = true;

		// Insert the parent types of the parent types and the composite types
		// of the parents
		{
			std::unordered_set<const RttiBase *> origParents = parents;
			for (const RttiBase *parent : origParents) {
				parent->initialize();
				parents.insert(parent->parents.begin(), parent->parents.end());
			}
			for (const RttiBase *parent : parents) {
				parent->initialize();
				compositeTypes.insert(parent->compositeTypes.begin(),
				                       parent->compositeTypes.end());
			}
			parents.insert(this);
		}

		// Insert the composite types of the composite types and the parents
		// of each composite type
		{
			std::unordered_set<const RttiBase *> origCompositeTypes =
			    compositeTypes;
			for (const RttiBase *compositeType : origCompositeTypes) {
				compositeType->initialize();
				compositeTypes.insert(compositeType->compositeTypes.begin(),
				                       compositeType->compositeTypes.end());
				compositeTypes.insert(compositeType->parents.begin(),
				                       compositeType->parents.end());
			}
		}
	}
}

bool RttiBase::isa(const RttiBase &other) const
{
	initialize();
	return parents.count(&other) > 0;
}

bool RttiBase::composedOf(const RttiBase &other) const
{
	initialize();
	return compositeTypes.count(&other) > 0;
}

/* Constant initialization */

const RttiBase RttiTypes::None;
}

