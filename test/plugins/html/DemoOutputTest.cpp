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

#include <plugins/html/DemoOutput.hpp>

#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>

#include <core/model/TestAdvanced.hpp>
#include <core/model/TestDomain.hpp>

namespace ousia {
namespace html {

TEST(DemoHTMLTransformer, writeHTML)
{
	// Construct Manager
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<model::SystemTypesystem> sys{new model::SystemTypesystem(mgr)};
	// Get the domains.
	Rooted<model::Domain> bookDom =
	    model::constructBookDomain(mgr, sys, logger);
	Rooted<model::Domain> headingDom =
	    model::constructHeadingDomain(mgr, sys, bookDom, logger);
	Rooted<model::Domain> listDom =
	    model::constructListDomain(mgr, sys, bookDom, logger);
	Rooted<model::Domain> emDom =
	    model::constructEmphasisDomain(mgr, sys, logger);
	// Construct the document.
	Rooted<model::Document> doc = model::constructAdvancedDocument(
	    mgr, logger, bookDom, headingDom, listDom, emDom);
	ASSERT_TRUE(doc != nullptr);

#ifdef MANAGER_GRAPHVIZ_EXPORT
	// dump the manager state
	mgr.exportGraphviz("bookDocument.dot");
#endif

	// TODO: change this. Don't use printouts
	DemoHTMLTransformer transformer;
	transformer.writeHTML(doc, std::cout);
}

TEST(DemoHTMLTransformer, AnnotationProcessing)
{
	// Construct Manager
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<model::SystemTypesystem> sys{new model::SystemTypesystem(mgr)};
	// Get the domains.
	Rooted<model::Domain> bookDom =
	    model::constructBookDomain(mgr, sys, logger);
	Rooted<model::Domain> emDom =
	    model::constructEmphasisDomain(mgr, sys, logger);
	// Construct a document only containing overlapping annotations.
	// it has the form: <em>bla<strong>blub</em>bla</strong>
	Rooted<model::Document> doc{new model::Document(mgr, "annotations.oxd")};
	doc->addDomains({bookDom, emDom});
	Rooted<model::StructuredEntity> book =
	    buildRootStructuredEntity(doc, logger, {"book"});
	ASSERT_TRUE(book != nullptr);
	Rooted<model::StructuredEntity> p =
	    buildStructuredEntity(doc, logger, book, {"paragraph"});
	ASSERT_TRUE(p != nullptr);
	Rooted<model::AnnotationEntity::Anchor> em_start =
	    buildAnchor(logger, p, "1");
	ASSERT_TRUE(em_start != nullptr);
	ASSERT_TRUE(addText(logger, doc, p, "bla"));
	Rooted<model::AnnotationEntity::Anchor> strong_start =
	    buildAnchor(logger, p, "2");
	ASSERT_TRUE(strong_start != nullptr);
	ASSERT_TRUE(addText(logger, doc, p, "blub"));
	Rooted<model::AnnotationEntity::Anchor> em_end =
	    buildAnchor(logger, p, "3");
	ASSERT_TRUE(em_end != nullptr);
	ASSERT_TRUE(addText(logger, doc, p, "bla"));
	Rooted<model::AnnotationEntity::Anchor> strong_end =
	    buildAnchor(logger, p, "4");
	ASSERT_TRUE(strong_end != nullptr);
	buildAnnotationEntity(doc, logger, {"emphasized"}, em_start, em_end);
	buildAnnotationEntity(doc, logger, {"strong"}, strong_start, strong_end);

	// TODO: change this. Don't use printouts
	DemoHTMLTransformer transformer;
	transformer.writeHTML(doc, std::cout);
}
}
}
