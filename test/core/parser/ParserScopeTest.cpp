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

#include <core/managed/Manager.hpp>
#include <core/model/Node.hpp>
#include <core/parser/ParserScope.hpp>

namespace ousia {

TEST(ParserScope, flags)
{
	Manager mgr;
	ParserScope scope;

	ASSERT_FALSE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.push(new Node{mgr});
	ASSERT_FALSE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.setFlag(ParserFlag::POST_HEAD, true);
	ASSERT_TRUE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.setFlag(ParserFlag::POST_HEAD, false);
	ASSERT_FALSE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.setFlag(ParserFlag::POST_HEAD, true);
	ASSERT_TRUE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.push(new Node{mgr});
	ASSERT_TRUE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.setFlag(ParserFlag::POST_HEAD, false);
	ASSERT_FALSE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.pop();
	ASSERT_TRUE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.pop();
	ASSERT_FALSE(scope.getFlag(ParserFlag::POST_HEAD));
	scope.setFlag(ParserFlag::POST_HEAD, true);
	ASSERT_TRUE(scope.getFlag(ParserFlag::POST_HEAD));
}

}

