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

#ifndef _MODEL_TEST_DOMAIN_HPP_
#define _MODEL_TEST_DOMAIN_HPP_

#include <core/model/Ontology.hpp>
#include <core/model/Typesystem.hpp>

namespace ousia {
/**
 * This constructs the "book" ontology for test purposes. The structure of the
 * ontology is fairly simple and can be seen from the construction itself.
 */
static Rooted<Ontology> constructBookOntology(Manager &mgr,
                                          Handle<SystemTypesystem> sys,
                                          Logger &logger)
{
	// Start with the Ontology itself.
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "book")};
	// Set up the cardinalities we'll need.
	Cardinality single;
	single.merge({1});

	// Set up the "book" node.
	Rooted<StructuredClass> book{new StructuredClass(
	    mgr, "book", ontology, single, {nullptr}, false, true)};

	// The structure field of it.
	Rooted<FieldDescriptor> book_field =
	    book->createFieldDescriptor(logger).first;

	// From there on the "section".
	Rooted<StructuredClass> section{
	    new StructuredClass(mgr, "section", ontology, Cardinality::any())};
	book_field->addChild(section);

	// And the field of it.
	Rooted<FieldDescriptor> section_field =
	    section->createFieldDescriptor(logger).first;

	// We also add the "paragraph", which is transparent.
	Rooted<StructuredClass> paragraph{new StructuredClass(
	    mgr, "paragraph", ontology, Cardinality::any(), {nullptr}, true)};
	section_field->addChild(paragraph);
	book_field->addChild(paragraph);

	// And the field of it.
	Rooted<FieldDescriptor> paragraph_field =
	    paragraph->createFieldDescriptor(logger).first;

	// We append "subsection" to section.
	Rooted<StructuredClass> subsection{
	    new StructuredClass(mgr, "subsection", ontology, Cardinality::any())};
	section_field->addChild(subsection);

	// And the field of it.
	Rooted<FieldDescriptor> subsection_field =
	    subsection->createFieldDescriptor(logger).first;

	// and we add the paragraph to subsections fields
	subsection_field->addChild(paragraph);

	// Finally we add the "text" node, which is transparent as well.
	Rooted<StructuredClass> text{new StructuredClass(
	    mgr, "text", ontology, Cardinality::any(), {nullptr}, true)};
	paragraph_field->addChild(text);

	// ... and has a primitive field.
	Rooted<FieldDescriptor> text_field =
	    text->createPrimitiveFieldDescriptor(sys->getStringType(), logger)
	        .first;

	return ontology;
}
}

#endif /* _TEST_DOMAIN_HPP_ */