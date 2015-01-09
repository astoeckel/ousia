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

namespace ousia {
namespace model {

static Rooted<StructuredClass> resolveDescriptor(Handle<Domain> domain,
                                                 const std::string &className)
{
	// use the actual resolve method.
	std::vector<ResolutionResult> resolved = domain->resolve(className, typeOf<StructuredClass>());
	// take the first valid result.
	for (auto &r : resolved) {
		return r.node.cast<StructuredClass>();
	}
	// if no valid result exists, return nullptr.
	return {nullptr};
}

/**
 * This constructs the "heading" domain given the book domain.
 */
static Rooted<Domain> constructHeadingDomain(Manager &mgr,
                                             Handle<SystemTypesystem> sys,
                                             Handle<Domain> bookDomain,
                                             Logger &logger)
{
	// set up domain node.
	Rooted<Domain> domain{new Domain(mgr, sys, "headings")};
	// set up cardinality (every section may have at most one heading).
	Cardinality card;
	card.merge({0, 1});
	// set up heading StructuredClass.
	Rooted<StructuredClass> heading{new StructuredClass(
	    mgr, "heading", domain, card, {nullptr}, {nullptr}, true)};
	// as field we actually want to refer to the field of paragraph.
	Rooted<StructuredClass> p = resolveDescriptor(bookDomain, "paragraph");
	heading->getFieldDescriptors().push_back(p->getFieldDescriptors()[0]);
	// add the class to the domain.
	domain->getStructureClasses().push_back(heading);
	// create a new field for headings in each section type.
	std::vector<std::string> secclasses{"book", "section", "subsection",
	                                    "paragraph"};
	for (auto &s : secclasses) {
		Rooted<StructuredClass> desc = resolveDescriptor(bookDomain, s);
		Rooted<FieldDescriptor> heading_field{new FieldDescriptor(
		    mgr, desc, FieldDescriptor::FieldType::SUBTREE, "heading")};
		heading_field->getChildren().push_back(heading);
		desc->getFieldDescriptors().push_back(heading_field);
	}
	return domain;
}

/**
 * This constructs the "list" domain given the book domain.
 */
static Rooted<Domain> constructListDomain(Manager &mgr,
                                          Handle<SystemTypesystem> sys,
                                          Handle<Domain> bookDomain,
                                          Logger &logger)
{
	// set up domain node.
	Rooted<Domain> domain{new Domain(mgr, sys, "list")};
	// set up cardinality
	Cardinality any;
	any.merge(Range<size_t>::typeRangeFrom(0));
	// get book.paragraph
	Rooted<StructuredClass> p = resolveDescriptor(bookDomain, "paragraph");
	// set up item StructuredClass;
	Rooted<StructuredClass> item{new StructuredClass(
	    mgr, "item", domain, any, {nullptr}, {nullptr}, false)};
	domain->getStructureClasses().push_back(item);
	// as field we actually want to refer to the field of paragraph.
	item->getFieldDescriptors().push_back(p->getFieldDescriptors()[0]);
	// set up list StructuredClasses.
	std::vector<std::string> listTypes{"ol", "ul"};
	for (auto &listType : listTypes) {
		Rooted<StructuredClass> list{new StructuredClass(
		    mgr, listType, domain, any, {nullptr}, p, false)};
		Rooted<FieldDescriptor> list_field{new FieldDescriptor(mgr, list)};
		list_field->getChildren().push_back(item);
		list->getFieldDescriptors().push_back(list_field);
		domain->getStructureClasses().push_back(list);
	}
	return domain;
}

/**
 * This constructs the "emphasis" domain.
 */
static Rooted<Domain> constructEmphasisDomain(Manager &mgr,
                                              Handle<SystemTypesystem> sys,
                                              Logger &logger)
{
	// set up domain node.
	Rooted<Domain> domain{new Domain(mgr, sys, "emphasis")};
	// create AnnotationClasses
	Rooted<AnnotationClass> em{
	    new AnnotationClass(mgr, "emphasized", domain, {nullptr})};
	domain->getAnnotationClasses().push_back(em);
	Rooted<AnnotationClass> strong{
	    new AnnotationClass(mgr, "strong", domain, {nullptr})};
	domain->getAnnotationClasses().push_back(strong);
	return domain;
}

static bool addText(Handle<StructuredEntity> parent,
                    std::vector<Handle<Domain>> &doms,
                    const std::string &content)
{
	// Add its text.
	Rooted<StructuredEntity> text =
	    StructuredEntity::buildEntity(parent, doms, "text");
	if (text.isNull()) {
		return false;
	}
	// And its primitive content
	Variant content_var{content.c_str()};
	Rooted<DocumentPrimitive> primitive =
	    DocumentPrimitive::buildEntity(text, content_var, "content");
	if (primitive.isNull()) {
		return false;
	}

	return true;
}

static bool addHeading(Handle<StructuredEntity> parent,
                       std::vector<Handle<Domain>> &doms,
                       const std::string &text)
{
	// Add the heading.
	Rooted<StructuredEntity> heading = StructuredEntity::buildEntity(
	    parent, doms, "heading", "heading", {}, "");
	if (heading.isNull()) {
		return false;
	}
	// Add its text.
	if (!addText(heading, doms, text)) {
		return false;
	}
	return true;
}

static int annoIdx = 1;

// Only works for non-overlapping annotations!
static bool addAnnotation(Handle<Document> doc, Handle<StructuredEntity> parent,
                          std::vector<Handle<Domain>> &doms,
                          const std::string &text, const std::string &annoClass)
{
	Rooted<AnnotationEntity::Anchor> start =
	    AnnotationEntity::buildAnchor(parent, std::to_string(annoIdx++));
	if (start.isNull()) {
		return false;
	}
	if (!addText(parent, doms, text)) {
		return false;
	}
	Rooted<AnnotationEntity::Anchor> end =
	    AnnotationEntity::buildAnchor(parent, std::to_string(annoIdx++));
	if (end.isNull()) {
		return false;
	}
	Rooted<AnnotationEntity> anno =
	    AnnotationEntity::buildEntity(doc, doms, annoClass, start, end);
	if (anno.isNull()) {
		return false;
	}
	return true;
}

/**
 * This constructs a more advanced book document using not only the book
 * domain but also headings, emphasis and lists.
 * TODO: insert emphasis and lists.
 */
static Rooted<Document> constructAdvancedDocument(Manager &mgr,
                                                  Handle<Domain> bookDom,
                                                  Handle<Domain> headingDom,
                                                  Handle<Domain> listDom)
{
	std::vector<Handle<Domain>> doms{bookDom, headingDom, listDom};

	// Start with the (empty) document.
	Rooted<Document> doc{new Document(mgr, "kant_was_ist_aufklaerung.oxd")};

	// Add the root.
	Rooted<StructuredEntity> book =
	    StructuredEntity::buildRootEntity(doc, doms, "book");
	if (book.isNull()) {
		return {nullptr};
	}

	// Add the heading.
	{
		Rooted<StructuredEntity> heading = StructuredEntity::buildEntity(
		    book, doms, "heading", "heading", {}, "");
		if (heading.isNull()) {
			return {nullptr};
		}
		if (!addText(heading, doms, "Beantwortung der Frage: ")) {
			return {nullptr};
		}
		if (!addAnnotation(doc, heading, doms, "Was ist Aufklärung?",
		                   "emphasized")) {
			return {nullptr};
		}
	}

	// Add the main section.
	Rooted<StructuredEntity> sec =
	    StructuredEntity::buildEntity(book, doms, "section");
	if (sec.isNull()) {
		return {nullptr};
	}

	// Add the heading.
	if (!addHeading(sec, doms, "Was ist Aufklärung?")) {
		return {nullptr};
	}

	// Add paragraph with main text.
	{
		Rooted<StructuredEntity> p =
		    StructuredEntity::buildEntity(sec, doms, "paragraph");
		if (p.isNull()) {
			return {nullptr};
		}
		// Add its text.
		{
			if (!addAnnotation(doc, p, doms,
			                   "Aufklärung ist der Ausgang des Menschen aus "
			                   "seiner selbstverschuldeten Unmündigkeit",
			                   "strong")) {
				return {nullptr};
			}
			if (!addAnnotation(doc, p, doms, "Unmündigkeit",
			                   "emphasized")) {
				return {nullptr};
			}
			if (!addText(p, doms,
			             "ist das Unvermögen, sich seines Verstandes ohne "
			             "Leitung eines anderen zu bedienen. ")) {
				return {nullptr};
			}
			if (!addAnnotation(doc, p, doms, "Selbstverschuldet",
			                   "emphasized")) {
				return {nullptr};
			}
			if (!addText(p, doms,
			             " ist diese Unmündigkeit, wenn die Ursache derselben "
			             "nicht am Mangel des Verstandes, sondern der "
			             "Entschließung und des Mutes liegt, sich seiner ohne "
			             "Leitung eines andern zu bedienen.")) {
				return {nullptr};
			}
			if (!addAnnotation(doc, p, doms,
			                   "Sapere aude! Habe Mut, dich deines eigenen "
			                   "Verstandes zu bedienen!",
			                   "emphasized")) {
				return {nullptr};
			}
			if (!addText(p, doms,
			             " ist also der Wahlspruch der Aufklärung.")) {
				return {nullptr};
			}
		}
	}

	// Add the "Lesarten" section
	Rooted<StructuredEntity> lesarten =
	    StructuredEntity::buildEntity(book, doms, "section");
	if (lesarten.isNull()) {
		return {nullptr};
	}
	// Add the heading.
	if (!addHeading(lesarten, doms, "Lesarten")) {
		return {nullptr};
	}
	// Add list with citations
	{
		// TODO: We need to restrict this to the list domain. Otherwise
		// this leads to resolve errors for some reason.
		Rooted<StructuredEntity> ul =
		    StructuredEntity::buildEntity(lesarten, {listDom}, "ul");
		if (ul.isNull()) {
			return {nullptr};
		}
		std::vector<std::string> citations{
		    "Berlinische Monatsschrift. Dezember-Heft 1784. S. 481–494.",
		    "Kant. Kleine Schriften. Neuwied 1793. Haupt. 8o. S. 34–50.",
		    "I. Kant. Zerstreute Aufsätze. Frankfurt und Leipzig 1793. 8o. S. "
		    "25–37.",
		    "I. Kant. Sämmtliche kleine Schriften. 4 Bände. 1797–98. 8o.  "
		    "Königsberg u. Leipzig (Voigt, Jena). Nachdruck. Bd. III, S. "
		    "159–172.",
		    "  I. Kant's vermischte Schriften. 3 Bände. Halle 1799. "
		    "(Tieftrunk). Bd. II. S. 687–700.",
		    "Kant. Vorzügliche kleine Schriften und Aufsätze, hrsg. mit Noten "
		    "von F. Ch. Starke. 2 Bände. Leipzig 1833 und Quedlinburg 1838. "
		    "Bd. I, S. 75–84."};
		for (auto &cit : citations) {
			// TODO: This needs to be restricted as well.
			Rooted<StructuredEntity> item =
			    StructuredEntity::buildEntity(ul, {listDom}, "item");
			if (item.isNull()) {
				return {nullptr};
			}
			if (!addText(item, doms, cit)) {
				return {nullptr};
			}
		}
	}

	return doc;
}
}
}

#endif /* _TEST_DOCUMENT_HPP_ */

