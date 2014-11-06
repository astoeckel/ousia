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

#include <string>
#include <unordered_map>
#include <vector>

#include "Node.hpp"

namespace ousia {
namespace dom {

/* Class NamedNode */

void NamedNode::doResolve(const std::vector<Rooted<Node>> &res,
                          const std::vector<std::string> &path,
                          ResolutionCallback callback, void *callbackData,
                          int idx, const VisitorMap &visited)
{
	// Do nothing in the default implementation
}

void NamedNode::resolve(const std::vector<Rooted<Node>> &res,
                        const std::vector<std::string> &path,
                        ResolutionCallback *callback, void *callbackData,
                        int idx, const VisitorMap &visited)
{
	// Abort if this node was already visited for this path index
	std::pair<Node *, int> recKey = std::make_pair(this, idx);
	if (visited.find(recKey) != visited.end()) {
		return res.size();
	}
	visited.insert(recKey);

	// Check whether the we can continue the path
	if (path[idx] == name) {
		// If we have reached the end of the path and the node is successfully
		// tested by the callback function, add it to the result. Otherwise
		// continue searching along the path
		if (idx == path.size() - 1) {
			if (!callback || callback(this, callbackData)) {
				res.push_back(this);
			}
		} else {
			doResolve(res, path, callback, callbackData, idx + 1, visited);
		}
	}

	// Restart the search from here in order to find all possible nodes that can
	// be matched to the given path
	doResolve(res, path, callback, callbackData, 0, visited);
}

std::vector<Rooted<Node>> NamedNode::resolve(
    const std::vector<std::string> &path, ResolutionCallback callback,
    void *callbackData)
{
	std::vector<Rooted<Node>> res;
	VisitorMap visited;
	resolve(res, path, callback, callbackData, 0, visited);
	return res;
}
}
}

