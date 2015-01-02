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

#ifndef _OUSIA_NODE_HPP_
#define _OUSIA_NODE_HPP_

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <unordered_set>

#include <core/common/Rtti.hpp>
#include <core/managed/Managed.hpp>
#include <core/managed/ManagedContainer.hpp>

#include "Index.hpp"

namespace ousia {

/**
 * The Node class builds the base class for any Node within the DOM graph. A
 * node may either be a descriptive node (such as a domain description etc.)
 * or a document element. Each node is identified by acharacteristic name and
 * a parent element. Note that the node name is not required to be unique. Nodes
 * without parent are considered root nodes.
 */
class Node : public Managed {
public:
	/**
	 * The Filter function is used when resolving names to Node instances. The
	 * filter tests whether the given node meets the requirements for inclusion
	 * in the result list.
	 *
	 * @param managed is the managed which should be tested.
	 * @param data is user-defined data passed to the filter.
	 * @return true if the node should be included in the result set, false
	 * otherwise.
	 */
	using Filter = bool (*)(Handle<Managed> managed, void *data);

	/**
	 * Hash functional used to convert pairs of nodes and int to hashes which
	 * can be used within a unordered_set.
	 */
	struct VisitorHash {
		size_t operator()(const std::pair<const Node *, int> &p) const
		{
			const std::hash<const Node *> nodeHash;
			const std::hash<int> intHash;
			return nodeHash(p.first) + 37 * intHash(p.second);
		}
	};

	/**
	 * Alias for the VisitorSet class which represents all nodes which have been
	 * visited in the name resolution process. The map stores pairs of node
	 * pointers and integers, indicating for which path start id the node has
	 * already been visited.
	 */
	using VisitorSet =
	    std::unordered_set<std::pair<const Node *, int>, VisitorHash>;

private:
	/**
	 * Name of the node. As names are always looked up relative to a node,
	 * names are not required to be unique.
	 */
	std::string name;

	/**
	 * Reference to a parent node instace.
	 */
	Owned<Node> parent;

	/**
	 * Private version of the "path" function used to construct the path. Calls
	 * the path function of the parent node and adds the own name to the given
	 * vector.
	 *
	 * @param p is the list the path should be constructed in.
	 */
	void path(std::vector<std::string> &p) const;

protected:
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
	 * @param filter is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The filter function can be used to restrict the
	 * type of matched functions.
	 * @param filterData is user-defined data that should be passed to the
	 * filter.
	 */
	virtual void doResolve(std::vector<Rooted<Managed>> &res,
	                       const std::vector<std::string> &path, Filter filter,
	                       void *filterData, unsigned idx, VisitorSet &visited);

public:
	/**
	 * Initializes the node with empty name and parent.
	 *
	 * @param mgr is a reference to the Manager instace the node belongs to.
	 */
	Node(Manager &mgr, Handle<Node> parent = nullptr)
	    : Managed(mgr), parent(acquire(parent))
	{
	}

	/**
	 * Constructs a new node with the given name and the given parent element.
	 *
	 * @param mgr is a reference to the Manager instace the node belongs to.
	 * @param name is the name of the Node.
	 * @param parent is a handle pointing at the parent node.
	 */
	Node(Manager &mgr, std::string name, Handle<Node> parent = nullptr)
	    : Managed(mgr), name(name), parent(acquire(parent))
	{
	}

	/**
	 * Sets the name of the node to the given name. Note: The name set here may
	 * be invalid (contain spaces, colons or other special characters). However,
	 * in this case the node will not be reachable as reference from a input
	 * document. This behaviour allows for gracefully degradation in error
	 * cases.
	 *
	 * @param name is the name that should be assigned to the node.
	 */
	void setName(std::string name);

	/**
	 * Returns the name of the node.
	 */
	std::string getName() const { return name; }

	/**
	 * Returns a reference to the name of the node.
	 */
	const std::string &getNameRef() const { return name; }

	/**
	 * Specifies whether the node has a name, e.g. whether the current name is
	 * not empty.
	 *
	 * @return true if the name of this node is not empty, false otherwise.
	 */
	bool hasName() const { return !name.empty(); }

	/**
	 * Sets the parent node.
	 *
	 * @param parent is a Handle to the parent node.
	 */
	void setParent(Handle<Node> parent) { this->parent = acquire(parent); }

