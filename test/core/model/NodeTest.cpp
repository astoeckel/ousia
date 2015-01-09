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

#include <gtest/gtest.h>

#include <core/managed/Managed.hpp>
#include <core/model/Node.hpp>

namespace ousia {

class TestNode : public Node {
private:
	std::vector<Owned<Node>> children;

protected:
	void continueResolve(ResolutionState &state) override
	{
		continueResolveComposita(children, state);
	}

public:
	using Node::Node;

	Rooted<TestNode> addChild(Handle<TestNode> node)
	{
		Owned<TestNode> nd = acquire(node);
		children.push_back(nd);
		return nd;
	}
};

namespace RttiTypes {
const Rtti<ousia::TestNode> TestNode =
    RttiBuilder("TestNode").parent(RttiTypes::Node).composedOf(&TestNode);
}

TEST(Node, isRoot)
{
	Manager mgr;
	Rooted<TestNode> n1{new TestNode(mgr)};
	Rooted<TestNode> n2{new TestNode(mgr)};
	Rooted<TestNode> n3{new TestNode(mgr, n2)};
	ASSERT_TRUE(n1->isRoot());
	ASSERT_TRUE(n2->isRoot());
	ASSERT_FALSE(n3->isRoot());

	n2->setParent(n1);
	ASSERT_TRUE(n1->isRoot());
	ASSERT_FALSE(n2->isRoot());
	ASSERT_FALSE(n3->isRoot());
}

TEST(Node, simpleResolve)
{
	Manager mgr;
	Rooted<TestNode> root{new TestNode(mgr, "root")};
	Rooted<TestNode> child1 = root->addChild(new TestNode(mgr, "child1"));
	Rooted<TestNode> child11 = child1->addChild(new TestNode(mgr, "child11"));

	std::vector<ResolutionResult> res;
	res = root->resolve(std::vector<std::string>{"root", "child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(1U, res.size());
	ASSERT_TRUE(child11 == res[0].node);

	res = root->resolve(std::vector<std::string>{"child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(1U, res.size());
	ASSERT_TRUE(child11 == res[0].node);

	res =
	    root->resolve(std::vector<std::string>{"child11"}, RttiTypes::TestNode);
	ASSERT_EQ(1U, res.size());
	ASSERT_TRUE(child11 == res[0].node);
}

}
