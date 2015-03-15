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
#include <core/model/Ontology.hpp>
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
extern const Rtti Ontology;
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

TEST(OsmlParser, emptyDocument)
{
	OsmlStandaloneEnvironment env(logger);
	Rooted<Node> node =
	    env.parse("empty_document.osml", "", "", RttiSet{&RttiTypes::Node});

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, emptyOntology)
{
	OsmlStandaloneEnvironment env(logger);
	Rooted<Node> node =
	    env.parse("empty_ontology.osml", "", "", RttiSet{&RttiTypes::Node});

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Ontology));
	ASSERT_EQ("testOntology", node->getName());
}

TEST(OsmlParser, emptyTypesystem)
{
	OsmlStandaloneEnvironment env(logger);
	Rooted<Node> node =
	    env.parse("empty_typesystem.osml", "", "", RttiSet{&RttiTypes::Node});

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Typesystem));
	ASSERT_EQ("testTypesystem", node->getName());
}

TEST(OsmlParser, rollbackOnInvalidElement)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	ASSERT_FALSE(logger.hasError());
	Rooted<Node> node = env.parse("rollback_on_invalid_element.osml", "", "",
	                              RttiSet{&RttiTypes::Node});
	ASSERT_TRUE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, inlineOntology)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node =
	    env.parse("inline_ontology.osml", "", "", RttiSet{&RttiTypes::Node});
	ASSERT_FALSE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, include)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node =
	    env.parse("include_root.osml", "", "", RttiSet{&RttiTypes::Node});
	ASSERT_FALSE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, includeRecursive)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	ASSERT_FALSE(logger.hasError());
	Rooted<Node> node = env.parse("include_recursive_root.osml", "", "",
	                              RttiSet{&RttiTypes::Node});
	ASSERT_TRUE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, structureInheritance)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node = env.parse("structure_inheritance.osml", "", "",
	                              RttiSet{&RttiTypes::Node});
	ASSERT_FALSE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Ontology));
}

TEST(OsmlParser, structWithNoField)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node = env.parse("struct_with_no_field.osml", "", "",
	                              RttiSet{&RttiTypes::Node});
	ASSERT_FALSE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, invalidExplicitFields)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	ASSERT_FALSE(logger.hasError());
	Rooted<Node> node = env.parse("invalid_explicit_fields.osml", "", "",
	                              RttiSet{&RttiTypes::Node});
	ASSERT_TRUE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, explicitFields)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node =
	    env.parse("explicit_fields.osml", "", "", RttiSet{&RttiTypes::Node});
	ASSERT_FALSE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, simpleAnnotation)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node =
	    env.parse("simple_annotation.osml", "", "", RttiSet{&RttiTypes::Node});
	ASSERT_FALSE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, undefinedAnnotation)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node =
	    env.parse("undefined_annotation.osml", "", "", RttiSet{&RttiTypes::Node});
	ASSERT_TRUE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, overlappingAnnotations)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node = env.parse("overlapping_annotations.osml", "", "",
	                              RttiSet{&RttiTypes::Node});
	ASSERT_FALSE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, errorAnnotationBoundaries)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node = env.parse("error_annotation_boundaries.osml", "", "",
	                              RttiSet{&RttiTypes::Node});
	ASSERT_TRUE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Document));
}

TEST(OsmlParser, syntaxDescription)
{
	OsmlStandaloneEnvironment env(logger);
	logger.reset();

	Rooted<Node> node =
	    env.parse("syntax_description.osml", "", "", RttiSet{&RttiTypes::Node});
	ASSERT_FALSE(logger.hasError());

	ASSERT_TRUE(node != nullptr);
	ASSERT_TRUE(node->isa(&RttiTypes::Ontology));

	Rooted<Ontology> ontology = node.cast<Ontology>();
	auto nodes = ontology->resolve(&RttiTypes::StructuredClass, "b");
	ASSERT_EQ(1U, nodes.size());

	Rooted<StructuredClass> structure = nodes[0].node.cast<StructuredClass>();
	auto descrs = structure->getFieldDescriptors();
	ASSERT_EQ(2U, descrs.size());

	ASSERT_EQ("f1", descrs[0]->getName());
	ASSERT_EQ("f2", descrs[1]->getName());

	ASSERT_FALSE(descrs[0]->getOpenToken().special);
	ASSERT_EQ("=", descrs[0]->getOpenToken().token);

	ASSERT_EQ(Tokens::Newline, descrs[0]->getCloseToken().id);
	ASSERT_TRUE(descrs[0]->getCloseToken().special);
	ASSERT_EQ("", descrs[0]->getCloseToken().token);

	ASSERT_EQ(WhitespaceMode::PRESERVE, descrs[0]->getWhitespaceMode());

	ASSERT_FALSE(descrs[1]->getOpenToken().special);
	ASSERT_EQ("++", descrs[1]->getOpenToken().token);

	ASSERT_FALSE(descrs[1]->getCloseToken().special);
	ASSERT_EQ("--", descrs[1]->getCloseToken().token);

	ASSERT_EQ(WhitespaceMode::COLLAPSE, descrs[1]->getWhitespaceMode());

	ASSERT_FALSE(structure->getShortToken().special);
	ASSERT_EQ("~", structure->getShortToken().token);
}
}