	/**
	 * Returns a handle to the parent node of the Node instance.
	 *
	 * @return a handle to the root node.
	 */
	Rooted<Managed> getParent() const { return parent; }

	/**
	 * Returns true, if the node does not have a parent. Root nodes may either
	 * be the root element of the complete DOM tree
	 *
	 * @return true if the node is a root node (has no parent) or false if the
	 * node is no root node (has a parent).
	 */
	bool isRoot() const { return parent.isNull(); }

	/**
	 * Returns the vector containing the complete path to this node (including
	 * the name of the parent nodes).
	 *
	 * @return a vector containing the path (starting with the root node) to
	 * this node as a list of names.
	 */
	std::vector<std::string> path() const;

	/**
	 * Function which resolves a name path to a list of possible nodes.
	 *
	 * @param res is the result list containing all possible nodes matching the
	 * name specified in the path.
	 * @param path is a list specifying a path of node names which is meant to
	 * specify a certain named node.
	 * @param filter is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The filter function can be used to restrict the
	 * type of matched functions.
	 * @param filterData is user-defined data that should be passed to the
	 * filter.
	 * @param idx is the current index in the path.
	 * @param visited is a map which is used to prevent unwanted recursion.
	 * @param alias is a pointer at a string which contains an alternative name
	 * for this node. If nullptr is given, not such alternative name is
	 * provided.
	 * @return the number of elements in the result list.
	 */
	int resolve(std::vector<Rooted<Managed>> &res,
	            const std::vector<std::string> &path, Filter filter,
	            void *filterData, unsigned idx, VisitorSet &visited,
	            const std::string *alias);

	/**
	 * Function which resolves a name path to a list of possible nodes starting
	 * from this node.
	 *
	 * @param path is a list specifying a path of node names meant to specify a
	 * certain named node.
	 * @param filter is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The filter function can e.g. be used to restrict
	 * the type of matched functions.
	 * @param filterData is user-defined data that should be passed to the
	 * filter.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Managed>> resolve(const std::vector<std::string> &path,
	                                     Filter filter, void *filterData);

	/**
	 * Function which resolves a name path to a list of possible nodes starting
	 * from this node.
	 *
	 * @param path is a list specifying a path of node names meant to specify a
	 * certain named node.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Managed>> resolve(const std::vector<std::string> &path)
	{
		return resolve(path, nullptr, nullptr);
	}

	/**
	 * Function which resolves a single name to a list of possible nodes
	 * starting from this node.
	 *
	 * @param name is the name which should be resolved.
	 * @param filter is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The filter function can e.g. be used to restrict
	 * the type of matched functions.
	 * @param filterData is user-defined data that should be passed to the
	 * filter.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Managed>> resolve(const char *, Filter filter,
	                                     void *filterData)
	{
		return resolve(std::vector<std::string>{name}, filter, filterData);
	}

	/**
	 * Function which resolves a single name to a list of possible nodes
	 * starting from this node.
	 *
	 * @param name is the name which should be resolved.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Managed>> resolve(const std::string &name)
	{
		return resolve(std::vector<std::string>{name}, nullptr, nullptr);
	}
};

// TODO: Use a different listener here for updating name maps

template <class T, class Listener = Index>
class NodeVector
    : public ManagedGenericList<T, std::vector<Handle<T>>,
                                ListAccessor<Handle<T>>, Listener> {
public:
	using Base = ManagedGenericList<T, std::vector<Handle<T>>,
	                                ListAccessor<Handle<T>>, Listener>;
	using Base::ManagedGenericList;

	Index& getIndex() { return this->listener;}
};

template <class K, class T, class Listener = Index>
class NodeMap
    : public ManagedGenericMap<K, T, std::map<K, Handle<T>>,
                               MapAccessor<std::pair<K, Handle<T>>>, Listener> {
public:
	using Base =
	    ManagedGenericMap<K, T, std::map<K, Handle<T>>,
	                      MapAccessor<std::pair<K, Handle<T>>>, Listener>;
	using Base::ManagedGenericMap;

	Index& getIndex() { return this->listener;}
};

namespace RttiTypes {
/**
 * Typeinformation for the base "Node" class.
 */
extern const Rtti<Node> Node;
}
}

#endif /* _OUSIA_NODE_HPP_ */

