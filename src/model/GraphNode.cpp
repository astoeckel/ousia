/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#include "GraphNode.hpp"

#include <iostream>
#include <sstream>

namespace ousia {
namespace model {

GraphNode::GraphNode(GraphNodeType type, std::shared_ptr<GraphNode> parent,
		const std::string &name) :
	type(type), parent(parent), name(name)
{
	// Do nothing here
}

const std::string GraphNode::getFullyQualifiedName()
{
	if (parent) {
		std::stringstream ss;
		ss << parent->getFullyQualifiedName() << "." << name;
		return ss.str();
	}
	return name;
}

}
}

