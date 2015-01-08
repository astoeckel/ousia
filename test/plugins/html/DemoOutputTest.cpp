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
#include <core/model/TestDocument.hpp>
#include <core/model/TestDomain.hpp>

namespace ousia {
namespace html {

TEST(DemoHTMLTransformer, writeHTML)
{
	// Construct Manager
	Logger logger;
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
	Rooted<model::Document> doc =
	    model::constructAdvancedDocument(mgr, bookDom, headingDom, listDom);

#ifdef MANAGER_GRAPHVIZ_EXPORT
	// dump the manager state
	mgr.exportGraphviz("bookDocument.dot");
#endif

	// TODO: change this. Don't use printouts
	DemoHTMLTransformer transformer;
	transformer.writeHTML(doc, std::cout);
}
}
}
