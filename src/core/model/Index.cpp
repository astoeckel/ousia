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

#include "Index.hpp"
#include "Node.hpp"

namespace ousia {

/* Class Index */

void Index::indexHandleNameChange(const Event &ev, Managed *owner, void *data)
{
	// Fetch the referenced node, the index instance and the NameChangeEvent
	const NameChangeEvent &nameEv = static_cast<const NameChangeEvent &>(ev);
	Handle<Node> node = static_cast<Node *>(ev.sender);
	Index &index = *(static_cast<Index *>(data));

	// Re-add the node to the index
	index.deleteFromIndex(nameEv.oldName, node);
	index.addToIndex(nameEv.newName, node);
}

void Index::addToIndex(const std::string &name, const Handle<Node> &node)
{
	if (!name.empty()) {
		index.emplace(name, node.get());
	}
}

void Index::deleteFromIndex(const std::string &name, const Handle<Node> &node)
{
	if (!name.empty()) {
		auto it = index.find(name);
		if (it != index.end() && it->second == node) {
			index.erase(it);
		}
	}
}

void Index::addElement(Handle<Node> node, Managed *owner)
{
	addToIndex(node->getName(), node);
	node->registerEvent(EventType::NAME_CHANGE, Index::indexHandleNameChange,
	                    owner, this);
}

void Index::deleteElement(Handle<Node> node, Managed *owner,
                          bool fromDestructor)
{
	if (!fromDestructor) {
		deleteFromIndex(node->getName(), node);
		node->unregisterEvent(EventType::NAME_CHANGE,
		                      Index::indexHandleNameChange, owner, this);
	} else if (owner) {
		owner->getManager().unregisterEvent(node.get(), EventType::NAME_CHANGE,
		                                    Index::indexHandleNameChange, owner,
		                                    this);
	}
}

Rooted<Node> Index::resolve(const std::string &name) const
{
	auto it = index.find(name);
	if (it != index.end()) {
		return it->second;
	}
	return nullptr;
}

}
