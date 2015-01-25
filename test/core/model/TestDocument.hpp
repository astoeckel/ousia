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

#ifndef _MODEL_TEST_DOCUMENT_HPP_
#define _MODEL_TEST_DOCUMENT_HPP_

#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Typesystem.hpp>

#include "TestDocumentBuilder.hpp"

namespace ousia {

/**
 * This constructs a fairly simple test document for the "book" domain. The
 * structure of the document can be seen in the Code below.
 */
static Rooted<Document> constructBookDocument(Manager &mgr, Logger &logger,
                                              Rooted<Domain> bookDomain)
{
	// Start with the (empty) document.
	Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
	doc->addDomain(bookDomain);

	// Add the root.
	Rooted<StructuredEntity> root =
	    buildRootStructuredEntity(doc, logger, {"book"});
	if (root.isNull()) {
		return {nullptr};
	}

	// Add a paragraph.
	Rooted<StructuredEntity> foreword =
	    buildStructuredEntity(doc, logger, root, {"paragraph"});
	if (foreword.isNull()) {
		return {nullptr};
	}
	// Add its text.
	Rooted<StructuredEntity> foreword_text =
	    buildStructuredEntity(doc, logger, foreword, {"text"});
	if (foreword_text.isNull()) {
		return {nullptr};
	}
	// And its primitive content
	Variant text{"Some introductory text"};
	Rooted<DocumentPrimitive> foreword_primitive{
	    new DocumentPrimitive(mgr, foreword_text, text, "content")};
	// Add a section.
	Rooted<StructuredEntity> section =
	    buildStructuredEntity(doc, logger, root, {"section"});
	// Add a paragraph for it.
	Rooted<StructuredEntity> main =
	    buildStructuredEntity(doc, logger, section, {"paragraph"});
	if (main.isNull()) {
		return {nullptr};
	}
	// Add its text.
	Rooted<StructuredEntity> main_text =
	    buildStructuredEntity(doc, logger, main, {"text"});
	if (main_text.isNull()) {
		return {nullptr};
	}
	// And its primitive content
	text = Variant{"Some actual text"};
	Rooted<DocumentPrimitive> main_primitive{
	    new DocumentPrimitive(mgr, main_text, text, "content")};
	if (main_primitive.isNull()) {
		return {nullptr};
	}

	return doc;
}
}

#endif /* _TEST_DOCUMENT_HPP_ */

