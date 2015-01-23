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

#include <core/common/Exceptions.hpp>
#include <core/parser/Parser.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/resource/ResourceLocator.hpp>
#include <core/Registry.hpp>

namespace ousia {

namespace {
class TestParser : public Parser {
protected:
	Rooted<Node> doParse(CharReader &reader, ParserContext &ctx) override
	{
		return new Node{ctx.manager};
	}
};
}

static const Rtti rtti1{"rtti1"};
static const Rtti rtti2{"rtti2"};

TEST(Registry, parsers)
{
	Registry registry;

	TestParser parser1;
	TestParser parser2;

	registry.registerParser({"text/vnd.ousia.oxm", "text/vnd.ousia.oxd"},
	                        {&rtti1, &rtti2}, &parser1);
	registry.registerParser({"text/vnd.ousia.opd"}, {&rtti2}, &parser2);

	ASSERT_THROW(
	    registry.registerParser({"text/vnd.ousia.opd"}, {&rtti2}, &parser1),
	    OusiaException);

	{
		auto res = registry.getParserForMimetype("text/vnd.ousia.oxm");
		ASSERT_EQ(&parser1, res.first);
		ASSERT_EQ(RttiSet({&rtti1, &rtti2}), res.second);
	}

	{
		auto res = registry.getParserForMimetype("text/vnd.ousia.opd");
		ASSERT_EQ(&parser2, res.first);
		ASSERT_EQ(RttiSet({&rtti2}), res.second);
	}

	{
		auto res = registry.getParserForMimetype("application/javascript");
		ASSERT_EQ(nullptr, res.first);
		ASSERT_EQ(RttiSet({}), res.second);
	}
}

TEST(Registry, extensions)
{
	Registry registry;

	registry.registerExtension("oxm", "text/vnd.ousia.oxm");
	registry.registerExtension("oxd", "text/vnd.ousia.oxd");
	ASSERT_EQ("text/vnd.ousia.oxm", registry.getMimetypeForExtension("oxm"));
	ASSERT_EQ("text/vnd.ousia.oxm", registry.getMimetypeForExtension("OXM"));
	ASSERT_EQ("text/vnd.ousia.oxd", registry.getMimetypeForExtension("OxD"));
	ASSERT_EQ("", registry.getMimetypeForExtension("pdf"));

	ASSERT_THROW(registry.registerExtension("oxm", "text/vnd.ousia.oxm"),
	             OusiaException);
}

TEST(Registry, locateResource)
{
	StaticResourceLocator locator;
	locator.store("path", "test");

	Registry registry;
	registry.registerResourceLocator(&locator);

	Resource res;
	ASSERT_TRUE(
	    registry.locateResource(res, "path", ResourceType::DOMAIN_DESC));
	ASSERT_TRUE(res.isValid());
	ASSERT_EQ(ResourceType::DOMAIN_DESC, res.getType());
	ASSERT_EQ("path", res.getLocation());
}
}
