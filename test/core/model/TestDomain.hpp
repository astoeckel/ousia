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

#include <core/model/Domain.hpp>
#include <core/model/Typesystem.hpp>

namespace ousia {
/**
 * This constructs the "book" domain for test purposes. The structure of the
 * domain is fairly simple and can be seen from the construction itself.
 */
static Rooted<Domain> constructBookDomain(Manager &mgr,
                                          Handle<SystemTypesystem> sys,
                                          Logger &logger)
{
	// Start with the Domain itself.
	Rooted<Domain> domain{new Domain(mgr, sys, "book")};
	// Set up the cardinalities we'll need.
	Cardinality single;
	single.merge({1});
	Cardinality any;
	any.merge(Range<size_t>::typeRangeFrom(0));

	// Set up the "book" node.
	Rooted<StructuredClass> book{new StructuredClass(
	    mgr, "book", domain, single, {nullptr}, {nullptr}, false, true)};

	// The structure field of it.
	Rooted<FieldDescriptor> book_field{new FieldDescriptor(mgr, book)};

	// From there on the "section".
	Rooted<StructuredClass> section{
	    new StructuredClass(mgr, "section", domain, any)};
	book_field->addChild(section);

	// And the field of it.
	Rooted<FieldDescriptor> section_field{new FieldDescriptor(mgr, section)};

	// We also add the "paragraph", which is transparent.
	Rooted<StructuredClass> paragraph{new StructuredClass(
	    mgr, "paragraph", domain, any, {nullptr}, {nullptr}, true)};
	section_field->addChild(paragraph);
	book_field->addChild(paragraph);

	// And the field of it.
	Rooted<FieldDescriptor> paragraph_field{
	    new FieldDescriptor(mgr, paragraph)};

	// We append "subsection" to section.
	Rooted<StructuredClass> subsection{
	    new StructuredClass(mgr, "subsection", domain, any)};
	section_field->addChild(subsection);

	// And the field of it.
	Rooted<FieldDescriptor> subsection_field{
	    new FieldDescriptor(mgr, subsection)};

	// and we add the paragraph to subsections fields
	subsection_field->addChild(paragraph);

	// Finally we add the "text" node, which is transparent as well.
	Rooted<StructuredClass> text{new StructuredClass(
	    mgr, "text", domain, any, {nullptr}, {nullptr}, true)};
	paragraph_field->addChild(text);

	// ... and has a primitive field.
	Rooted<FieldDescriptor> text_field{new FieldDescriptor(
	    mgr, text, domain->getTypesystems()[0]->getTypes()[0], "content",
	    false)};

	return domain;
}
}

#endif /* _TEST_DOMAIN_HPP_ */
