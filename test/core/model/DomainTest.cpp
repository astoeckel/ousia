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
#include <core/model/Domain.hpp>

#include "TestDomain.hpp"

namespace ousia {
namespace model {

void assert_path(const ResolutionResult &res, const RttiType &expected_type,
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
	res = domain->resolve("book", typeOf<Domain>());
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], typeOf<Domain>(), {"book"});

	// There is one domain called "book"
	res = domain->resolve("book", typeOf<StructuredClass>());
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], typeOf<StructuredClass>(), {"book", "book"});

	// If we explicitly ask for the "book, book" path, then only the
	// StructuredClass should be returned.
	res = domain->resolve(std::vector<std::string>{"book", "book"},
	                      typeOf<Domain>());
	ASSERT_EQ(0U, res.size());

	res = domain->resolve(std::vector<std::string>{"book", "book"},
	                      typeOf<StructuredClass>());
	ASSERT_EQ(1U, res.size());

	// If we ask for "section" the result should be unique as well.
	res = domain->resolve("section", typeOf<StructuredClass>());
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], typeOf<StructuredClass>(), {"book", "section"});

	// If we ask for "paragraph" it is referenced two times in the Domain graph,
	// but should be returned only once.
	res = domain->resolve("paragraph", typeOf<StructuredClass>());
	ASSERT_EQ(1U, res.size());
	assert_path(res[0], typeOf<StructuredClass>(), {"book", "paragraph"});
}

Rooted<StructuredClass> getClass(const std::string name, Handle<Domain> dom)
{
	std::vector<ResolutionResult> res =
	    dom->resolve(name, RttiTypes::StructuredClass);
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
	// Now we build a really nasty domain with lots of transparency
	// and inheritance
	Logger logger;
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the domain.
	Rooted<Domain> domain {new Domain(mgr, sys, "nasty")};
	Cardinality any;
	any.merge(Range<size_t>::typeRangeFrom(0));
	
	// Our root class A
	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", domain, any, {nullptr}, {nullptr}, false, true)};
	domain->addStructuredClass(A);
	// We also create a field for it.
	Rooted<FieldDescriptor> A_field{new FieldDescriptor(mgr, A)};
	A->addFieldDescriptor(A_field);
	
	// our first transparent child B
	Rooted<StructuredClass> B{new StructuredClass(
	    mgr, "B", domain, any, {nullptr}, {nullptr}, true)};
	A_field->addChild(B);
	
	//TODO: Continue
}

}
}
