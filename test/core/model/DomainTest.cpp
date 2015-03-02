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

#include <core/common/Rtti.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/model/Domain.hpp>

#include "TestDomain.hpp"

namespace ousia {

void assert_path(const ResolutionResult &res, const Rtti *expected_type,
                 std::vector<std::string> expected_path)
{
	// Check class/type
	ASSERT_TRUE(res.node->isa(expected_type));

	// Check path
	ASSERT_EQ(expected_path, res.node->path());
}

TEST(Domain, testDomainResolving)
{
	// Construct Manager
	Logger logger;
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the domain.
	Rooted<Domain> domain = constructBookDomain(mgr, sys, logger);

	std::vector<ResolutionResult> res;

	// There is one domain called "book"
	res = domain->resolve(&RttiTypes::Domain, "book");
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], &RttiTypes::Domain, {"book"});

	// There is one domain called "book"
	res = domain->resolve(&RttiTypes::StructuredClass, "book");
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], &RttiTypes::StructuredClass, {"book", "book"});

	// If we explicitly ask for the "book, book" path, then only the
	// StructuredClass should be returned.
	res = domain->resolve(&RttiTypes::Domain,
	                      std::vector<std::string>{"book", "book"});
	ASSERT_EQ(0U, res.size());

	res = domain->resolve(&RttiTypes::StructuredClass,
	                      std::vector<std::string>{"book", "book"});
	ASSERT_EQ(1U, res.size());

	// If we ask for "section" the result should be unique as well.
	res = domain->resolve(&RttiTypes::StructuredClass, "section");
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], &RttiTypes::StructuredClass, {"book", "section"});

	// If we ask for "paragraph" it is referenced two times in the Domain graph,
	// but should be returned only once.
	res = domain->resolve(&RttiTypes::StructuredClass, "paragraph");
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], &RttiTypes::StructuredClass, {"book", "paragraph"});
}

// i use this wrapper due to the strange behaviour of GTEST.
static void assertFalse(bool b) { ASSERT_FALSE(b); }

static Rooted<FieldDescriptor> createUnsortedPrimitiveField(
    Handle<StructuredClass> strct, Handle<Type> type, Logger &logger, bool tree,
    std::string name)
{
	FieldDescriptor::FieldType fieldType = FieldDescriptor::FieldType::SUBTREE;
	if (tree) {
		fieldType = FieldDescriptor::FieldType::TREE;
	}

	auto res = strct->createPrimitiveFieldDescriptor(type, logger, fieldType,
	                                                 std::move(name));
	assertFalse(res.second);
	return res.first;
}

TEST(StructuredClass, getFieldDescriptors)
{
	/*
	 * We construct a case with the three levels:
	 * 1.) A has the SUBTREE fields a and b as well as a TREE field.
	 * 2.) B is a subclass of A and has the SUBTREE fields b and c as well as
	 *     a TREE field.
	 * 3.) C is a subclass of B and has the SUBTREE field a.
	 * As a result we expect C to have none of As fields, the TREE field of B,
	 * and the SUBTREE fields a (of C) , b and c (of B).
	 */
	TerminalLogger logger{std::cout};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Domain> domain{new Domain(mgr, sys, "myDomain")};

	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), nullptr, false, true)};
	Rooted<FieldDescriptor> A_a = createUnsortedPrimitiveField(
	    A, sys->getStringType(), logger, false, "a");
	Rooted<FieldDescriptor> A_b = createUnsortedPrimitiveField(
	    A, sys->getStringType(), logger, false, "b");
	Rooted<FieldDescriptor> A_main = createUnsortedPrimitiveField(
	    A, sys->getStringType(), logger, true, "somename");

	Rooted<StructuredClass> B{new StructuredClass(
	    mgr, "B", domain, Cardinality::any(), A, false, true)};
	Rooted<FieldDescriptor> B_b = createUnsortedPrimitiveField(
	    B, sys->getStringType(), logger, false, "b");
	Rooted<FieldDescriptor> B_c = createUnsortedPrimitiveField(
	    B, sys->getStringType(), logger, false, "c");
	Rooted<FieldDescriptor> B_main = createUnsortedPrimitiveField(
	    B, sys->getStringType(), logger, true, "othername");

	Rooted<StructuredClass> C{new StructuredClass(
	    mgr, "C", domain, Cardinality::any(), B, false, true)};
	Rooted<FieldDescriptor> C_a = createUnsortedPrimitiveField(
	    C, sys->getStringType(), logger, false, "a");

	ASSERT_TRUE(domain->validate(logger));

	// check all FieldDescriptors
	{
		NodeVector<FieldDescriptor> fds = A->getFieldDescriptors();
		ASSERT_EQ(3, fds.size());
		ASSERT_EQ(A_a, fds[0]);
		ASSERT_EQ(A_b, fds[1]);
		ASSERT_EQ(A_main, fds[2]);
	}
	{
		NodeVector<FieldDescriptor> fds = B->getFieldDescriptors();
		ASSERT_EQ(4, fds.size());
		ASSERT_EQ(A_a, fds[0]);
		ASSERT_EQ(B_b, fds[1]);
		ASSERT_EQ(B_c, fds[2]);
		ASSERT_EQ(B_main, fds[3]);
	}
	{
		NodeVector<FieldDescriptor> fds = C->getFieldDescriptors();
		ASSERT_EQ(4, fds.size());
		ASSERT_EQ(B_b, fds[0]);
		ASSERT_EQ(B_c, fds[1]);
		// superclass fields come before subclass fields (except for the TREE
		// field, which is always last).
		ASSERT_EQ(C_a, fds[2]);
		ASSERT_EQ(B_main, fds[3]);
	}
}

