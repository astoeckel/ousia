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

#include <core/common/CharReader.hpp>
#include <core/common/SourceContextReader.hpp>
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Typesystem.hpp>
#include <core/model/Node.hpp>
#include <core/model/Project.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/StandaloneEnvironment.hpp>

#include <plugins/filesystem/FileLocator.hpp>
#include <formats/osml/OsmlParser.hpp>

namespace ousia {

namespace RttiTypes {
extern const Rtti Document;
extern const Rtti Domain;
extern const Rtti Typesystem;
}

struct OsmlStandaloneEnvironment : public StandaloneEnvironment {
	OsmlParser parser;
	FileLocator fileLocator;

	OsmlStandaloneEnvironment(ConcreteLogger &logger)
	    : StandaloneEnvironment(logger)
	{
		fileLocator.addDefaultSearchPaths();
		fileLocator.addUnittestSearchPath("osmlparser");

		registry.registerDefaultExtensions();
		registry.registerParser({"text/vnd.ousia.osml"}, {&RttiTypes::Node},
		                        &parser);
		registry.registerResourceLocator(&fileLocator);
	}
};

static TerminalLogger logger(std::cerr, true);

TEST(OsmlParser, empty_document)
{
	OsmlStandaloneEnvironment env(logger);
	Rooted<Node> node =
	    env.parse("empty_document.osml", "", "", RttiSet{&RttiTypes::Node});

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, empty_domain)
{
	OsmlStandaloneEnvironment env(logger);
	Rooted<Node> node =
	    env.parse("empty_domain.osml", "", "", RttiSet{&RttiTypes::Node});

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Domain));
	ASSERT_EQ("testDomain", node->getName());
}

TEST(OsmlParser, empty_typesystem)
{
	OsmlStandaloneEnvironment env(logger);
	Rooted<Node> node =
	    env.parse("empty_typesystem.osml", "", "", RttiSet{&RttiTypes::Node});

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Typesystem));
	ASSERT_EQ("testTypesystem", node->getName());
}
}

