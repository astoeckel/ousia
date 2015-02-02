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
 * @file RootNode.hpp
 *
 * Defines a base class for Nodes that may act as a root and thus are capable of
 * importing other nodes.
 *
 * @author Andreas Stöckel (astoecke@techfk.uni-bielefeld.de)
 */

#ifndef _OUSIA_ROOT_NODE_HPP_
#define _OUSIA_ROOT_NODE_HPP_

#include "Node.hpp"

namespace ousia {

/**
 * The RootNode class represents a Node that may be a Root node (such as
 * Documents, Typesystems and Domains). Root nodes have the property, that the
 * allow importing/referencing other Nodes.
 */
class RootNode : public Node {
protected:
	/**
	 * Imports the given node. The node was checked to be one of the supported
	 * types.
	 *
	 * @param node is the node that should be imported.
	 */
	virtual void doImport(Handle<Node> node) = 0;

	/**
	 * Should return a set of types that can be imported by this Node.
	 *
	 * @return a set of Node types that may be imported by this Node.
	 */
	virtual RttiSet doGetImportTypes() = 0;

public:
	using Node::Node;

	/**
	 * Tries to import the given node. Throws an exception if the node is not of
	 * the supported types.
	 *
	 * @param node is the node that should be imported.
	 */
	void import(Handle<Node> node);

	/**
	 * Returns a set of types that can be imported by this Node.
	 *
	 * @return a set of types that can be imported.
	 */
	RttiSet getImportTypes();
};

namespace RttiTypes {
/**
 * Rtti descriptor for the RootNode class.
 */
extern const Rtti RootNode;
}
}

#endif /* _OUSIA_ROOT_NODE_HPP_ */