TEST(StructuredClass, getFieldDescriptorsCycles)
{
	Logger logger;
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Domain> domain{new Domain(mgr, sys, "myDomain")};

	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), nullptr, false, true)};
	A->addSubclass(A, logger);
	Rooted<FieldDescriptor> A_a = createUnsortedPrimitiveField(
	    A, sys->getStringType(), logger, false, "a");
	ASSERT_FALSE(domain->validate(logger));
	// if we call getFieldDescriptors that should still return a valid result.
	NodeVector<FieldDescriptor> fds = A->getFieldDescriptors();
	ASSERT_EQ(1, fds.size());
	ASSERT_EQ(A_a, fds[0]);
}

Rooted<StructuredClass> getClass(const std::string name, Handle<Domain> dom)
{
	std::vector<ResolutionResult> res =
	    dom->resolve(&RttiTypes::StructuredClass, name);
	return res[0].node.cast<StructuredClass>();
}

TEST(Descriptor, pathTo)
{
	// Start with some easy examples from the book domain.
	TerminalLogger logger{std::cout};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the domain.
	Rooted<Domain> domain = constructBookDomain(mgr, sys, logger);

	// get the book node and the section node.
	Rooted<StructuredClass> book = getClass("book", domain);
	Rooted<StructuredClass> section = getClass("section", domain);
	// get the path in between.
	NodeVector<Node> path = book->pathTo(section, logger);
	ASSERT_EQ(1U, path.size());
	ASSERT_TRUE(path[0]->isa(&RttiTypes::FieldDescriptor));

	// get the text node.
	Rooted<StructuredClass> text = getClass("text", domain);
	// get the path between book and text via paragraph.
	path = book->pathTo(text, logger);
	ASSERT_EQ(3U, path.size());
	ASSERT_TRUE(path[0]->isa(&RttiTypes::FieldDescriptor));
	ASSERT_TRUE(path[1]->isa(&RttiTypes::StructuredClass));
	ASSERT_EQ("paragraph", path[1]->getName());
	ASSERT_TRUE(path[2]->isa(&RttiTypes::FieldDescriptor));

	// get the subsection node.
	Rooted<StructuredClass> subsection = getClass("subsection", domain);
	// try to get the path between book and subsection.
	path = book->pathTo(subsection, logger);
	// this should be impossible.
	ASSERT_EQ(0U, path.size());

	// try to construct the path between section and the text field.
	auto res = section->pathTo(text->getFieldDescriptor(), logger);
	ASSERT_TRUE(res.second);
	path = res.first;
	ASSERT_EQ(4U, path.size());
	ASSERT_TRUE(path[0]->isa(&RttiTypes::FieldDescriptor));
	ASSERT_TRUE(path[1]->isa(&RttiTypes::StructuredClass));
	ASSERT_EQ("paragraph", path[1]->getName());
	ASSERT_TRUE(path[2]->isa(&RttiTypes::FieldDescriptor));
	ASSERT_TRUE(path[3]->isa(&RttiTypes::StructuredClass));
	ASSERT_EQ("text", path[3]->getName());
}

