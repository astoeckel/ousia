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

#include <core/model/Style.hpp>

namespace ousia {
namespace model {
TEST(Specificity, testOperators)
{
	Specificity s1{0, 0, 1};
	Specificity s2{0, 1, 1};
	Specificity s3{1, 1, 1};
	Specificity s4{0, 0, 2};
	Specificity s5{1, 0, 2};

	// This should be s1 < s4 < s2 < s5 < s3

	ASSERT_TRUE(s1 == s1);
	ASSERT_FALSE(s1 < s1);
	ASSERT_FALSE(s1 > s1);
	ASSERT_FALSE(s1 == s2);
	ASSERT_TRUE(s1 < s2);
	ASSERT_FALSE(s1 > s2);
	ASSERT_FALSE(s1 == s3);
	ASSERT_TRUE(s1 < s3);
	ASSERT_FALSE(s1 > s3);
	ASSERT_FALSE(s1 == s4);
	ASSERT_TRUE(s1 < s4);
	ASSERT_FALSE(s1 > s4);
	ASSERT_FALSE(s1 == s5);
	ASSERT_TRUE(s1 < s5);
	ASSERT_FALSE(s1 > s5);

	ASSERT_FALSE(s2 == s1);
	ASSERT_FALSE(s2 < s1);
	ASSERT_TRUE(s2 > s1);
	ASSERT_TRUE(s2 == s2);
	ASSERT_FALSE(s2 < s2);
	ASSERT_FALSE(s2 > s2);
	ASSERT_FALSE(s2 == s3);
	ASSERT_TRUE(s2 < s3);
	ASSERT_FALSE(s2 > s3);
	ASSERT_FALSE(s2 == s4);
	ASSERT_FALSE(s2 < s4);
	ASSERT_TRUE(s2 > s4);
	ASSERT_FALSE(s2 == s5);
	ASSERT_TRUE(s2 < s5);
	ASSERT_FALSE(s2 > s5);

	ASSERT_FALSE(s3 == s1);
	ASSERT_FALSE(s3 < s1);
	ASSERT_TRUE(s3 > s1);
	ASSERT_FALSE(s3 == s2);
	ASSERT_FALSE(s3 < s2);
	ASSERT_TRUE(s3 > s2);
	ASSERT_TRUE(s3 == s3);
	ASSERT_FALSE(s3 < s3);
	ASSERT_FALSE(s3 > s3);
	ASSERT_FALSE(s3 == s4);
	ASSERT_FALSE(s3 < s4);
	ASSERT_TRUE(s3 > s4);
	ASSERT_FALSE(s3 == s5);
	ASSERT_FALSE(s3 < s5);
	ASSERT_TRUE(s3 > s5);

	ASSERT_FALSE(s4 == s1);
	ASSERT_FALSE(s4 < s1);
	ASSERT_TRUE(s4 > s1);
	ASSERT_FALSE(s4 == s2);
	ASSERT_TRUE(s4 < s2);
	ASSERT_FALSE(s4 > s2);
	ASSERT_FALSE(s4 == s3);
	ASSERT_TRUE(s4 < s3);
	ASSERT_FALSE(s4 > s3);
	ASSERT_TRUE(s4 == s4);
	ASSERT_FALSE(s4 < s4);
	ASSERT_FALSE(s4 > s4);
	ASSERT_FALSE(s4 == s5);
	ASSERT_TRUE(s4 < s5);
	ASSERT_FALSE(s4 > s5);

	ASSERT_FALSE(s5 == s1);
	ASSERT_FALSE(s5 < s1);
	ASSERT_TRUE(s5 > s1);
	ASSERT_FALSE(s5 == s2);
	ASSERT_FALSE(s5 < s2);
	ASSERT_TRUE(s5 > s2);
	ASSERT_FALSE(s5 == s3);
	ASSERT_TRUE(s5 < s3);
	ASSERT_FALSE(s5 > s3);
	ASSERT_FALSE(s5 == s4);
	ASSERT_FALSE(s5 < s4);
	ASSERT_TRUE(s5 > s4);
	ASSERT_TRUE(s5 == s5);
	ASSERT_FALSE(s5 < s5);
	ASSERT_FALSE(s5 > s5);
}

TEST(SelectorNode, testGetChildren)
{
	Manager mgr(1);
	// build some children.
	Rooted<SelectorNode> root{new SelectorNode{mgr, "root"}};
	Rooted<SelectorNode> A{new SelectorNode{mgr, "A"}};
	Rooted<SelectorNode> AMy_Select{
	    new SelectorNode{mgr, "A", {"my_select", {"a", "b"}, false}}};
	Rooted<SelectorNode> B{new SelectorNode{mgr, "B"}};

	std::vector<Rooted<SelectorNode>> children = {A, AMy_Select, B};

	for (auto &c : children) {
		root->getEdges().push_back(new SelectorNode::SelectorEdge{mgr, c});
	}
	root->getEdges().push_back(new SelectorNode::SelectorEdge{
	    mgr, B, SelectionOperator::DIRECT_DESCENDANT});

	// make some checks.
	std::vector<Rooted<SelectorNode>> expected = {A};
	std::vector<Rooted<SelectorNode>> actual =
	    root->getChildren(SelectionOperator::DESCENDANT, "A", {"true", false});
	ASSERT_EQ(expected, actual);

	expected = {A, AMy_Select};
	actual = root->getChildren(SelectionOperator::DESCENDANT, "A");
	ASSERT_EQ(expected, actual);
	actual = root->getChildren("A");
	ASSERT_EQ(expected, actual);

	expected = {A, AMy_Select, B};
	actual = root->getChildren(SelectionOperator::DESCENDANT);
	ASSERT_EQ(expected, actual);

	expected = {B};
	actual = root->getChildren(SelectionOperator::DIRECT_DESCENDANT);
	ASSERT_EQ(expected, actual);

	expected = {B, B};
	actual = root->getChildren("B");
	ASSERT_EQ(expected, actual);

	{
		expected = {A, B, B};
		PseudoSelector select = {"true", false};
		actual = root->getChildren(select);
		ASSERT_EQ(expected, actual);
	}

	expected = {A, AMy_Select, B, B};
	actual = root->getChildren();
	ASSERT_EQ(expected, actual);
}

TEST(SelectorNode, testAppend)
{
	Manager mgr(1);
	// build the root.
	Rooted<SelectorNode> root{new SelectorNode{mgr, "root"}};
	// append a child.
	Rooted<SelectorNode> A{new SelectorNode{mgr, "A"}};
	// this should work without any unmerged leafs.
	ASSERT_EQ(0U, (root->append(A)).size());
	/*
	 * check the result. We expect the SelectorTree
	 *
	 * root
	 *  |
	 *  A
	 */
	ASSERT_EQ(1U, (root->getEdges()).size());
	std::vector<Rooted<SelectorNode>> children = root->getChildren("A");
	ASSERT_EQ(1U, children.size());
	ASSERT_EQ(A, children[0]);

	// append another child.
	Rooted<SelectorNode> B{new SelectorNode{mgr, "B"}};
	ASSERT_EQ(0U, (root->append(B)).size());
	/*
	 * check the result. We expect the SelectorTree
	 *
	 * root
	 *  | \
	 *  A  B
	 */
	ASSERT_EQ(2U, (root->getEdges()).size());
	children = root->getChildren("B");
	ASSERT_EQ(1U, children.size());
	ASSERT_EQ(B, children[0]);

	// append a grandchild using a path.
	Rooted<SelectorNode> C{new SelectorNode{mgr, "C"}};
	{
		Rooted<SelectorNode> pathA{new SelectorNode{mgr, "A"}};
		pathA->append(C);

		ASSERT_EQ(0U, (root->append(pathA)).size());
	}
	/*
	 * check the result. We expect the SelectorTree
	 *
	 * root
	 *  | \
	 *  A  B
	 *  |
	 *  C
	 */
	ASSERT_EQ(2U, (root->getEdges()).size());
	children = root->getChildren("A");
	ASSERT_EQ(1U, children.size());
	ASSERT_EQ(1U, (children[0]->getEdges()).size());
	children = children[0]->getChildren("C");
	ASSERT_EQ(1U, children.size());
	ASSERT_EQ(C, children[0]);

	// append a subtree that is partially contained.
	Rooted<SelectorNode> D{new SelectorNode{mgr, "D"}};
	{
		Rooted<SelectorNode> pathA{new SelectorNode{mgr, "A"}};
		Rooted<SelectorNode> pathC{new SelectorNode{mgr, "C"}};
		pathA->append(pathC);
		pathA->append(D);

		// The C leaf can not be appended because it is already part of the
		// tree. So we expect that as a return value.
		std::vector<Rooted<SelectorNode>> unmergedLeafs = root->append(pathA);
		ASSERT_EQ(1U, unmergedLeafs.size());
		ASSERT_EQ(C.get(), unmergedLeafs[0].get());
	}
	/*
	 * check the result. We expect the SelectorTree
	 *
	 * root
	 *  | \
	 *  A  B
	 *  |\
	 *  C D
	 */
	ASSERT_EQ(2U, (root->getEdges()).size());
	children = root->getChildren("A");
	ASSERT_EQ(1U, children.size());
	ASSERT_EQ(2U, (children[0]->getEdges()).size());
	children = children[0]->getChildren("D");
	ASSERT_EQ(1U, children.size());
	ASSERT_EQ(D, children[0]);

	// append a child with a non-trivial PseudoSelector.
	Rooted<SelectorNode> ASelect{
	    new SelectorNode{mgr, "A", {"my_select", {"a", "b"}, false}}};
	ASSERT_EQ(0U, (root->append(ASelect)).size());
	ASSERT_EQ(3U, (root->getEdges()).size());
	children = root->getChildren("A");
	ASSERT_EQ(2U, children.size());
	ASSERT_EQ(ASelect, children[1]);
}
}
}
