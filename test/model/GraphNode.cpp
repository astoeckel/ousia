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

#include <gtest/gtest.h>

#include <model/GraphNode.hpp>

namespace ousia {
namespace model {

// Make the protected constructor of GraphNode available
class TestGraphNode : public GraphNode {

public:
	TestGraphNode(GraphNodeType type, std::shared_ptr<GraphNode> parent,
			const std::string &name) :
		GraphNode(type, parent, name)
	{
		// Do nothing here
	}

};

TEST(GraphNodeTest, FullyQuallifiedNameTest)
{
	std::shared_ptr<GraphNode> nd1{new TestGraphNode(
			GraphNodeType::Domain, nullptr, "node1")};
	std::shared_ptr<GraphNode> nd2{new TestGraphNode(
			GraphNodeType::Domain, nd1, "node2")};

	ASSERT_STREQ("node1.node2", nd2->getFullyQualifiedName().c_str());
}

}
}

