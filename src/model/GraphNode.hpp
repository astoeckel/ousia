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

#ifndef _GRAPH_NODE_HPP_
#define _GRAPH_NODE_HPP_

#include <string>
#include <memory>

namespace ousia {

class GraphNode {

private:
	std::string name;
	std::shared_ptr<GraphNode> parent;

public:
	GraphNode();

	GraphNode(std::shared_ptr<GraphNode> parent);

	GraphNode(const std::string &name, std::shared_ptr<GraphNode> parent = nullptr);

	const std::string& getName()
	{
		return name;
	}

	const std::string getFullyQualifiedName();

	void setName(const std::string &name)
	{
		this->name = name;
	}

	std::shared_ptr<GraphNode> getParent()
	{
		return std::shared_ptr<GraphNode>(parent);
	}

	void setParent(std::shared_ptr<GraphNode> parent)
	{
		this->parent = parent;
	}


};

}

#endif /* _GRAPH_NODE_HPP_ */

