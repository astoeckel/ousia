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

#include <iostream>
#include <sstream>

#include <plugins/css/CSSParser.hpp>

#include <core/parser/StandaloneParserContext.hpp>

namespace ousia {
TEST(CSSParser, testParseSelectors)
{
	// create a string describing a SelectorTree
	std::string data{"A>B,A B:r, C#a A[bla=\"blub\"], A::g(4,2,3)"};
	/* This should describe the tree:
	 * root_____
	 * | \      \
	 * A  C#a  A::g(4,2,3)
	 * |\    \
	 * B B::r A[bla="blub"]
	 */

	// initialize an empty parser context.
	StandaloneParserContext ctx;

	// parse the data.
	CSSParser instance;
	Rooted<SelectorNode> root =
	    instance.parse(data, ctx.context).cast<SelectorNode>();

	// we expect three children of the root node overall.
	ASSERT_EQ(3U, root->getEdges().size());
	// get all "A" children, which should be two.
	std::vector<Rooted<SelectorNode>> children = root->getChildren("A");
	ASSERT_EQ(2U, children.size());
	// assert A
	Rooted<SelectorNode> A = children[0];
	ASSERT_EQ("A", A->getName());
	{
		PseudoSelector select{"true", false};
		ASSERT_EQ(select, A->getPseudoSelector());
	}
	ASSERT_EQ(2U, A->getEdges().size());
	ASSERT_FALSE(A->isAccepting());
	ASSERT_EQ(0U, A->getRuleSet()->getRules().size());
	{
		// assert A > B
		std::vector<Rooted<SelectorNode>> Achildren =
		    A->getChildren(SelectionOperator::DIRECT_DESCENDANT, "B");
		ASSERT_EQ(1U, Achildren.size());
		Rooted<SelectorNode> B = Achildren[0];
		ASSERT_EQ("B", B->getName());
		{
			PseudoSelector select{"true", false};
			ASSERT_EQ(select, B->getPseudoSelector());
		}
		ASSERT_EQ(0U, B->getEdges().size());
		ASSERT_TRUE(B->isAccepting());
		ASSERT_EQ(0U, B->getRuleSet()->getRules().size());
		// assert A B:r
		Achildren = A->getChildren(SelectionOperator::DESCENDANT, "B");
		ASSERT_EQ(1U, Achildren.size());
		Rooted<SelectorNode> Br = Achildren[0];
		ASSERT_EQ("B", Br->getName());
		{
			PseudoSelector select{"r", false};
			ASSERT_EQ(select, Br->getPseudoSelector());
		}
		ASSERT_EQ(0U, Br->getEdges().size());
		ASSERT_TRUE(Br->isAccepting());
		ASSERT_EQ(0U, Br->getRuleSet()->getRules().size());
	}
	// assert C#a
	children = root->getChildren("C");
	ASSERT_EQ(1U, children.size());
	Rooted<SelectorNode> C = children[0];
	ASSERT_EQ("C", C->getName());
	{
		PseudoSelector select{"has_id", {"a"}, false};
		ASSERT_EQ(select, C->getPseudoSelector());
	}
	ASSERT_EQ(1U, C->getEdges().size());
	ASSERT_FALSE(C->isAccepting());
	ASSERT_EQ(0U, C->getRuleSet()->getRules().size());
	{
		// assert C#a A[bla=\"blub\"]
		std::vector<Rooted<SelectorNode>> Cchildren =
		    C->getChildren(SelectionOperator::DESCENDANT, "A");
		ASSERT_EQ(1U, Cchildren.size());
		Rooted<SelectorNode> A = Cchildren[0];
		ASSERT_EQ("A", A->getName());
		{
			PseudoSelector select{"has_value", {"bla", "blub"}, false};
			ASSERT_EQ(select, A->getPseudoSelector());
		}
		ASSERT_EQ(0U, A->getEdges().size());
		ASSERT_TRUE(A->isAccepting());
		ASSERT_EQ(0U, A->getRuleSet()->getRules().size());
	}
	// assert A::g(4,2,3)
	children = root->getChildren("A");
	ASSERT_EQ(2U, children.size());
	Rooted<SelectorNode> Ag = children[1];
	ASSERT_EQ("A", Ag->getName());
	{
		PseudoSelector select{"g", {Variant(4), Variant(2), Variant(3)}, true};
		ASSERT_EQ(select, Ag->getPseudoSelector());
	}
	ASSERT_EQ(0U, Ag->getEdges().size());
	ASSERT_TRUE(Ag->isAccepting());
	ASSERT_EQ(0U, Ag->getRuleSet()->getRules().size());
}

TEST(CSSParser, testParseCSS)
{
	// create the CSS input
	std::stringstream input;
	input << "A, B A {\n";
	input << "/*\n";
	input << " * Some multiline\n";
	input << " * comment\n";
	input << " */\n";
	input << "\t ident1 : \"val1\";\n";
	input << "\t ident2 : \"val2\";\n";
	input << "}\n";
	input << "A:select(a,b) {\n";
	input << "\t ident3 : \"val3\";\n";
	input << "}\n";
	input << "A {\n";
	input << "\t ident1 : \"val4\";\n";
	input << "}\n";

	// initialize an empty parser context.
	StandaloneParserContext ctx;

	// parse the input.
	CSSParser instance;
	CharReader reader{input};
	Rooted<SelectorNode> root =
	    instance.parse(reader, ctx.context).cast<SelectorNode>();

	// we expect three children of the root node overall.
	ASSERT_EQ(3U, root->getEdges().size());
	// get all "A" children, which should be two.
	std::vector<Rooted<SelectorNode>> children = root->getChildren("A");
	ASSERT_EQ(2U, children.size());
	// assert A
	/*
	 * A {
	 * ident1 : "val1";
	 * ident2 : "val2";
	 * }
	 *
	 * and
	 *
	 * A {
	 * ident1 : "val4";
	 * }
	 *
	 * should be merged.
	 */
	Rooted<SelectorNode> A = children[0];
	ASSERT_EQ("A", A->getName());
	{
		PseudoSelector select{"true", false};
		ASSERT_EQ(select, A->getPseudoSelector());
	}
	ASSERT_EQ(0U, A->getEdges().size());
	ASSERT_TRUE(A->isAccepting());
	{
		Rooted<RuleSet> ruleSet = A->getRuleSet();
		ASSERT_EQ(2U, ruleSet->getRules().size());
		Variant v = ruleSet->getRules()["ident1"];
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("val4", v.asString());
		v = ruleSet->getRules()["ident2"];
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("val2", v.asString());
	}
	/*
	 * For this part of the SelectorTree we only have
	 *
	 * A:select(a,b) {
	 * ident3 : val3;
	 * }
	 */
	Rooted<SelectorNode> Aselect = children[1];
	ASSERT_EQ("A", Aselect->getName());
	{
		PseudoSelector select{"select", {"a", "b"}, false};
		ASSERT_EQ(select, Aselect->getPseudoSelector());
	}
	ASSERT_EQ(0U, Aselect->getEdges().size());
	ASSERT_TRUE(Aselect->isAccepting());
	{
		Rooted<RuleSet> ruleSet = Aselect->getRuleSet();
		ASSERT_EQ(1U, ruleSet->getRules().size());
		Variant v = ruleSet->getRules()["ident3"];
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("val3", v.asString());
	}
	/*
	 * The remaining part of the SelectorTree is
	 *
	 * B A {
	 * ident1 : val1;
	 * ident2 : val2;
	 * }
	 */
	children = root->getChildren("B");
	ASSERT_EQ(1U, children.size());

	Rooted<SelectorNode> B = children[0];
	ASSERT_EQ("B", B->getName());
	{
		PseudoSelector select{"true", false};
		ASSERT_EQ(select, B->getPseudoSelector());
	}
	ASSERT_EQ(1U, B->getEdges().size());
	ASSERT_FALSE(B->isAccepting());
	ASSERT_EQ(0U, B->getRuleSet()->getRules().size());

	children = B->getChildren("A");
	ASSERT_EQ(1U, children.size());

	Rooted<SelectorNode> BA = children[0];
	ASSERT_EQ("A", BA->getName());
	{
		PseudoSelector select{"true", false};
		ASSERT_EQ(select, BA->getPseudoSelector());
	}
	ASSERT_EQ(0U, BA->getEdges().size());
	ASSERT_TRUE(BA->isAccepting());
	{
		Rooted<RuleSet> ruleSet = BA->getRuleSet();
		ASSERT_EQ(2U, ruleSet->getRules().size());
		Variant v = ruleSet->getRules()["ident1"];
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("val1", v.asString());
		v = ruleSet->getRules()["ident2"];
		ASSERT_TRUE(v.isString());
		ASSERT_EQ("val2", v.asString());
	}
}

void assertException(std::string css)
{
	CharReader reader(css);
	TerminalLogger logger(std::cerr, true);
	{
		ScopedLogger sl(logger, "test.css", SourceLocation{},
		                CharReader::contextCallback, &reader);
		StandaloneParserContext ctx(sl);

		CSSParser instance;
		try {
			instance.parse(reader, ctx.context).cast<SelectorNode>();
		}
		catch (LoggableException ex) {
			logger.log(ex);
		}
		ASSERT_TRUE(logger.hasError());
	}
}

TEST(CSSParser, testParseExceptions)
{
	assertException(", ");
	assertException("A::myGenerative , ");
	assertException("A::(a)");
	assertException("A::f()");
	assertException("A#");
	assertException("A[]");
	assertException("A[a");
	assertException("A[a=]");
	assertException("A > ");
}
}
