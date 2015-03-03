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

#include <core/parser/stack/Callbacks.hpp>
#include <core/parser/stack/TokenRegistry.hpp>

namespace ousia {
namespace parser_stack {

class ParserCallbacksProxy : public ParserCallbacks {
public:
	size_t registerTokenCount = 0;
	size_t unregisterTokenCount = 0;

	TokenId registerToken(const std::string &token) override
	{
		registerTokenCount++;
		return registerTokenCount;
	}

	void unregisterToken(TokenId id) override { unregisterTokenCount++; }
};

TEST(TokenRegistry, simple)
{
	ParserCallbacksProxy parser;
	{
		TokenRegistry registry(parser);

		ASSERT_EQ(0U, parser.registerTokenCount);
		ASSERT_EQ(0U, parser.unregisterTokenCount);

		ASSERT_EQ(1U, registry.registerToken("test"));
		ASSERT_EQ(1U, registry.registerToken("test"));
		ASSERT_EQ(2U, registry.registerToken("test2"));
		ASSERT_EQ(2U, registry.registerToken("test2"));
		ASSERT_EQ(3U, registry.registerToken("test3"));
		ASSERT_EQ(3U, parser.registerTokenCount);
		ASSERT_EQ(0U, parser.unregisterTokenCount);

		registry.unregisterToken(1);
		ASSERT_EQ(3U, parser.registerTokenCount);
		ASSERT_EQ(0U, parser.unregisterTokenCount);

		registry.unregisterToken(1);
		ASSERT_EQ(3U, parser.registerTokenCount);
		ASSERT_EQ(1U, parser.unregisterTokenCount);

		registry.unregisterToken(1);
		ASSERT_EQ(3U, parser.registerTokenCount);
		ASSERT_EQ(1U, parser.unregisterTokenCount);

		registry.unregisterToken(2);
		ASSERT_EQ(3U, parser.registerTokenCount);
		ASSERT_EQ(1U, parser.unregisterTokenCount);

		registry.unregisterToken(2);
		ASSERT_EQ(3U, parser.registerTokenCount);
		ASSERT_EQ(2U, parser.unregisterTokenCount);
	}
	ASSERT_EQ(3U, parser.unregisterTokenCount);
}
}
}

