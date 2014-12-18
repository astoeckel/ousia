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

#include <core/model/Domain.hpp>

#include "ModelTestUtils.hpp"

namespace ousia {
namespace model {

void assert_path(std::vector<Rooted<Managed>> &result, size_t idx,
                 const std::type_info& expected_type,
                 std::vector<std::string> expected_path)
{
	ASSERT_TRUE(result.size() > idx);
	// check class/type
	ASSERT_EQ(expected_type, typeid(*result[idx]));
	// transform to node
	Managed *m = &(*result[idx]);
	Node *n = dynamic_cast<Node *>(m);
	ASSERT_TRUE(n);
	// extract actual path
	std::vector<std::string> actual_path = n->path();
	// check path
	ASSERT_EQ(expected_path, actual_path);
}

TEST(Document, testDomainResolving)
{
	// Construct Manager
	Manager mgr{1};
	// Get the domain.
	Rooted<Domain> domain = constructBookDomain(mgr);

	/*
	 * Start with the "book" search keyword. This should resolve to the domain
	 * itself (because it is called "book"), as well as the structure "book"
	 * node.
	 */
	std::vector<Rooted<Managed>> res =
	    domain->resolve(std::vector<std::string>{"book"});
	// First we expect the book domain.
	assert_path(res, 0, typeid(Domain), {"book"});
	// Then the book structure.
	assert_path(res, 1, typeid(StructuredClass), {"book", "book"});
}
}
}