TEST(Descriptor, pathToAdvanced)
{
	/*
	 * Now we build a really nasty domain with lots of transparency
	 * and inheritance. The basic idea is to have three paths from start to
	 * finish, where one is blocked by overriding fields and the longer valid
	 * one is found first such that it has to be replaced by the shorter one
	 * during the search.
	 *
	 * To achieve that we have the following structure:
	 * 1.) The start class inherits from A.
	 * 2.) A has B as child in the default field.
	 * 3.) B is transparent and has no children (but C as subclass)
	 * 4.) C is a subclass of B, transparent and has
	 *     the target as child (shortest path).
	 * 5.) A has D as child in the default field.
	 * 6.) D is transparent has E as child in the default field.
	 * 7.) E is transparent and has target as child in the default field
	 *     (longer path)
	 *
	 * So the path A_second_field, C, C_field should be returned.
	 */
	Manager mgr{1};
	TerminalLogger logger{std::cout};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "nasty")};

	// Let's create the classes that we need first
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), {nullptr}, false, true)};

	Rooted<StructuredClass> start{new StructuredClass(
	    mgr, "start", domain, Cardinality::any(), A, false, false)};

	Rooted<StructuredClass> B{new StructuredClass(
	    mgr, "B", domain, Cardinality::any(), {nullptr}, true, false)};

	Rooted<StructuredClass> C{new StructuredClass(
	    mgr, "C", domain, Cardinality::any(), B, true, false)};

	Rooted<StructuredClass> D{new StructuredClass(
	    mgr, "D", domain, Cardinality::any(), {nullptr}, true, false)};

	Rooted<StructuredClass> E{new StructuredClass(
	    mgr, "E", domain, Cardinality::any(), {nullptr}, true, false)};

	Rooted<StructuredClass> target{
	    new StructuredClass(mgr, "target", domain, Cardinality::any())};

	// We create a field for A
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	A_field->addChild(B);
	A_field->addChild(D);

	// We create no field for B
	// One for C
	Rooted<FieldDescriptor> C_field = C->createFieldDescriptor(logger).first;
	C_field->addChild(target);

	// One for D
	Rooted<FieldDescriptor> D_field = D->createFieldDescriptor(logger).first;
	D_field->addChild(E);

	// One for E
	Rooted<FieldDescriptor> E_field = E->createFieldDescriptor(logger).first;
	E_field->addChild(target);

	ASSERT_TRUE(domain->validate(logger));

#ifdef MANAGER_GRAPHVIZ_EXPORT
	// dump the manager state
	mgr.exportGraphviz("nastyDomain.dot");
#endif

	// and now we should be able to find the shortest path as suggested
	NodeVector<Node> path = start->pathTo(target, logger);
	ASSERT_EQ(3U, path.size());
	ASSERT_TRUE(path[0]->isa(&RttiTypes::FieldDescriptor));
	ASSERT_EQ("", path[0]->getName());
	ASSERT_TRUE(path[1]->isa(&RttiTypes::StructuredClass));
	ASSERT_EQ("C", path[1]->getName());
	ASSERT_TRUE(path[2]->isa(&RttiTypes::FieldDescriptor));
	ASSERT_EQ("", path[2]->getName());
}

TEST(Descriptor, pathToCycles)
{
	// build a domain with a cycle.
	Manager mgr{1};
	Logger logger;
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "cycles")};
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), {nullptr}, true, true)};
	A->addSubclass(A, logger);
	ASSERT_FALSE(domain->validate(logger));
	Rooted<StructuredClass> B{new StructuredClass(
	    mgr, "B", domain, Cardinality::any(), {nullptr}, false, true)};
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	A_field->addChild(B);
	/*
	 * Now try to create the path from A to B. A direct path is possible but
	 * in the worst case this could also try to find shorter paths via an
	 * endless repition of A instances.
	 * As we cut the search tree at paths that are longer than our current
	 * optimum this should not happen, though.
	 */
	NodeVector<Node> path = A->pathTo(B, logger);
	ASSERT_EQ(1, path.size());
	ASSERT_EQ(A_field, path[0]);
}

