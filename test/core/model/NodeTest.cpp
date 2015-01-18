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

#include <core/common/RttiBuilder.hpp>
#include <core/managed/Managed.hpp>
#include <core/model/Node.hpp>

namespace ousia {

class TestNode : public Node {
protected:
	void doResolve(ResolutionState &state) override
	{
		continueResolveComposita(composita, composita.getIndex(), state);
		continueResolveReferences(references, state);
	}

public:
	NodeVector<TestNode> composita;
	NodeVector<TestNode> references;

	TestNode(Manager &mgr, Handle<Node> parent = nullptr)
	    : Node(mgr, parent), composita(this), references(this)
	{
	}

	TestNode(Manager &mgr, std::string name, Handle<Node> parent = nullptr)
	    : Node(mgr, name, parent), composita(this), references(this)
	{
	}

	Rooted<TestNode> addCompositum(Handle<TestNode> n)
	{
		composita.push_back(n);
		return n;
	}

	Rooted<TestNode> addReference(Handle<TestNode> n)
	{
		references.push_back(n);
		return n;
	}
};

namespace RttiTypes {
const Rtti TestNode = RttiBuilder<ousia::TestNode>("TestNode")
                              .parent(&RttiTypes::Node)
                              .composedOf(&TestNode);
}

TEST(Node, isRoot)
{
	Manager mgr{1};
	Rooted<TestNode> n1{new TestNode(mgr)};
	Rooted<TestNode> n2{new TestNode(mgr)};
	Rooted<TestNode> n3{new TestNode(mgr, n2)};
	ASSERT_TRUE(n1->isRoot());
	ASSERT_TRUE(n2->isRoot());
	ASSERT_FALSE(n3->isRoot());
}

TEST(Node, resolveCompositaSimple)
{
	Manager mgr{1};
	Rooted<TestNode> root{new TestNode(mgr, "root")};
	Rooted<TestNode> child1 = root->addCompositum(new TestNode(mgr, "child1"));
	Rooted<TestNode> child11 =
	    child1->addCompositum(new TestNode(mgr, "child11"));

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

TEST(Node, resolveCompositaDouble)
{
	Manager mgr{1};
	Rooted<TestNode> root{new TestNode(mgr, "root")};
	Rooted<TestNode> root2 = root->addCompositum(new TestNode(mgr, "root"));
	Rooted<TestNode> child1 = root2->addCompositum(new TestNode(mgr, "child1"));
	Rooted<TestNode> child11 =
	    child1->addCompositum(new TestNode(mgr, "child11"));

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

TEST(Node, resolveAmbigousComposita)
{
	Manager mgr{1};
	Rooted<TestNode> root{new TestNode(mgr, "root")};
	Rooted<TestNode> a = root->addCompositum(new TestNode(mgr, "a"));
	Rooted<TestNode> b = root->addCompositum(new TestNode(mgr, "b"));
	Rooted<TestNode> child1 = a->addCompositum(new TestNode(mgr, "child1"));
	Rooted<TestNode> child11 =
	    child1->addCompositum(new TestNode(mgr, "child11"));
	Rooted<TestNode> child12 = b->addCompositum(new TestNode(mgr, "child1"));
	Rooted<TestNode> child112 =
	    child12->addCompositum(new TestNode(mgr, "child11"));

	std::vector<ResolutionResult> res;
	res = root->resolve(std::vector<std::string>{"child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(2U, res.size());
	ASSERT_TRUE(child11 == res[0].node || child11 == res[1].node);
	ASSERT_TRUE(child112 == res[0].node || child112 == res[1].node);

	res =
	    root->resolve(std::vector<std::string>{"child11"}, RttiTypes::TestNode);
	ASSERT_EQ(2U, res.size());
	ASSERT_TRUE(child11 == res[0].node || child11 == res[1].node);
	ASSERT_TRUE(child112 == res[0].node || child112 == res[1].node);
}

TEST(Node, resolveReferences)
{
	Manager mgr{1};
	Rooted<TestNode> root{new TestNode(mgr, "root")};
	Rooted<TestNode> a = root->addReference(new TestNode(mgr, "a"));
	Rooted<TestNode> b = root->addReference(new TestNode(mgr, "b"));
	Rooted<TestNode> child1 = a->addCompositum(new TestNode(mgr, "child1"));
	Rooted<TestNode> child11 =
	    child1->addCompositum(new TestNode(mgr, "child11"));
	Rooted<TestNode> child12 = b->addCompositum(new TestNode(mgr, "child1"));
	Rooted<TestNode> child112 =
	    child12->addCompositum(new TestNode(mgr, "child11"));

	std::vector<ResolutionResult> res;
	res = root->resolve(std::vector<std::string>{"a", "child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(1U, res.size());
	ASSERT_TRUE(child11 == res[0].node);

	res = root->resolve(std::vector<std::string>{"b", "child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(1U, res.size());
	ASSERT_TRUE(child112 == res[0].node);

	res = root->resolve(std::vector<std::string>{"child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(2U, res.size());
	ASSERT_TRUE(child11 == res[0].node || child11 == res[1].node);
	ASSERT_TRUE(child112 == res[0].node || child112 == res[1].node);

	res =
	    root->resolve(std::vector<std::string>{"child11"}, RttiTypes::TestNode);
	ASSERT_EQ(2U, res.size());
	ASSERT_TRUE(child11 == res[0].node || child11 == res[1].node);
	ASSERT_TRUE(child112 == res[0].node || child112 == res[1].node);

	res =
	    root->resolve(std::vector<std::string>{"child1"}, RttiTypes::TestNode);
	ASSERT_EQ(2U, res.size());
	ASSERT_TRUE(child1 == res[0].node || child1 == res[1].node);
	ASSERT_TRUE(child12 == res[0].node || child12 == res[1].node);
}

TEST(Node, resolveReferencesAndComposita)
{
	Manager mgr{1};
	Rooted<TestNode> root{new TestNode(mgr, "root")};
	Rooted<TestNode> a = root->addReference(new TestNode(mgr, "a"));
	Rooted<TestNode> b = root->addReference(new TestNode(mgr, "b"));
	Rooted<TestNode> child1 = a->addCompositum(new TestNode(mgr, "child1"));
	Rooted<TestNode> child11 =
	    child1->addCompositum(new TestNode(mgr, "child11"));
	Rooted<TestNode> child12 = b->addCompositum(new TestNode(mgr, "child1"));
	Rooted<TestNode> child112 =
	    child12->addCompositum(new TestNode(mgr, "child11"));
	Rooted<TestNode> child13 = root->addCompositum(new TestNode(mgr, "child1"));

	std::vector<ResolutionResult> res;
	res = root->resolve(std::vector<std::string>{"a", "child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(1U, res.size());
	ASSERT_TRUE(child11 == res[0].node);

	res = root->resolve(std::vector<std::string>{"b", "child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(1U, res.size());
	ASSERT_TRUE(child112 == res[0].node);

	res = root->resolve(std::vector<std::string>{"child1", "child11"},
	                    RttiTypes::TestNode);
	ASSERT_EQ(2U, res.size());
	ASSERT_TRUE(child11 == res[0].node || child11 == res[1].node);
	ASSERT_TRUE(child112 == res[0].node || child112 == res[1].node);

	res =
	    root->resolve(std::vector<std::string>{"child11"}, RttiTypes::TestNode);
	ASSERT_EQ(2U, res.size());
	ASSERT_TRUE(child11 == res[0].node || child11 == res[1].node);
	ASSERT_TRUE(child112 == res[0].node || child112 == res[1].node);

	// Resolving for "child1" should not descend into the referenced nodes
	res =
	    root->resolve(std::vector<std::string>{"child1"}, RttiTypes::TestNode);
	ASSERT_EQ(1U, res.size());
	ASSERT_TRUE(child13 == res[0].node);
}
}
