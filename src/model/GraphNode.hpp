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

#ifndef _OUSIA_MODEL_GRAPH_NODE_HPP_
#define _OUSIA_MODEL_GRAPH_NODE_HPP_

#include <string>
#include <memory>

namespace ousia {
namespace model {

enum class GraphNodeType {
	Domain, Class, Annotation, Structure, ClassCategory, AnnotationCategory
};

class GraphNode {

private:
	GraphNodeType type;
	std::shared_ptr<GraphNode> parent;
	std::string name;

protected:
	GraphNode(GraphNodeType type, std::shared_ptr<GraphNode> parent = nullptr,
			const std::string &name = "");

public:
	const std::string getFullyQualifiedName();

	const std::string& getName()
	{
		return name;
	}

	void setName(const std::string &name)
	{
		this->name = name;
	}

	std::shared_ptr<GraphNode> getParent()
	{
		return parent;
	}

	void setParent(std::shared_ptr<GraphNode> parent)
	{
		this->parent = parent;
	}

	GraphNodeType getType()
	{
		return type;
	}

};

}
}

#endif /* _OUSIA_MODEL_GRAPH_NODE_HPP_ */