TEST(Descriptor, getDefaultFields)
{
	// construct a domain with lots of default fields to test.
	// start with a single structure class.
	Manager mgr{1};
	TerminalLogger logger{std::cout};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "nasty")};

	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), nullptr, false, true)};

	// in this trivial case no field should be found.
	ASSERT_TRUE(A->getDefaultFields().empty());

	// create a field.
	Rooted<FieldDescriptor> A_prim_field =
	    A->createPrimitiveFieldDescriptor(sys->getStringType(), logger).first;
	// now we should find that.
	auto fields = A->getDefaultFields();
	ASSERT_EQ(1U, fields.size());
	ASSERT_EQ(A_prim_field, fields[0]);

	// remove that field from A and add it to another class.

	Rooted<StructuredClass> B{new StructuredClass(
	    mgr, "B", domain, Cardinality::any(), nullptr, false, true)};

	B->moveFieldDescriptor(A_prim_field, logger);

	// new we shouldn't find the field anymore.
	ASSERT_TRUE(A->getDefaultFields().empty());

	// but we should find it again if we set B as superclass of A.
	A->setSuperclass(B, logger);
	fields = A->getDefaultFields();
	ASSERT_EQ(1U, fields.size());
	ASSERT_EQ(A_prim_field, fields[0]);

	// and we should not be able to find it if we override the field.
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	ASSERT_TRUE(A->getDefaultFields().empty());

	// add a transparent child class.

	Rooted<StructuredClass> C{new StructuredClass(
	    mgr, "C", domain, Cardinality::any(), nullptr, true, false)};
	A_field->addChild(C);

	// add a primitive field for it.
	Rooted<FieldDescriptor> C_field =
	    C->createPrimitiveFieldDescriptor(sys->getStringType(), logger).first;

	// now we should find that.
	fields = A->getDefaultFields();
	ASSERT_EQ(1U, fields.size());
	ASSERT_EQ(C_field, fields[0]);

	// add another transparent child class to A with a daughter class that has
	// in turn a subclass with a primitive field.
	Rooted<StructuredClass> D{new StructuredClass(
	    mgr, "D", domain, Cardinality::any(), nullptr, true, false)};
	A_field->addChild(D);
	Rooted<FieldDescriptor> D_field = D->createFieldDescriptor(logger).first;
	Rooted<StructuredClass> E{new StructuredClass(
	    mgr, "E", domain, Cardinality::any(), nullptr, true, false)};
	D_field->addChild(E);
	Rooted<StructuredClass> F{new StructuredClass(
	    mgr, "E", domain, Cardinality::any(), E, true, false)};
	Rooted<FieldDescriptor> F_field =
	    F->createPrimitiveFieldDescriptor(sys->getStringType(), logger).first;

	// now we should find both primitive fields, but the C field first.
	fields = A->getDefaultFields();
	ASSERT_EQ(2U, fields.size());
	ASSERT_EQ(C_field, fields[0]);
	ASSERT_EQ(F_field, fields[1]);
}

TEST(Descriptor, getDefaultFieldsCycles)
{
	// build a domain with a cycle.
	Manager mgr{1};
	Logger logger;
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "cycles")};
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), {nullptr}, true, true)};
	A->addSubclass(A, logger);
	ASSERT_FALSE(domain->validate(logger));
	Rooted<FieldDescriptor> A_field =
	    A->createPrimitiveFieldDescriptor(sys->getStringType(), logger).first;
	/*
	 * Now try to get the default fields of A. This should not lead to cycles
	 * if we correctly note all already visited nodes.
	 */
	NodeVector<FieldDescriptor> defaultFields = A->getDefaultFields();
	ASSERT_EQ(1, defaultFields.size());
	ASSERT_EQ(A_field, defaultFields[0]);
}

