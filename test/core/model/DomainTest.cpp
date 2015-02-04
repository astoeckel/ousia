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

void assert_path(const ResolutionResult &res, const Rtti &expected_type,
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
	res = domain->resolve(RttiTypes::Domain, "book");
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], RttiTypes::Domain, {"book"});

	// There is one domain called "book"
	res = domain->resolve(RttiTypes::StructuredClass, "book");
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], RttiTypes::StructuredClass, {"book", "book"});

	// If we explicitly ask for the "book, book" path, then only the
	// StructuredClass should be returned.
	res = domain->resolve(RttiTypes::Domain,
	                      std::vector<std::string>{"book", "book"});
	ASSERT_EQ(0U, res.size());

	res = domain->resolve(RttiTypes::StructuredClass,
	                      std::vector<std::string>{"book", "book"});
	ASSERT_EQ(1U, res.size());

	// If we ask for "section" the result should be unique as well.
	res = domain->resolve(RttiTypes::StructuredClass, "section");
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], RttiTypes::StructuredClass, {"book", "section"});

	// If we ask for "paragraph" it is referenced two times in the Domain graph,
	// but should be returned only once.
	res = domain->resolve(RttiTypes::StructuredClass, "paragraph");
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], RttiTypes::StructuredClass, {"book", "paragraph"});
}

Rooted<StructuredClass> getClass(const std::string name, Handle<Domain> dom)
{
	std::vector<ResolutionResult> res =
	    dom->resolve(RttiTypes::StructuredClass, name);
	return res[0].node.cast<StructuredClass>();
}

TEST(Descriptor, pathTo)
{
	// Start with some easy examples from the book domain.
	Logger logger;
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the domain.
	Rooted<Domain> domain = constructBookDomain(mgr, sys, logger);

	// get the book node and the section node.
	Rooted<StructuredClass> book = getClass("book", domain);
	Rooted<StructuredClass> section = getClass("section", domain);
	// get the path in between.
	std::vector<Rooted<Node>> path = book->pathTo(section);
	ASSERT_EQ(1U, path.size());
	ASSERT_TRUE(path[0]->isa(RttiTypes::FieldDescriptor));

	// get the text node.
	Rooted<StructuredClass> text = getClass("text", domain);
	// get the path between book and text via paragraph.
	path = book->pathTo(text);
	ASSERT_EQ(3U, path.size());
	ASSERT_TRUE(path[0]->isa(RttiTypes::FieldDescriptor));
	ASSERT_TRUE(path[1]->isa(RttiTypes::StructuredClass));
	ASSERT_EQ("paragraph", path[1]->getName());
	ASSERT_TRUE(path[2]->isa(RttiTypes::FieldDescriptor));

	// get the subsection node.
	Rooted<StructuredClass> subsection = getClass("subsection", domain);
	// try to get the path between book and subsection.
	path = book->pathTo(subsection);
	// this should be impossible.
	ASSERT_EQ(0U, path.size());
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
	 * 2.) A has the target as child in the default field, but the default
	 *     field is overridden in the start class.
	 * 3.) A has B as child in another field.
	 * 4.) B is transparent and has no children (but C as subclass)
	 * 5.) C is a subclass of B, transparent and has
	 *     the target as child (shortest path).
	 * 6.) start has D as child in the default field.
	 * 7.) D is transparent has E as child in the default field.
	 * 8.) E is transparent and has target as child in the default field
	 *     (longer path)
	 *
	 * So the path start_field , E , E_field should be returned.
	 */
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct the domain
	Rooted<Domain> domain{new Domain(mgr, sys, "nasty")};

	// Let's create the classes that we need first
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, AnyCardinality, {nullptr}, {nullptr}, false, true)};

	Rooted<StructuredClass> start{new StructuredClass(
	    mgr, "start", domain, AnyCardinality, {nullptr}, A, false, false)};

	Rooted<StructuredClass> B{new StructuredClass(
	    mgr, "B", domain, AnyCardinality, {nullptr}, {nullptr}, true, false)};

	Rooted<StructuredClass> C{new StructuredClass(
	    mgr, "C", domain, AnyCardinality, {nullptr}, B, true, false)};

	Rooted<StructuredClass> D{new StructuredClass(
	    mgr, "D", domain, AnyCardinality, {nullptr}, {nullptr}, true, false)};

	Rooted<StructuredClass> E{new StructuredClass(
	    mgr, "E", domain, AnyCardinality, {nullptr}, {nullptr}, true, false)};

	Rooted<StructuredClass> target{
	    new StructuredClass(mgr, "target", domain, AnyCardinality)};

	// We create two fields for A
	Rooted<FieldDescriptor> A_field{new FieldDescriptor(mgr, A)};

	A_field->addChild(target);
	Rooted<FieldDescriptor> A_field2{new FieldDescriptor(
	    mgr, A, FieldDescriptor::FieldType::SUBTREE, "second")};

	A_field2->addChild(B);
	// We create no field for B
	// One for C
	Rooted<FieldDescriptor> C_field{new FieldDescriptor(mgr, C)};

	C_field->addChild(target);
	// one for start
	Rooted<FieldDescriptor> start_field{new FieldDescriptor(mgr, start)};

	start_field->addChild(D);
	// One for D
	Rooted<FieldDescriptor> D_field{new FieldDescriptor(mgr, D)};

	D_field->addChild(E);
	// One for E
	Rooted<FieldDescriptor> E_field{new FieldDescriptor(mgr, E)};

	E_field->addChild(target);

