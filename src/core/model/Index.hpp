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

/**
 * @file Index.hpp
 *
 * Contains the Index class which facilitates resolution of Node names.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_INDEX_HPP_
#define _OUSIA_INDEX_HPP_

#include <string>
#include <unordered_map>

#include <core/managed/Managed.hpp>

namespace ousia {

// Forward declarations
class Node;

/**
 * The Index class is a listener for NodeContainer instances and automatically 
 * creates a dictionary for looking up Node instances by name. The Index class
 * automatically maintains consistency when nodes are added or removed from the
 * container or the Nodes themself are renamed. It is not a replacement for the  
 * NodeContainer or ManagedContainer classes, but is used as Listener class 
 * inside these classes.
 */
class Index {
private:
	/**
	 * Map from names to the corresponding nodes.
	 */
	std::unordered_map<std::string, Node *> index;

	/**
	 * Adds a reference to the given node with the given name to the index.
	 * Empty names are ignored.
	 *
	 * @param name is the name under which the element should be found in the
	 * index. Elements with empty name are ignored. The name must be unique.
	 * @param node is a reference to the node that should be associated with the
	 * given name.
	 */
	void addToIndex(const std::string &name, const Handle<Node> &node);

	/**
	 * Deletes a reference to the given node from the index.
	 *
	 * @param name is the name under which the element should be found in the
	 * index. Elements with empty name are ignored. Does nothing if no entry
	 * with the given name exists.
	 * @param node is a reference to the node that should be associated with the
	 * given name.
	 */
	void deleteFromIndex(const std::string &name, const Handle<Node> &node);

	/**
	 * Called automatically whenever the name of a node in the index changes.
	 *
	 * @param ev contain the NameChangeEvent data.
	 * @param owner is the Managed object that owns the node for which the event
	 * handler was registered.
	 * @param data contains the reference to the Index instance.
	 */
	static void indexHandleNameChange(const Event &ev, Managed *owner,
	                                  void *data);

public:
	/**
	 * Adds an element to the index. Called by the ManagedContainer class.
	 *
	 * @param val is a reference to the node instance that should be indexed.
	 * @param owner is the Managed object that owns the given node.
	 */
	void addElement(Handle<Node> node, Managed *owner);

	/**
	 * Removes an element from the index. Called by the ManagedContainer class.
	 *
	 * @param val is a reference to the node instance that should be indexed.
	 * @param owner is the Managed object that owns the given node.
	 * @param fromDestructor set to true, if the function is called from the
	 * ManagedContainer destructor and the node may no longer be valid.
	 */
	void deleteElement(Handle<Node> node, Managed *owner, bool fromDestructor);

	/**
	 * Resolves the given name to a reference to a node with this name or to
	 * nullptr if such a node does not exist.
	 */
	Rooted<Node> resolve(const std::string &name) const;
};

}

#endif /* _OUSIA_INDEX_HPP_ */

