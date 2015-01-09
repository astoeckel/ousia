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

void assert_path(const ResolutionResult &res, const RttiBase &expected_type,
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
}
}