TEST(Descriptor, getPermittedChildren)
{
	// analyze the book domain.
	TerminalLogger logger{std::cout};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the domain.
	Rooted<Domain> domain = constructBookDomain(mgr, sys, logger);
	// get the relevant classes.
	Rooted<StructuredClass> book = getClass("book", domain);
	Rooted<StructuredClass> section = getClass("section", domain);
	Rooted<StructuredClass> paragraph = getClass("paragraph", domain);
	Rooted<StructuredClass> text = getClass("text", domain);
	/*
	 * as permitted children we expect section, paragraph and text in exactly
	 * that order. section should be before paragraph because of declaration
	 * order and text should be last because it needs a transparent paragraph
	 * in between.
	 */
	NodeVector<StructuredClass> children = book->getPermittedChildren();
	ASSERT_EQ(3U, children.size());
	ASSERT_EQ(section, children[0]);
	ASSERT_EQ(paragraph, children[1]);
	ASSERT_EQ(text, children[2]);

	// Now we add a subclass to text.
	Rooted<StructuredClass> sub{new StructuredClass(
	    mgr, "Subclass", domain, Cardinality::any(), text, true, false)};
	// And that should be in the result list as well now.
	children = book->getPermittedChildren();
	ASSERT_EQ(4U, children.size());
	ASSERT_EQ(section, children[0]);
	ASSERT_EQ(paragraph, children[1]);
	ASSERT_EQ(text, children[2]);
	ASSERT_EQ(sub, children[3]);
}

TEST(Descriptor, getPermittedChildrenCycles)
{
	// build a domain with a cycle.
	Manager mgr{1};
	Logger logger;
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "cycles")};
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), {nullptr}, true, true)};
	A->addSubclass(A, logger);
	ASSERT_FALSE(domain->validate(logger));
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	// we make the cycle worse by adding A as child of itself.
	A_field->addChild(A);
	/*
	 * Now try to get the permitted children of A. This should not lead to
	 * cycles
	 * if we correctly note all already visited nodes.
	 */
	NodeVector<StructuredClass> children = A->getPermittedChildren();
	ASSERT_EQ(1, children.size());
	ASSERT_EQ(A, children[0]);
}

TEST(Descriptor, getSyntaxDescriptor)
{
	// build an ontology with some custom syntax.
	Manager mgr{1};
	Logger logger;
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "ontology")};
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), {nullptr}, true, true)};
	A->setStartToken(TokenDescriptor(Tokens::Indent));
	A->setEndToken(TokenDescriptor(Tokens::Dedent));
	{
		TokenDescriptor sh{"<+>"};
		sh.id = 1;
		A->setShortToken(sh);
	}
	// check the SyntaxDescriptor
	SyntaxDescriptor stx = A->getSyntaxDescriptor();
	ASSERT_EQ(Tokens::Indent, stx.start);
	ASSERT_EQ(Tokens::Dedent, stx.end);
	ASSERT_EQ(1, stx.shortForm);
	ASSERT_EQ(A, stx.descriptor);
	ASSERT_TRUE(stx.isStruct());
	ASSERT_FALSE(stx.isAnnotation());
	ASSERT_FALSE(stx.isFieldDescriptor());
}

TEST(Descriptor, getPermittedTokens)
{
	// build an ontology with some custom syntax.
	Manager mgr{1};
	Logger logger;
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "ontology")};
	// add one StructuredClass with all tokens set.
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), {nullptr}, true, true)};
	A->setStartToken(TokenDescriptor(Tokens::Indent));
	A->setEndToken(TokenDescriptor(Tokens::Dedent));
	{
		TokenDescriptor sh{"<+>"};
		sh.id = 1;
		A->setShortToken(sh);
	}
	// add a field with one token set.
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	A_field->setEndToken(TokenDescriptor(Tokens::Newline));
	A_field->addChild(A);
	// add an annotation with start and end set.
	Rooted<AnnotationClass> A_anno = domain->createAnnotationClass("A");
	{
		TokenDescriptor start{"<"};
		start.id = 7;
		A_anno->setStartToken(start);
	}
	{
		TokenDescriptor end{">"};
		end.id = 8;
		A_anno->setEndToken(end);
	}
	// add a trivial annotation, which should not be returned.
	Rooted<AnnotationClass> B_anno = domain->createAnnotationClass("B");
	ASSERT_TRUE(domain->validate(logger));

	// check result.
	std::vector<SyntaxDescriptor> stxs = A->getPermittedTokens();
	ASSERT_EQ(3, stxs.size());
	// the field should be first, because A itself should not be collected
	// directly.
	ASSERT_EQ(A_field, stxs[0].descriptor);
	ASSERT_EQ(Tokens::Empty, stxs[0].start);
	ASSERT_EQ(Tokens::Newline, stxs[0].end);
	ASSERT_EQ(Tokens::Empty, stxs[0].shortForm);
	ASSERT_EQ(A, stxs[1].descriptor);
	ASSERT_EQ(Tokens::Indent, stxs[1].start);
	ASSERT_EQ(Tokens::Dedent, stxs[1].end);
	ASSERT_EQ(1, stxs[1].shortForm);
	ASSERT_EQ(A_anno, stxs[2].descriptor);
	ASSERT_EQ(7, stxs[2].start);
	ASSERT_EQ(8, stxs[2].end);
	ASSERT_EQ(Tokens::Empty, stxs[2].shortForm);
}

