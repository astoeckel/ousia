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

#include <cassert>
#include <queue>

#include <core/common/Rtti.hpp>

#include "Managed.hpp"
#include "ManagedContainer.hpp"

namespace ousia {

/* Class Managed */

void Managed::storeData(const std::string &key, Handle<Managed> h) {
	mgr.storeData(this, key, h.get());
}

bool Managed::hasDataKey(const std::string &key)
{
	return mgr.readData(this, key) != nullptr;
}

Rooted<Managed> Managed::readData(const std::string &key) {
	return mgr.readData(this, key);
}

std::map<std::string, Rooted<Managed>> Managed::readData() {
	auto map = mgr.readData(this);
	std::map<std::string, Rooted<Managed>> res;
	for (auto e : map) {
		res.emplace(e.first, e.second);
	}
	return res;
}

bool Managed::deleteData(const std::string &key) {
	return mgr.deleteData(this, key);
}

EventId Managed::registerEvent(EventType type, EventHandler handler,
                      Handle<Managed> owner)
{
	return mgr.registerEvent(this, type, handler, owner.get());
}

bool Managed::unregisterEvent(EventId id)
{
	return mgr.unregisterEvent(this, id);
}

bool Managed::triggerEvent(Event &data)
{
	return mgr.triggerEvent(this, data);
}

const RttiBase &Managed::type() const
{
	return typeOf(*this);
}

bool Managed::isa(const RttiBase &t) const { return type().isa(t); }

}
