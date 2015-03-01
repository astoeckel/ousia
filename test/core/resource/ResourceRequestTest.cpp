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

#include <iostream>

#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/parser/Parser.hpp>
#include <core/resource/ResourceRequest.hpp>
#include <core/Registry.hpp>

namespace ousia {

namespace RttiTypes {
extern const Rtti Document;
extern const Rtti Ontology;
extern const Rtti Typesystem;
}

static TerminalLogger logger(std::cerr, true);
// static ConcreteLogger logger;

namespace {
class ModuleParser : public Parser {
protected:
	void doParse(CharReader &reader, ParserContext &ctx) override
	{
		// Stub
	}
};

class DocumentParser : public Parser {
protected:
	void doParse(CharReader &reader, ParserContext &ctx) override
	{
		// Stub
	}
};

struct TestSetup {
	ModuleParser pModule;
	DocumentParser pDocument;
	Registry registry;

	TestSetup()
	{
		registry.registerExtension("ontology", "application/ontology");
		registry.registerExtension("typesystem", "application/typesystem");
		registry.registerExtension("document", "application/document");
		registry.registerParser(
		    {"application/ontology", "application/typesystem"},
		    {&RttiTypes::Ontology, &RttiTypes::Typesystem}, &pModule);
		registry.registerParser({"application/document"},
		                        {&RttiTypes::Document}, &pDocument);
	}
};
}

TEST(ResourceRequest, deduction)
{
	TestSetup t;

	ResourceRequest req("test.ontology", "", "", {&RttiTypes::Ontology});

	ASSERT_TRUE(req.deduce(t.registry, logger));

	ASSERT_EQ("test.ontology", req.getPath());
	ASSERT_EQ("application/ontology", req.getMimetype());
	ASSERT_EQ(RttiSet({&RttiTypes::Ontology}), req.getSupportedTypes());
	ASSERT_EQ(ResourceType::ONTOLOGY, req.getResourceType());
	ASSERT_EQ(&t.pModule, req.getParser());
	ASSERT_EQ(RttiSet({&RttiTypes::Ontology, &RttiTypes::Typesystem}),
	          req.getParserTypes());
}

TEST(ResourceRequest, deductionWithMimetype)
{
	TestSetup t;

	ResourceRequest req("test.ontology", "application/typesystem", "",
	                    {&RttiTypes::Typesystem});

	ASSERT_TRUE(req.deduce(t.registry, logger));

	ASSERT_EQ("test.ontology", req.getPath());
	ASSERT_EQ("application/typesystem", req.getMimetype());
	ASSERT_EQ(RttiSet({&RttiTypes::Typesystem}), req.getSupportedTypes());
	ASSERT_EQ(ResourceType::TYPESYSTEM, req.getResourceType());
	ASSERT_EQ(&t.pModule, req.getParser());
	ASSERT_EQ(RttiSet({&RttiTypes::Ontology, &RttiTypes::Typesystem}),
	          req.getParserTypes());
}

TEST(ResourceRequest, deductionWithUnknownResourceType)
{
	TestSetup t;

	ResourceRequest req("test.ontology", "", "",
	                    {&RttiTypes::Ontology, &RttiTypes::Typesystem});

	ASSERT_TRUE(req.deduce(t.registry, logger));

	ASSERT_EQ("test.ontology", req.getPath());
	ASSERT_EQ("application/ontology", req.getMimetype());
	ASSERT_EQ(RttiSet({&RttiTypes::Ontology, &RttiTypes::Typesystem}),
	          req.getSupportedTypes());
	ASSERT_EQ(ResourceType::UNKNOWN, req.getResourceType());
	ASSERT_EQ(&t.pModule, req.getParser());
	ASSERT_EQ(RttiSet({&RttiTypes::Ontology, &RttiTypes::Typesystem}),
	          req.getParserTypes());
}

TEST(ResourceRequest, deductionWithRel)
{
	TestSetup t;

	ResourceRequest req("test.ontology", "", "ontology",
	                    {&RttiTypes::Ontology, &RttiTypes::Typesystem});

	ASSERT_TRUE(req.deduce(t.registry, logger));

	ASSERT_EQ("test.ontology", req.getPath());
	ASSERT_EQ("application/ontology", req.getMimetype());
	ASSERT_EQ(RttiSet({&RttiTypes::Ontology}), req.getSupportedTypes());
	ASSERT_EQ(ResourceType::ONTOLOGY, req.getResourceType());
	ASSERT_EQ(&t.pModule, req.getParser());
	ASSERT_EQ(RttiSet({&RttiTypes::Ontology, &RttiTypes::Typesystem}),
	          req.getParserTypes());
}
}

