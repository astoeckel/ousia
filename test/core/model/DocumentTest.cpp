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

#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>

#include "TestDocument.hpp"
#include "TestDomain.hpp"

namespace ousia {
namespace model {

TEST(Document, testDocumentConstruction)
{
	// Construct Manager
	Logger logger;
	Manager mgr{1};
	// Get the domain.
	Rooted<Domain> domain = constructBookDomain(mgr, logger);
	// Construct the document.
	Rooted<Document> doc = constructBookDocument(mgr, domain);

	// If that works we are happy already.
	ASSERT_FALSE(doc.isNull());
}
}
}
