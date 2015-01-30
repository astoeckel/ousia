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
#include <core/model/Project.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/StandaloneEnvironment.hpp>

#include <plugins/filesystem/FileLocator.hpp>
#include <plugins/xml/XmlParser.hpp>

namespace ousia {

namespace RttiTypes {
extern const Rtti Document;
extern const Rtti Domain;
extern const Rtti Typesystem;
}

struct XmlStandaloneEnvironment : public StandaloneEnvironment {
	XmlParser xmlParser;
	FileLocator fileLocator;

	XmlStandaloneEnvironment(ConcreteLogger &logger)
	    : StandaloneEnvironment(logger)
	{
		fileLocator.addUnittestSearchPath("xmlparser");

		registry.registerDefaultExtensions();
		registry.registerParser(
		    {"text/vnd.ousia.oxm", "text/vnd.ousia.oxd"},
		    {&RttiTypes::Document, &RttiTypes::Typesystem, &RttiTypes::Domain},
		    &xmlParser);
		registry.registerResourceLocator(&fileLocator);
	}
};

static TerminalLogger logger(std::cerr, true);

TEST(XmlParser, mismatchedTag)
{
	XmlStandaloneEnvironment env(logger);
	env.parse("mismatchedTag.oxm", "", "", RttiSet{&RttiTypes::Document});
	ASSERT_TRUE(logger.hasError());
}

TEST(XmlParser, generic)
{
	XmlStandaloneEnvironment env(logger);
	env.parse("generic.oxm", "", "", RttiSet{&RttiTypes::Node});
#ifdef MANAGER_GRAPHVIZ_EXPORT
	env.manager.exportGraphviz("xmlDocument.dot");
#endif
}
}

