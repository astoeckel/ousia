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
namespace model {

/**
 * This constructs a somewhat trivial system of standard types.
 *
 * Currently contained: string, text (struct wrapper for string)
 */
static Rooted<Typesystem> constructTypeSystem(Manager &mgr)
{
	Rooted<Typesystem> sys{new Typesystem(mgr, "std")};
	Rooted<StringType> string{new StringType(mgr, sys)};
	sys->addType(string);
	Rooted<StructType> string_struct{new StructType(
	    mgr, "text", sys, {new Attribute(mgr, "content", string, "", false)})};
	sys->addType(string_struct);

	return sys;
}

/**
 * This constructs the "book" domain for test purposes. The structure of the
 * domain is fairly simple and can be seen from the construction itself.
 */
static Rooted<Domain> constructBookDomain(Manager &mgr)
{
	// Start with the Domain itself.
	Rooted<Domain> domain{new Domain(mgr, "book")};
	// The standard type system.
	domain->getTypesystems().push_back(constructTypeSystem(mgr));
	// Set up the cardinalities we'll need.
	Cardinality single;
	single.merge({1});
	Cardinality any;
	any.merge(Range<size_t>::typeRangeFrom(0));

	// Set up the "book" node.
	Rooted<StructuredClass> book{new StructuredClass(
	    mgr, "book", domain, single, {nullptr}, {nullptr}, false, true)};
	domain->getStructureClasses().push_back(book);
	// The structure field of it.
	Rooted<FieldDescriptor> book_field{new FieldDescriptor(mgr, book)};
	book->getFieldDescriptors().push_back(book_field);

	// From there on the "section".
	Rooted<StructuredClass> section{
	    new StructuredClass(mgr, "section", domain, any)};
	book_field->getChildren().push_back(section);
	domain->getStructureClasses().push_back(section);
	// And the field of it.
	Rooted<FieldDescriptor> section_field{new FieldDescriptor(mgr, section)};
	section->getFieldDescriptors().push_back(section_field);

	// We also add the "paragraph", which is transparent.
	Rooted<StructuredClass> paragraph{new StructuredClass(
	    mgr, "paragraph", domain, any, {nullptr}, {nullptr}, true)};
	section_field->getChildren().push_back(paragraph);
	book_field->getChildren().push_back(paragraph);
	domain->getStructureClasses().push_back(paragraph);
	// ... and has a primitive field.
	Rooted<FieldDescriptor> paragraph_field{new FieldDescriptor(
	    mgr, paragraph, domain->getTypesystems()[0]->getTypes()[1], "text",
	    false)};
	paragraph->getFieldDescriptors().push_back(paragraph_field);

	return domain;
}
}
}

#endif /* _TEST_DOMAIN_HPP_ */

