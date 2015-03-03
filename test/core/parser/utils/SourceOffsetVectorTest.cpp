/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#include <core/parser/utils/SourceOffsetVector.hpp>

namespace ousia {

TEST(SourceOffsetVector, simple)
{
	SourceOffsetVector vec;

	for (size_t i = 0; i < 1000; i++) {
		vec.storeOffset(i * 3 + 5, (i + 1) * 3 + 5);
	}

	for (size_t i = 0; i < 1000; i++) {
		auto elem = vec.loadOffset(i);
		EXPECT_EQ(i * 3 + 5, elem.first);
		EXPECT_EQ((i + 1) * 3 + 5, elem.second);
	}
	auto elem = vec.loadOffset(1000);
	EXPECT_EQ(1000U * 3 + 5, elem.first);
	EXPECT_EQ(1000U * 3 + 5, elem.second);
}

TEST(SourceOffsetVector, gaps)
{
	SourceOffsetVector vec;

	for (size_t i = 0; i < 1000; i++) {
		vec.storeOffset(i * 3 + 5, i * 3 + 7);
	}

	for (size_t i = 0; i < 999; i++) {
		auto elem = vec.loadOffset(i);
		EXPECT_EQ(i * 3 + 5, elem.first);
		EXPECT_EQ(i * 3 + 7, elem.second);
	}
	auto elem = vec.loadOffset(999);
	EXPECT_EQ(999U * 3 + 5, elem.first);
	EXPECT_EQ(999U * 3 + 7, elem.second);

	elem = vec.loadOffset(1000);
	EXPECT_EQ(999U * 3 + 7, elem.first);
	EXPECT_EQ(999U * 3 + 7, elem.second);
}
}
