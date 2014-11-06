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

#ifndef _OUSIA_DOM_NODE_CLASSES_HPP_
#define _OUSIA_DOM_NODE_CLASSES_HPP_

#include <string>
#include <unordered_map>
#include <vector>

#include "Node.hpp"

namespace ousia {
namespace dom {

/**
 * The ResolutionCallback is used when resolving names to Node instances. The
 * callback tests whether the given node meets the requirements for inclusion
 * in the result list.
 *
 * @param node is the node which should be tested.
 * @param userData is user-defined data which is passed to the callback.
 * @return true if the node should be included in the result set, false
 * otherwise.
 */
typedef bool (*ResolutionCallback)(Handle<Node> node, void *userData);

/**
 * The NamedNode class can be used as a mixin for other node classes which want
 * to have a unique name which is known at construction time (this includes
 * nodes representing domains, classes, types, etc.).
 */
class NamedNode : virtual public Node {
protected:
	struct VisitorHash {
		size_t operator()(const std::pair<Node *, int> &p) const
		{
			return hash<Node *>(p.first) + 37 * hash<int>(p.second);
		}
	};

	using VisitorMap = std::unordered_map<std::pair<Node *, int>, VisitorHash>;

	/**
	 * Function which should be overwritten by derived classes in order to
	 * resolve node names to a list of possible nodes. The implementations of
	 * this function do not need to do anything but call the "resovle" function
	 * of any child instance of NamedNode.
	 *
	 * @param res is the result list containing all possible nodes matching the
	 * name specified in the path.
	 * @param path is a list specifying a path of node names which is meant to
	 * specify a certain named node.
	 * @param idx is the current index in the path.
	 * @param visited is a map which is used to prevent unwanted recursion.
	 * @param callback is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The callback function can be used to restrict the
	 * type of matched functions.
	 * @param callbackData is user-defined data that should be passed to the
	 * callback.
	 */
	virtual void doResolve(const std::vector<Rooted<Node>> &res,
	                       const std::vector<std::string> &path,
	                       ResolutionCallback callback, void *callbackData,
	                       int idx, const VisitorMap &visited);

	/**
	 * Function which resolves a name paht to a list of possible nodes.
	 *
	 * @param res is the result list containing all possible nodes matching the
	 * name specified in the path.
	 * @param path is a list specifying a path of node names which is meant to
	 * specify a certain named node.
	 * @param callback is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The callback function can be used to restrict the
	 * type of matched functions.
	 * @param callbackData is user-defined data that should be passed to the
	 * callback.
	 * @param idx is the current index in the path.
	 * @param visited is a map which is used to prevent unwanted recursion.
	 */
	void resolve(const std::vector<Rooted<Node>> &res,
	             const std::vector<std::string> &path,
	             ResolutionCallback callback, void *callbackData, int idx,
	             const VisitorMap &visited);

public:
	/**
	 * Constructor of the NamedNode class.
	 *
	 * @param name is the name the element should be known under.
	 */
	Named(std::string name) : name{name} {};

	/**
	 * Name of the node.
	 */
	const std::string name;

	/**
	 * Function which resolves a name paht to a list of possible nodes.
	 *
	 * @param path is a list specifying a path of node names which is meant to
	 * specify a certain named node.
	 * @param callback is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The callback function can be used to restrict the
	 * type of matched functions.
	 * @param callbackData is user-defined data that should be passed to the
	 * callback.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Node>> resolve(const std::vector<std::string> &path,
	                                  ResolutionCallback callback = nullptr,
	                                  void *callbackData = nullptr);
};
}
}

#endif /* _OUSIA_DOM_NODE_CLASSES_HPP_ */

