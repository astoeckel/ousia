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

#include <core/common/Rtti.hpp>

#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>

#include "TestDocument.hpp"
#include "TestDomain.hpp"

namespace ousia {
namespace model {

TEST(Document, testDocumentConstruction)
{
	// Construct Manager
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the domain.
	Rooted<Domain> domain = constructBookDomain(mgr, sys, logger);
	// Construct the document.
	Rooted<Document> doc = constructBookDocument(mgr, logger, domain);

	// Check the document content.
	ASSERT_FALSE(doc.isNull());
	// get root node.
	Rooted<StructuredEntity> root = doc->getRoot();
	ASSERT_FALSE(root.isNull());
	ASSERT_EQ("book", root->getDescriptor()->getName());
	ASSERT_TRUE(root->hasField());
	ASSERT_EQ(2U, root->getField().size());
	// get foreword (paragraph)
	{
		Rooted<StructuredEntity> foreword =
		    root->getField()[0].cast<StructuredEntity>();
		ASSERT_FALSE(foreword.isNull());
		ASSERT_EQ("paragraph", foreword->getDescriptor()->getName());
		// it should contain one text node
		ASSERT_TRUE(foreword->hasField());
		ASSERT_EQ(1U, foreword->getField().size());
		// which in turn should have a primitive content field containing the
		// right text.
		{
			Rooted<StructuredEntity> text =
			    foreword->getField()[0].cast<StructuredEntity>();
			ASSERT_FALSE(text.isNull());
			ASSERT_EQ("text", text->getDescriptor()->getName());
			ASSERT_TRUE(text->hasField());
			ASSERT_EQ(1U, text->getField().size());
			ASSERT_TRUE(text->getField()[0]->isa(typeOf<DocumentPrimitive>()));
			Variant content =
			    text->getField()[0].cast<DocumentPrimitive>()->getContent();
			ASSERT_EQ("Some introductory text", content.asString());
		}
	}
	// get section
	{
		Rooted<StructuredEntity> section =
		    root->getField()[1].cast<StructuredEntity>();
		ASSERT_FALSE(section.isNull());
		ASSERT_EQ("section", section->getDescriptor()->getName());
		// it should contain one paragraph
		ASSERT_TRUE(section->hasField());
		ASSERT_EQ(1U, section->getField().size());
		{
			Rooted<StructuredEntity> par =
			    section->getField()[0].cast<StructuredEntity>();
			ASSERT_FALSE(par.isNull());
			ASSERT_EQ("paragraph", par->getDescriptor()->getName());
			// it should contain one text node
			ASSERT_TRUE(par->hasField());
			ASSERT_EQ(1U, par->getField().size());
			// which in turn should have a primitive content field containing
			// the right text.
			{
				Rooted<StructuredEntity> text =
				    par->getField()[0].cast<StructuredEntity>();
				ASSERT_FALSE(text.isNull());
				ASSERT_EQ("text", text->getDescriptor()->getName());
				ASSERT_TRUE(text->hasField());
				ASSERT_EQ(1U, text->getField().size());
				ASSERT_TRUE(
				    text->getField()[0]->isa(typeOf<DocumentPrimitive>()));
				Variant content =
				    text->getField()[0].cast<DocumentPrimitive>()->getContent();
				ASSERT_EQ("Some actual text", content.asString());
			}
		}
	}
}
}
}