TEST(StructuredClass, isSubclassOf)
{
	// create an inheritance hierarchy.
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Domain> domain{new Domain(mgr, sys, "inheritance")};
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), {nullptr}, false, true)};
	// first branch
	Rooted<StructuredClass> B{
	    new StructuredClass(mgr, "B", domain, Cardinality::any(), A)};
	Rooted<StructuredClass> C{
	    new StructuredClass(mgr, "C", domain, Cardinality::any(), B)};
	// second branch
	Rooted<StructuredClass> D{
	    new StructuredClass(mgr, "D", domain, Cardinality::any(), A)};
	Rooted<StructuredClass> E{
	    new StructuredClass(mgr, "E", domain, Cardinality::any(), D)};
	Rooted<StructuredClass> F{
	    new StructuredClass(mgr, "F", domain, Cardinality::any(), D)};

	// check function results
	ASSERT_FALSE(A->isSubclassOf(A));
	ASSERT_FALSE(A->isSubclassOf(B));
	ASSERT_FALSE(A->isSubclassOf(C));
	ASSERT_FALSE(A->isSubclassOf(D));
	ASSERT_FALSE(A->isSubclassOf(E));
	ASSERT_FALSE(A->isSubclassOf(F));

	ASSERT_TRUE(B->isSubclassOf(A));
	ASSERT_FALSE(B->isSubclassOf(B));
	ASSERT_FALSE(B->isSubclassOf(C));
	ASSERT_FALSE(B->isSubclassOf(D));
	ASSERT_FALSE(B->isSubclassOf(E));
	ASSERT_FALSE(B->isSubclassOf(F));

	ASSERT_TRUE(C->isSubclassOf(A));
	ASSERT_TRUE(C->isSubclassOf(B));
	ASSERT_FALSE(C->isSubclassOf(C));
	ASSERT_FALSE(C->isSubclassOf(D));
	ASSERT_FALSE(C->isSubclassOf(E));
	ASSERT_FALSE(C->isSubclassOf(F));

	ASSERT_TRUE(D->isSubclassOf(A));
	ASSERT_FALSE(D->isSubclassOf(B));
	ASSERT_FALSE(D->isSubclassOf(C));
	ASSERT_FALSE(D->isSubclassOf(D));
	ASSERT_FALSE(D->isSubclassOf(E));
	ASSERT_FALSE(D->isSubclassOf(F));

	ASSERT_TRUE(E->isSubclassOf(A));
	ASSERT_FALSE(E->isSubclassOf(B));
	ASSERT_FALSE(E->isSubclassOf(C));
	ASSERT_TRUE(E->isSubclassOf(D));
	ASSERT_FALSE(E->isSubclassOf(E));
	ASSERT_FALSE(E->isSubclassOf(F));

	ASSERT_TRUE(F->isSubclassOf(A));
	ASSERT_FALSE(F->isSubclassOf(B));
	ASSERT_FALSE(F->isSubclassOf(C));
	ASSERT_TRUE(F->isSubclassOf(D));
	ASSERT_FALSE(F->isSubclassOf(E));
	ASSERT_FALSE(F->isSubclassOf(F));
}

