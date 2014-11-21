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

#include <core/Managed.hpp>

#include <core/Node.hpp>

namespace ousia {
namespace dom {

class TestNode : public Node {
private:
	std::vector<Owned<Node>> children;

protected:
	void doResolve(std::vector<Rooted<Node>> &res,
	               const std::vector<std::string> &path, Filter filter,
	               void *filterData, unsigned idx, VisitorSet &visited) override
	{
		for (auto &n : children) {
			n->resolve(res, path, filter, filterData, idx, visited, nullptr);
		}
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

	std::vector<Rooted<Node>> res;
	res = root->resolve(std::vector<std::string>{"root", "child1", "child11"});
	ASSERT_EQ(1, res.size());
	ASSERT_TRUE(child11 == *(res.begin()));

	res = root->resolve(std::vector<std::string>{"child1", "child11"});
	ASSERT_EQ(1, res.size());
	ASSERT_TRUE(child11 == *(res.begin()));

	res = root->resolve(std::vector<std::string>{"child11"});
	ASSERT_EQ(1, res.size());
	ASSERT_TRUE(child11 == *(res.begin()));
}

class TestManagedEventOwner : public Managed {
public:
	using Managed::Managed;

	int triggered = false;
};

static void handleEvent(const Event &ev, Handle<Managed> owner)
{
	owner.cast<TestManagedEventOwner>()->triggered++;
}

static void handleEventStop(const Event &ev, Handle<Managed> owner)
{
	owner.cast<TestManagedEventOwner>()->triggered++;
	ev.stopPropagation();
}

TEST(Node, events)
{
	Manager mgr;
	Rooted<Node> n{new Node(mgr)};

	Rooted<TestManagedEventOwner> e1{new TestManagedEventOwner(mgr)};
	Rooted<TestManagedEventOwner> e2{new TestManagedEventOwner(mgr)};
	Rooted<TestManagedEventOwner> e3{new TestManagedEventOwner(mgr)};

	ASSERT_EQ(0, n->registerEventHandler(EventType::UPDATE, handleEvent, e1));
	ASSERT_EQ(1, n->registerEventHandler(EventType::NAME_CHANGE, handleEvent, e2));
	ASSERT_EQ(2, n->registerEventHandler(EventType::NAME_CHANGE, handleEvent, e3));

	ASSERT_FALSE(e1->triggered);
	ASSERT_FALSE(e2->triggered);
	ASSERT_FALSE(e3->triggered);

	{
		Event ev{EventType::ADD_CHILD};
		ASSERT_FALSE(n->triggerEvent(ev));
	}

	{
		Event ev{EventType::UPDATE};
		ASSERT_TRUE(n->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(0, e2->triggered);
		ASSERT_EQ(0, e3->triggered);
	}

	{
		Event ev{EventType::NAME_CHANGE};
		ASSERT_TRUE(n->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
		ASSERT_EQ(1, e3->triggered);
	}

	ASSERT_TRUE(n->unregisterEventHandler(1));
	ASSERT_FALSE(n->unregisterEventHandler(1));

	{
		Event ev{EventType::NAME_CHANGE};
		ASSERT_TRUE(n->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
		ASSERT_EQ(2, e3->triggered);
	}

	ASSERT_TRUE(n->unregisterEventHandler(0));
	ASSERT_FALSE(n->unregisterEventHandler(0));

	{
		Event ev{EventType::UPDATE};
		ASSERT_FALSE(n->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
		ASSERT_EQ(2, e3->triggered);
	}

	ASSERT_TRUE(n->unregisterEventHandler(2));
	ASSERT_FALSE(n->unregisterEventHandler(2));

	{
		Event ev{EventType::NAME_CHANGE};
		ASSERT_FALSE(n->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
		ASSERT_EQ(2, e3->triggered);
	}
}

TEST(Node, eventBubbling)
{
	Manager mgr;
	Rooted<Node> n1{new Node(mgr)};
	Rooted<Node> n2{new Node(mgr, n1)};

	Rooted<TestManagedEventOwner> e1{new TestManagedEventOwner(mgr)};
	Rooted<TestManagedEventOwner> e2{new TestManagedEventOwner(mgr)};
	Rooted<TestManagedEventOwner> e3{new TestManagedEventOwner(mgr)};

	ASSERT_EQ(0, n1->registerEventHandler(EventType::UPDATE, handleEvent, e1, true));
	ASSERT_EQ(1, n1->registerEventHandler(EventType::NAME_CHANGE, handleEvent, e2, true));
	ASSERT_EQ(2, n1->registerEventHandler(EventType::NAME_CHANGE, handleEvent, e3, false));

	ASSERT_FALSE(e1->triggered);
	ASSERT_FALSE(e2->triggered);
	ASSERT_FALSE(e3->triggered);

	{
		Event ev{EventType::ADD_CHILD};
		ASSERT_FALSE(n2->triggerEvent(ev));
	}

	{
		Event ev{EventType::UPDATE};
		ASSERT_TRUE(n2->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(0, e2->triggered);
		ASSERT_EQ(0, e3->triggered);
	}

	{
		Event ev{EventType::UPDATE, false};
		ASSERT_FALSE(n2->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(0, e2->triggered);
		ASSERT_EQ(0, e3->triggered);
	}

	{
		Event ev{EventType::NAME_CHANGE};
		ASSERT_TRUE(n2->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
		ASSERT_EQ(0, e3->triggered);
	}

	ASSERT_TRUE(n1->unregisterEventHandler(1));
	ASSERT_FALSE(n1->unregisterEventHandler(1));

	{
		Event ev{EventType::NAME_CHANGE};
		ASSERT_FALSE(n2->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
		ASSERT_EQ(0, e3->triggered);
	}

	ASSERT_TRUE(n1->unregisterEventHandler(0));
	ASSERT_FALSE(n1->unregisterEventHandler(0));

	{
		Event ev{EventType::UPDATE};
		ASSERT_FALSE(n2->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
		ASSERT_EQ(0, e3->triggered);
	}

	ASSERT_TRUE(n1->unregisterEventHandler(2));
	ASSERT_FALSE(n1->unregisterEventHandler(2));

	{
		Event ev{EventType::NAME_CHANGE};
		ASSERT_FALSE(n2->triggerEvent(ev));
		ASSERT_EQ(1, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
		ASSERT_EQ(0, e3->triggered);
	}
}

TEST(Node, eventStopPropagation)
{
	Manager mgr;
	Rooted<Node> n1{new Node(mgr)};
	Rooted<Node> n2{new Node(mgr, n1)};

	Rooted<TestManagedEventOwner> e1{new TestManagedEventOwner(mgr)};
	Rooted<TestManagedEventOwner> e2{new TestManagedEventOwner(mgr)};

	ASSERT_EQ(0, n1->registerEventHandler(EventType::UPDATE, handleEvent, e1, true));
	ASSERT_EQ(0, n2->registerEventHandler(EventType::UPDATE, handleEventStop, e2, true));

	ASSERT_FALSE(e1->triggered);
	ASSERT_FALSE(e2->triggered);

	{
		Event ev{EventType::UPDATE};
		n2->triggerEvent(ev);

		ASSERT_EQ(0, e1->triggered);
		ASSERT_EQ(1, e2->triggered);
	}

}

}
}
