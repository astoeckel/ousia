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

#include "Node.hpp"

namespace ousia {

/* Class Node */

void Node::setName(std::string name)
{
	// Call the name change event
	NameChangeEvent ev{this->name, name};
	triggerEvent(ev);

	// Set the new name
	this->name = std::move(name);
}

void Node::path(std::vector<std::string> &p) const
{
	if (!isRoot()) {
		parent->path(p);
	}
	p.push_back(name);
}

std::vector<std::string> Node::path() const
{
	std::vector<std::string> res;
	path(res);
	return res;
}

void Node::doResolve(std::vector<Rooted<Node>> &res,
                     const std::vector<std::string> &path, Filter filter,
                     void *filterData, unsigned idx, VisitorSet &visited)
{
	// Do nothing in the default implementation
}

int Node::resolve(std::vector<Rooted<Node>> &res,
                  const std::vector<std::string> &path, Filter filter,
                  void *filterData, unsigned idx, VisitorSet &visited,
                  const std::string *alias)
{
	// Abort if this node was already visited for this path index
	std::pair<const Node *, int> recKey = std::make_pair(this, idx);
	if (visited.find(recKey) != visited.end()) {
		return res.size();
	}
	visited.insert(recKey);

	// Check whether the we can continue the path
	if (path[idx] == name || (alias && *alias == name)) {
		// If we have reached the end of the path and the node is successfully
		// tested by the filter function, add it to the result. Otherwise
		// continue searching along the path
		if (idx + 1 == path.size()) {
			if (!filter || filter(this, filterData)) {
				res.push_back(this);
			}
		} else {
			doResolve(res, path, filter, filterData, idx + 1, visited);
		}
	}

	// Restart the search from here in order to find all possible nodes that can
	// be matched to the given path
	doResolve(res, path, filter, filterData, 0, visited);

	return res.size();
}

std::vector<Rooted<Node>> Node::resolve(const std::vector<std::string> &path,
                                        Filter filter = nullptr,
                                        void *filterData = nullptr)
{
	std::vector<Rooted<Node>> res;
	VisitorSet visited;
	resolve(res, path, filter, filterData, 0, visited, nullptr);
	return res;
}

int Node::registerEventHandler(EventType type, EventHandler handler,
                               Handle<Managed> owner,
                               bool includeChildren)
{
	const int id = handlerIdCounter++;
	handlers.insert(std::make_pair(
	    type,
	    EventHandlerDescriptor{id, handler, owner, this, includeChildren}));
	return id;
}

bool Node::unregisterEventHandler(int id) {
	for (auto it = handlers.begin(); it != handlers.end(); it++) {
		if (it->second.id == id) {
			handlers.erase(it);
			return true;
		}
	}
	return false;
}

bool Node::triggerEvent(Event &event, bool fromChild) {
	bool res = false;
	// Iterate over all event handlers
	const auto range = handlers.equal_range(event.type);
	for (auto it = range.first; it != range.second; it++) {
		// Fetch a reference to the descriptor, check whether it should be
		// called for bubbled events
		EventHandlerDescriptor descr = it->second;
		if (!fromChild || descr.includeChildren) {
			descr.handler(event, descr.owner);
			res = true;
		}
	}

	// If possible, let the event bubble up to the parent node
	if (event.canBubble() && !parent.isNull()) {
		res = parent->triggerEvent(event, true) | res;
	}
	return res;
}

}