TEST(Domain, validate)
{
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// start with an easy example: Our book domain should be valid.
	{
		Rooted<Domain> domain = constructBookDomain(mgr, sys, logger);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
	}
	{
		// Even easier: An empty domain should be valid.
		Rooted<Domain> domain{new Domain(mgr, sys, "domain")};
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// if we add a StructureClass it should be valid still.
		Rooted<StructuredClass> base{
		    new StructuredClass(mgr, "myClass", domain)};
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// if we tamper with the name, however, it shouldn't be valid anymore.
		base->setName("");
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		base->setName("my class");
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		base->setName("myClass");
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// Let's add a primitive field (without a primitive type at first)
		Rooted<FieldDescriptor> base_field =
		    base->createPrimitiveFieldDescriptor(nullptr, logger).first;
		// this should not be valid.
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		// but it should be if we set the type.
		base_field->setPrimitiveType(sys->getStringType());
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// add an invalid start token.
		base_field->setStartToken(TokenDescriptor("< + >"));
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		// make it valid.
		base_field->setStartToken(TokenDescriptor("<"));
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// add a subclass for our base class.
		Rooted<StructuredClass> sub{new StructuredClass(mgr, "sub", domain)};
		// this should be valid in itself.
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// and still if we add a superclass.
		sub->setSuperclass(base, logger);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// and still if we remove the subclass from the base class.
		base->removeSubclass(sub, logger);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		ASSERT_EQ(nullptr, sub->getSuperclass());
		// and still if we re-add it.
		base->addSubclass(sub, logger);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		ASSERT_EQ(base, sub->getSuperclass());
		// add a non-primitive field to the child class.
		Rooted<FieldDescriptor> sub_field =
		    sub->createFieldDescriptor(logger).first;
		// this should not be valid because we allow no children.
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		// we should also be able to add a child and make it valid.
		sub_field->addChild(base);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// it should be invalid if we add it twice.
		sub_field->addChild(base);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		// and valid again if we remove it once.
		sub_field->removeChild(base);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// if we set a primitive type it should be invalid
		sub_field->setPrimitiveType(sys->getStringType());
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		// and valid again if we unset it.
		sub_field->setPrimitiveType(nullptr);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// It should be invalid if we set another TREE field.
		Rooted<FieldDescriptor> sub_field2 =
		    sub->createFieldDescriptor(logger, FieldDescriptor::FieldType::TREE,
		                               "test", false).first;
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		// but valid again if we remove it
		sub->removeFieldDescriptor(sub_field2);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
	}
}

TEST(Domain, getAllTokenDescriptors)
{
	// build an ontology with some custom syntax.
	Manager mgr{1};
	Logger logger;
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "ontology")};
	// add one StructuredClass with all tokens set.
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, Cardinality::any(), {nullptr}, true, true)};
	A->setStartToken(TokenDescriptor(Tokens::Indent));
	A->setEndToken(TokenDescriptor(Tokens::Dedent));
	{
		TokenDescriptor sh{"<+>"};
		sh.id = 1;
		A->setShortToken(sh);
	}
	// add a field with one token set.
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	A_field->setEndToken(TokenDescriptor(Tokens::Newline));
	A_field->addChild(A);
	// add an annotation with start and end set.
	Rooted<AnnotationClass> A_anno = domain->createAnnotationClass("A");
	{
		TokenDescriptor start{"<"};
		start.id = 7;
		A_anno->setStartToken(start);
	}
	{
		TokenDescriptor end{">"};
		end.id = 8;
		A_anno->setEndToken(end);
	}
	// add a trivial annotation, which should not be returned.
	Rooted<AnnotationClass> B_anno = domain->createAnnotationClass("B");
	ASSERT_TRUE(domain->validate(logger));

	// check the result.
	std::vector<TokenDescriptor *> tks = domain->getAllTokenDescriptors();

	// A short token
	ASSERT_EQ("<+>", tks[0]->token);
	ASSERT_EQ(1, tks[0]->id);
	ASSERT_FALSE(tks[0]->special);
	// A start token
	ASSERT_EQ("", tks[1]->token);
	ASSERT_EQ(Tokens::Indent, tks[1]->id);
	ASSERT_TRUE(tks[1]->special);
	// A end token
	ASSERT_EQ("", tks[2]->token);
	ASSERT_EQ(Tokens::Dedent, tks[2]->id);
	ASSERT_TRUE(tks[2]->special);
	// A field end token
	ASSERT_EQ("", tks[3]->token);
	ASSERT_EQ(Tokens::Newline, tks[3]->id);
	ASSERT_TRUE(tks[3]->special);
	// A anno start token
	ASSERT_EQ("<", tks[4]->token);
	ASSERT_EQ(7, tks[4]->id);
	ASSERT_FALSE(tks[4]->special);
	// A anno end token
	ASSERT_EQ(">", tks[5]->token);
	ASSERT_EQ(8, tks[5]->id);
	ASSERT_FALSE(tks[5]->special);
}
}