#ifdef MANAGER_GRAPHVIZ_EXPORT
	// dump the manager state
	mgr.exportGraphviz("nastyDomain.dot");
#endif

	// and now we should be able to find the shortest path as suggested
	std::vector<Rooted<Node>> path = start->pathTo(target);
	ASSERT_EQ(3U, path.size());
	ASSERT_TRUE(path[0]->isa(RttiTypes::FieldDescriptor));
	ASSERT_EQ("second", path[0]->getName());
	ASSERT_TRUE(path[1]->isa(RttiTypes::StructuredClass));
	ASSERT_EQ("B", path[1]->getName());
	ASSERT_TRUE(path[2]->isa(RttiTypes::FieldDescriptor));
	ASSERT_EQ("$default", path[2]->getName());
}

TEST(StructuredClass, isSubclassOf)
{
	// create an inheritance hierarchy.
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Domain> domain{new Domain(mgr, sys, "inheritance")};
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, AnyCardinality, {nullptr}, {nullptr}, false, true)};
	// first branch
	Rooted<StructuredClass> B{
	    new StructuredClass(mgr, "B", domain, AnyCardinality, {nullptr}, A)};
	Rooted<StructuredClass> C{
	    new StructuredClass(mgr, "C", domain, AnyCardinality, {nullptr}, B)};
	// second branch
	Rooted<StructuredClass> D{
	    new StructuredClass(mgr, "D", domain, AnyCardinality, {nullptr}, A)};
	Rooted<StructuredClass> E{
	    new StructuredClass(mgr, "E", domain, AnyCardinality, {nullptr}, D)};
	Rooted<StructuredClass> F{
	    new StructuredClass(mgr, "F", domain, AnyCardinality, {nullptr}, D)};

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
		Rooted<FieldDescriptor> base_field{
		    new FieldDescriptor(mgr, base, nullptr)};
		// this should not be valid.
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		// but it should be if we set the type.
		base_field->setPrimitiveType(sys->getStringType());
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// not anymore, however, if we tamper with the FieldType.
		base_field->setFieldType(FieldDescriptor::FieldType::TREE);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		base_field->setFieldType(FieldDescriptor::FieldType::PRIMITIVE);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// add a subclass for our base class.
		Rooted<StructuredClass> sub{new StructuredClass(mgr, "sub", domain)};
		// this should be valid in itself.
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// and still if we add a superclass.
		sub->setSuperclass(base);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// and still we we remove the subclass from the base class.
		base->removeSubclass(sub);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		ASSERT_EQ(nullptr, sub->getSuperclass());
		// and still if we re-add it.
		base->addSubclass(sub);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		ASSERT_EQ(base, sub->getSuperclass());
		// add a non-primitive field to the child class.
		Rooted<FieldDescriptor> sub_field{new FieldDescriptor(mgr, sub)};
		// this should be valid
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// .. until we set a primitive type.
		sub_field->setPrimitiveType(sys->getStringType());
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_FALSE(domain->validate(logger));
		// and valid again if we unset it.
		sub_field->setPrimitiveType(nullptr);
		ASSERT_EQ(ValidationState::UNKNOWN, domain->getValidationState());
		ASSERT_TRUE(domain->validate(logger));
		// we should also be able to add a child and have it still be valid.
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
	}
}
}
