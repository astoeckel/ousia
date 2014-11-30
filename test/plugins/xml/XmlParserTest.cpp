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

#include <iostream>

#include <gtest/gtest.h>

#include <plugins/xml/XmlParser.hpp>

namespace ousia {
namespace parser {
namespace xml {

struct TestParserContext : public ParserContext {

private:
	Logger log;
	Registry r;
	Scope s;

public:
	TestParserContext() : ParserContext(s, r, log), r(log), s(nullptr) {};

};

TEST(XmlParser, mismatchedTagException)
{
	TestParserContext ctx;
	XmlParser p;

	bool hadException = false;
	try {
		p.parse("<test foo=\"bar\">data<![CDATA[bla]]>\n</btest>", ctx);
	}
	catch (ParserException ex) {
		ASSERT_EQ(2, ex.line);
		ASSERT_FALSE(ex.fatal);
		hadException = true;
	}
	ASSERT_TRUE(hadException);
}

const char* TEST_DATA = "<?xml version=\"1.0\" standalone=\"yes\"?>\n"
	"<document a:bc=\"b\">\n"
	"	<bla:test xmlAttr=\"blub\" />\n"
	"</document>\n";

TEST(XmlParser, namespaces)
{
	TestParserContext ctx;
	XmlParser p;

	p.parse(TEST_DATA, ctx);
}

}
}
}

