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

#include <core/resource/ResourceLocator.hpp>

namespace ousia {

TEST(StaticResourceLocator, locate)
{
	StaticResourceLocator locator;
	locator.store("path", "test");

	Resource res;
	ASSERT_TRUE(locator.locate(res, "path"));
	ASSERT_TRUE(res.isValid());
	ASSERT_EQ(ResourceType::UNKNOWN, res.getType());
	ASSERT_EQ("path", res.getLocation());
}

TEST(StaticResourceLocator, stream)
{
	StaticResourceLocator locator;
	locator.store("path", "test");

	Resource res;
	ASSERT_TRUE(locator.locate(res, "path"));

	auto is = res.stream();

	std::string str;
	*is >> str;

	ASSERT_EQ("test", str);
}
}
