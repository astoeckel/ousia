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

#include <sstream>

#include <core/resource/ResourceLocator.hpp>
#include <core/Registry.hpp>

namespace ousia {

TEST(Registry, locateResource)
{
	StaticResourceLocator locator;
	locator.store("path", "test");

	Registry registry;
	registry.registerResourceLocator(locator);

	Resource res;
	ASSERT_TRUE(
	    registry.locateResource(res, "path", ResourceType::DOMAIN_DESC));
	ASSERT_TRUE(res.isValid());
	ASSERT_EQ(ResourceType::DOMAIN_DESC, res.getType());
	ASSERT_EQ("path", res.getLocation());
}
}
