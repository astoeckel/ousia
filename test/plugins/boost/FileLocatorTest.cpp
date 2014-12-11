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

#include <plugins/boost/FileLocator.hpp>

namespace ousia {
TEST(FileLocator, testAddSearchPath)
{
	FileLocator instance;
	ASSERT_EQ(0, instance.getSearchPaths().size());

	// Add one path for three types.
	instance.addSearchPath(
	    ".", {ResourceLocator::Type::DOMAIN_DESC, ResourceLocator::Type::SCRIPT,
	          ResourceLocator::Type::TYPESYS});

	ASSERT_EQ(3, instance.getSearchPaths().size());

	auto it =
	    instance.getSearchPaths().find(ResourceLocator::Type::DOMAIN_DESC);

	ASSERT_EQ(1, it->second.size());
	ASSERT_EQ(".", it->second[0].generic_string());

	it = instance.getSearchPaths().find(ResourceLocator::Type::SCRIPT);

	ASSERT_EQ(1, it->second.size());
	ASSERT_EQ(".", it->second[0].generic_string());

	it = instance.getSearchPaths().find(ResourceLocator::Type::TYPESYS);

	ASSERT_EQ(1, it->second.size());
	ASSERT_EQ(".", it->second[0].generic_string());

	// Add another path for only one of those types.

	instance.addSearchPath("..", {ResourceLocator::Type::DOMAIN_DESC});

	ASSERT_EQ(3, instance.getSearchPaths().size());

	auto it =
	    instance.getSearchPaths().find(ResourceLocator::Type::DOMAIN_DESC);

	ASSERT_EQ(2, it->second.size());
	ASSERT_EQ(".", it->second[0].generic_string());
	ASSERT_EQ("..", it->second[1].generic_string());
}
}
