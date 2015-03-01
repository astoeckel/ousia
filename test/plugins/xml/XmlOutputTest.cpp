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
#include <sstream>

#include <plugins/xml/XmlOutput.hpp>

#include <core/common/Rtti.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/model/Document.hpp>
#include <core/model/Ontology.hpp>

#include <core/model/TestAdvanced.hpp>
#include <core/model/TestOntology.hpp>

namespace ousia {
namespace xml {

TEST(DemoHTMLTransformer, writeHTML)
{
	// Construct Manager
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the ontologies.
	Rooted<Ontology> bookDom = constructBookOntology(mgr, sys, logger);
	Rooted<Ontology> headingDom =
	    constructHeadingOntology(mgr, sys, bookDom, logger);
	Rooted<Ontology> listDom = constructListOntology(mgr, sys, bookDom, logger);
	Rooted<Ontology> emDom = constructEmphasisOntology(mgr, sys, logger);
	// Construct the document.
	Rooted<Document> doc = constructAdvancedDocument(
	    mgr, logger, bookDom, headingDom, listDom, emDom);
	ASSERT_TRUE(doc != nullptr);

	// we can only do a rough check here.
	ResourceManager dummy;
	XmlTransformer transformer;
	std::stringstream out;
	transformer.writeXml(doc, out, logger, dummy, true);
	const std::string res = out.str();
	ASSERT_FALSE(res == "");
	ASSERT_TRUE(res.find("Was ist Aufklärung?") != std::string::npos);
	ASSERT_TRUE(res.find(
	                "Aufklärung ist der Ausgang des Menschen aus seiner "
	                "selbstverschuldeten Unmündigkeit!") != std::string::npos);
	ASSERT_TRUE(res.find("Sapere aude!") != std::string::npos);
}

TEST(DemoHTMLTransformer, AnnotationProcessing)
{
	// Construct Manager
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the ontologies.
	Rooted<Ontology> bookDom = constructBookOntology(mgr, sys, logger);
	Rooted<Ontology> emDom = constructEmphasisOntology(mgr, sys, logger);
	// Construct a document only containing overlapping annotations.
	// it has the form: <em>bla<strong>blub</em>bla</strong>
	Rooted<Document> doc{new Document(mgr, "annotations.oxd")};
	doc->referenceOntologys({bookDom, emDom});
	Rooted<StructuredEntity> book =
	    buildRootStructuredEntity(doc, logger, {"book"});
	ASSERT_TRUE(book != nullptr);
	Rooted<StructuredEntity> p =
	    buildStructuredEntity(doc, logger, book, {"paragraph"});
	ASSERT_TRUE(p != nullptr);
	Rooted<Anchor> em_start{new Anchor(mgr, p)};
	ASSERT_TRUE(addText(logger, doc, p, "bla"));
	Rooted<Anchor> strong_start{new Anchor(mgr, p)};
	ASSERT_TRUE(addText(logger, doc, p, "blub"));
	Rooted<Anchor> em_end{new Anchor(mgr, p)};
	ASSERT_TRUE(addText(logger, doc, p, "bla"));
	Rooted<Anchor> strong_end{new Anchor(mgr, p)};
	buildAnnotationEntity(doc, logger, {"emphasized"}, em_start, em_end);
	buildAnnotationEntity(doc, logger, {"strong"}, strong_start, strong_end);

	// Check serialization.
	ResourceManager dummy;
	XmlTransformer transformer;
	std::stringstream out;
	transformer.writeXml(doc, out, logger, dummy, false);
	const std::string res = out.str();
	// In HTML the overlapping structure must be serialized as follows:
	ASSERT_TRUE(
	    res.find(
	        "<a:start:emphasized/><book:text>bla</book:text><a:start:strong/"
	        "><book:text>blub</book:text><a:end:emphasized/"
	        "><book:text>bla</book:text><a:end:strong/>") != std::string::npos);
}

TEST(DemoHTMLTransformer, PrimitiveSubtreeFields)
{
	// Construct Manager
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Construct a simple ontology.
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "myOntology")};

	Rooted<StructuredClass> A{new StructuredClass(
	    mgr, "A", ontology, Cardinality::any(), nullptr, false, true)};
	Rooted<FieldDescriptor> A_a =
	    A->createPrimitiveFieldDescriptor(sys->getStringType(), logger,
	                                      FieldDescriptor::FieldType::SUBTREE,
	                                      "a").first;
	Rooted<FieldDescriptor> A_b =
	    A->createPrimitiveFieldDescriptor(sys->getStringType(), logger,
	                                      FieldDescriptor::FieldType::SUBTREE,
	                                      "b").first;
	Rooted<FieldDescriptor> A_main =
	    A->createPrimitiveFieldDescriptor(sys->getStringType(), logger).first;
	ASSERT_TRUE(ontology->validate(logger));
	// Construct a document for it.
	Rooted<Document> doc{new Document(mgr, "myDoc")};
	Rooted<StructuredEntity> A_impl = doc->createRootStructuredEntity(A);
	A_impl->createChildDocumentPrimitive("test_a", "a");
	A_impl->createChildDocumentPrimitive("test_b", "b");
	A_impl->createChildDocumentPrimitive("test");
	ASSERT_TRUE(doc->validate(logger));
	// now transform this document.
	ResourceManager dummy;
	XmlTransformer transformer;
	std::stringstream out;
	transformer.writeXml(doc, out, logger, dummy, false);
	const std::string res = out.str();
	ASSERT_TRUE(
	    res.find("<myOntology:A><a>test_a</a><b>test_b</b>test</myOntology:A>") !=
	    std::string::npos);
}
}
}