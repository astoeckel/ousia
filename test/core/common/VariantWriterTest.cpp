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

#include <sstream>
#include <gtest/gtest.h>

#include <core/common/Variant.hpp>
#include <core/common/VariantWriter.hpp>

namespace ousia {

TEST(VariantWriter, writeJsonPretty)
{
	Variant v{Variant::mapType{{"a", "this is\na\ntest\""},
	                           {"b", 1},
	                           {"c", Variant::arrayType{1, 2, 3}}}};
	std::stringstream stream;
	VariantWriter::writeJson(v, stream, true);
	ASSERT_EQ(
	    "{\n\t\"a\": \"this is\\na\\ntest\\\"\",\n\t\"b\": 1,\n\t\"c\": "
	    "[\n\t\t1,\n\t\t2,\n\t\t3\n\t]\n}",
	    stream.str());
}

TEST(VariantWriter, writeJson)
{
	Variant v{Variant::mapType{{"a", "this is\na\ntest\""},
	                           {"b", 1},
	                           {"c", Variant::arrayType{1, 2, 3}}}};
	std::stringstream stream;
	VariantWriter::writeJson(v, stream, false);
	ASSERT_EQ(
	    "{\"a\":\"this is\\na\\ntest\\\"\",\"b\":1,\"c\":[1,2,3]}",
	    stream.str());
}

}

