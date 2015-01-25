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

#ifndef _MODEL_TEST_ADVANCED_HPP_
#define _MODEL_TEST_ADVANCED_HPP_

#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Typesystem.hpp>

#include "TestDocumentBuilder.hpp"

namespace ousia {

static Rooted<StructuredClass> resolveDescriptor(Handle<Domain> domain,
                                                 const std::string &className)
{
	// use the actual resolve method.
	std::vector<ResolutionResult> resolved =
	    domain->resolve(className, typeOf<StructuredClass>());
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
	Variant::cardinalityType card;
	card.merge({0, 1});
	// set up heading StructuredClass.
	Rooted<StructuredClass> heading{new StructuredClass(
	    mgr, "heading", domain, card, {nullptr}, {nullptr}, true)};
	// as field want to copy the field of paragraph.
	Rooted<StructuredClass> p = resolveDescriptor(bookDomain, "paragraph");
	heading->copyFieldDescriptor(p->getFieldDescriptors()[0]);
	// create a new field for headings in each section type.
	std::vector<std::string> secclasses{"book", "section", "subsection",
	                                    "paragraph"};
	for (auto &s : secclasses) {
		Rooted<StructuredClass> desc = resolveDescriptor(bookDomain, s);
		Rooted<FieldDescriptor> heading_field{new FieldDescriptor(
		    mgr, desc, FieldDescriptor::FieldType::SUBTREE, "heading")};
		heading_field->addChild(heading);
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
	Variant::cardinalityType any;
	any.merge(Range<size_t>::typeRange());
	// get book.paragraph
	Rooted<StructuredClass> p = resolveDescriptor(bookDomain, "paragraph");
	// set up item StructuredClass;
	Rooted<StructuredClass> item{new StructuredClass(
	    mgr, "item", domain, any, {nullptr}, {nullptr}, false)};

	// as field we want to copy the field of paragraph.
	item->copyFieldDescriptor(p->getFieldDescriptors()[0]);
	// set up list StructuredClasses.
	std::vector<std::string> listTypes{"ol", "ul"};
	for (auto &listType : listTypes) {
		Rooted<StructuredClass> list{new StructuredClass(
		    mgr, listType, domain, any, {nullptr}, p, false)};
		Rooted<FieldDescriptor> list_field{new FieldDescriptor(mgr, list)};
		list_field->addChild(item);
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
	Rooted<AnnotationClass> em{new AnnotationClass(mgr, "emphasized", domain)};

	Rooted<AnnotationClass> strong{new AnnotationClass(mgr, "strong", domain)};

	return domain;
}

static bool addText(Logger &logger, Handle<Document> doc,
                    Handle<StructuredEntity> parent, const std::string &content)
{
	// Add the text.
	Rooted<StructuredEntity> text =
	    buildStructuredEntity(doc, logger, parent, {"text"});
	if (text.isNull()) {
		return false;
	}
	// And the primitive content
	Variant content_var{content.c_str()};
	Rooted<DocumentPrimitive> primitive{new DocumentPrimitive(
	    parent->getManager(), text, content_var, "content")};
	return true;
}

static bool addHeading(Logger &logger, Handle<Document> doc,
                       Handle<StructuredEntity> parent, const std::string &text)
{
	// Add the heading.
	Rooted<StructuredEntity> heading = buildStructuredEntity(
	    doc, logger, parent, {"heading"}, "heading", {}, "");
	if (heading.isNull()) {
		return false;
	}
	// Add its text.
	if (!addText(logger, doc, heading, text)) {
		return false;
	}
	return true;
}

static int annoIdx = 1;

// Only works for non-overlapping annotations!
static bool addAnnotation(Logger &logger, Handle<Document> doc,
                          Handle<StructuredEntity> parent,
                          const std::string &text, const std::string &annoClass)
{
	Manager& mgr = parent->getManager();
	Rooted<Anchor> start{new Anchor(mgr, std::to_string(annoIdx++), parent)};
	if (!addText(logger, doc, parent, text)) {
		return false;
	}
	Rooted<Anchor> end{new Anchor(mgr, std::to_string(annoIdx++), parent)};
	Rooted<AnnotationEntity> anno =
	    buildAnnotationEntity(doc, logger, {annoClass}, start, end);
	if (anno.isNull()) {
		return false;
	}
	return true;
}

/**
 * This constructs a more advanced book document using not only the book
 * domain but also headings, emphasis and lists.
 */
static Rooted<Document> constructAdvancedDocument(Manager &mgr, Logger &logger,
                                                  Handle<Domain> bookDom,
                                                  Handle<Domain> headingDom,
                                                  Handle<Domain> listDom,
                                                  Handle<Domain> emphasisDom)
{
	// Start with the (empty) document.
	Rooted<Document> doc{new Document(mgr, "kant_was_ist_aufklaerung.oxd")};
	doc->addDomains({bookDom, headingDom, listDom, emphasisDom});

	// Add the root.
	Rooted<StructuredEntity> book =
	    buildRootStructuredEntity(doc, logger, {"book"});
	if (book.isNull()) {
		return {nullptr};
	}

	// Add the heading.
	{
		Rooted<StructuredEntity> heading = buildStructuredEntity(
		    doc, logger, book, {"heading"}, "heading", {}, "");
		if (heading.isNull()) {
			return {nullptr};
		}
		if (!addText(logger, doc, heading, "Beantwortung der Frage: ")) {
			return {nullptr};
		}
		if (!addAnnotation(logger, doc, heading, "Was ist Aufklärung?",
		                   "emphasized")) {
			return {nullptr};
		}
	}

	// Add the main section.
	Rooted<StructuredEntity> sec =
	    buildStructuredEntity(doc, logger, book, {"section"});
	if (sec.isNull()) {
		return {nullptr};
	}

	// Add the heading.
	if (!addHeading(logger, doc, sec, "Was ist Aufklärung?")) {
		return {nullptr};
	}

	// Add paragraph with main text.
	{
		Rooted<StructuredEntity> p =
		    buildStructuredEntity(doc, logger, sec, {"paragraph"});
		if (p.isNull()) {
			return {nullptr};
		}
		// Add its text.
		{
			if (!addAnnotation(logger, doc, p,
			                   "Aufklärung ist der Ausgang des Menschen aus "
			                   "seiner selbstverschuldeten Unmündigkeit!",
			                   "strong")) {
				return {nullptr};
			}
			if (!addAnnotation(logger, doc, p, "Unmündigkeit", "emphasized")) {
				return {nullptr};
			}
			if (!addText(logger, doc, p,
			             "ist das Unvermögen, sich seines Verstandes ohne "
			             "Leitung eines anderen zu bedienen. ")) {
				return {nullptr};
			}
			if (!addAnnotation(logger, doc, p, "Selbstverschuldet",
			                   "emphasized")) {
				return {nullptr};
			}
			if (!addText(logger, doc, p,
			             " ist diese Unmündigkeit, wenn die Ursache derselben "
			             "nicht am Mangel des Verstandes, sondern der "
			             "Entschließung und des Mutes liegt, sich seiner ohne "
			             "Leitung eines andern zu bedienen.")) {
				return {nullptr};
			}
			if (!addAnnotation(logger, doc, p,
			                   "Sapere aude! Habe Mut, dich deines eigenen "
			                   "Verstandes zu bedienen!",
			                   "emphasized")) {
				return {nullptr};
			}
			if (!addText(logger, doc, p,
			             " ist also der Wahlspruch der Aufklärung.")) {
				return {nullptr};
			}
		}
	}

	// Add the "Lesarten" section
	Rooted<StructuredEntity> lesarten =
	    buildStructuredEntity(doc, logger, book, {"section"});
	if (lesarten.isNull()) {
		return {nullptr};
	}
	// Add the heading.
	if (!addHeading(logger, doc, lesarten, "Lesarten")) {
		return {nullptr};
	}
	// Add list with citations
	{
		Rooted<StructuredEntity> ul =
		    buildStructuredEntity(doc, logger, lesarten, {"ul"});
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
			Rooted<StructuredEntity> item =
			    buildStructuredEntity(doc, logger, ul, {"item"});
			if (item.isNull()) {
				return {nullptr};
			}
			if (!addText(logger, doc, item, cit)) {
				return {nullptr};
			}
		}
	}

	return doc;
}
}

#endif /* _TEST_DOCUMENT_HPP_ */
