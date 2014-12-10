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

#include <core/ResourceLocator.hpp>

#include <sstream>

namespace ousia {

class TestResourceLocator : public ResourceLocator {
public:
	ResourceLocator::Location locate(
	    const std::string &path, const std::string &relativeTo,
	    const ResourceLocator::Type type) const override
	{
		// trivial test implementation.
		return ResourceLocator::Location(true, *this, type, path);
	}

	std::unique_ptr<std::istream> stream(
	    const std::string &location) const override
	{
		// trivial test implementation.
		std::unique_ptr<std::stringstream> ss(new std::stringstream());
		(*ss) << "test";
		return std::move(ss);
	}
};

TEST(ResourceLocator, locate)
{
	TestResourceLocator instance;
	ResourceLocator::Location location =
	    instance.locate("path", "", ResourceLocator::Type::DOMAIN);
	ASSERT_TRUE(location.found);
	ASSERT_EQ(ResourceLocator::Type::DOMAIN, location.type);
	ASSERT_EQ("path", location.location);
}

TEST(ResourceLocator, stream)
{
	TestResourceLocator instance;
	ResourceLocator::Location location =
	    instance.locate("path", "", ResourceLocator::Type::DOMAIN);
	std::unique_ptr<std::istream> is = location.stream();

	std::string str;
	*is >> str;

	ASSERT_EQ("test", str);
}
}
