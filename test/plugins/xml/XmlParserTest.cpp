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

#include <core/common/Logger.hpp>

#include <plugins/xml/XmlParser.hpp>

namespace ousia {
namespace parser {
namespace xml {

TerminalLogger logger(std::cerr, true);

TEST(XmlParser, mismatchedTagException)
{
	StandaloneParserContext ctx;
	XmlParser p;

	bool hadException = false;
	try {
		p.parse("<document>\n</document2>", ctx);
	}
	catch (LoggableException ex) {
		ASSERT_EQ(2, ex.loc.line);
		hadException = true;
	}
	ASSERT_TRUE(hadException);
}

const char *TEST_DATA =
    "<?xml version=\"1.0\" standalone=\"yes\"?>\n"
    "<document a:bc=\"b\">\n"
    "	<head>\n"
    "		<typesystem name=\"color\">\n"
    "			<types>\n"
    "				<struct name=\"color\">\n"
    "				</struct>\n"
    "			</type>\n"
    "		</typesystem>\n"
    "	</head>\n"
    "	<body xmlAttr=\"blub\">\n"
    "		<book>Dies ist ein Test&gt;</book>\n"
    "	</body>\n"
    "</document>\n";

TEST(XmlParser, namespaces)
{
	StandaloneParserContext ctx(logger);
	XmlParser p;
	CharReader reader(TEST_DATA);
	{
		ScopedLogger sl(logger, "test.oxd", SourceLocation{},
		                CharReader::contextCallback, &reader);
		try {
			p.parse(TEST_DATA, ctx);
		}
		catch (LoggableException ex) {
			logger.log(ex);
		}
	}
}
}
}
}

