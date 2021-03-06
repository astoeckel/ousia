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
#include <core/model/Ontology.hpp>
#include <core/model/Typesystem.hpp>

#include "TestDocumentBuilder.hpp"

namespace ousia {

static Rooted<StructuredClass> resolveDescriptor(Handle<Ontology> ontology,
                                                 const std::string &className)
{
	// use the actual resolve method.
	std::vector<ResolutionResult> resolved =
	    ontology->resolve(&RttiTypes::StructuredClass, className);
	// take the first valid result.
	for (auto &r : resolved) {
		return r.node.cast<StructuredClass>();
	}
	// if no valid result exists, return nullptr.
	return {nullptr};
}

/**
 * This constructs the "heading" ontology given the book ontology.
 */
static Rooted<Ontology> constructHeadingOntology(Manager &mgr,
                                                 Handle<SystemTypesystem> sys,
                                                 Handle<Ontology> bookOntology,
                                                 Logger &logger)
{
	// set up ontology node.
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "headings")};
	// set up cardinality (every section may have at most one heading).
	Cardinality card;
	card.merge({0, 1});
	// set up heading StructuredClass.
	Rooted<StructuredClass> heading{
	    new StructuredClass(mgr, "heading", ontology, card, {nullptr}, true)};
	// as field want to reference the field of paragraph.
	Rooted<StructuredClass> p = resolveDescriptor(bookOntology, "paragraph");
	heading->addFieldDescriptor(p->getFieldDescriptor(), logger);
	// create a new field for headings in each section type.
	std::vector<std::string> secclasses{"book", "section", "subsection",
	                                    "paragraph"};
	for (auto &s : secclasses) {
		Rooted<StructuredClass> desc = resolveDescriptor(bookOntology, s);
		Rooted<FieldDescriptor> heading_field =
		    desc->createFieldDescriptor(logger,
		                                FieldDescriptor::FieldType::SUBTREE,
		                                "heading", true).first;
		heading_field->addChild(heading);
	}
	return ontology;
}

/**
 * This constructs the "list" ontology given the book ontology.
 */
static Rooted<Ontology> constructListOntology(Manager &mgr,
                                              Handle<SystemTypesystem> sys,
                                              Handle<Ontology> bookOntology,
                                              Logger &logger)
{
	// set up ontology node.
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "list")};
	// get book.paragraph
	Rooted<StructuredClass> p = resolveDescriptor(bookOntology, "paragraph");
	// set up item StructuredClass;
	Rooted<StructuredClass> item{new StructuredClass(
	    mgr, "item", ontology, Cardinality::any(), {nullptr}, false)};

	// as field we want to reference the field of paragraph.
	item->addFieldDescriptor(p->getFieldDescriptor(), logger);
	// set up list StructuredClasses.
	std::vector<std::string> listTypes{"ol", "ul"};
	for (auto &listType : listTypes) {
		Rooted<StructuredClass> list{new StructuredClass(
		    mgr, listType, ontology, Cardinality::any(), p, false)};
		Rooted<FieldDescriptor> list_field =
		    list->createFieldDescriptor(logger).first;
		list_field->addChild(item);
	}
	return ontology;
}

/**
 * This constructs the "emphasis" ontology.
 */
static Rooted<Ontology> constructEmphasisOntology(Manager &mgr,
                                                  Handle<SystemTypesystem> sys,
                                                  Logger &logger)
{
	// set up ontology node.
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "emphasis")};
	// create AnnotationClasses
	Rooted<AnnotationClass> em{
	    new AnnotationClass(mgr, "emph", ontology)};

	Rooted<AnnotationClass> strong{
	    new AnnotationClass(mgr, "strong", ontology)};

	return ontology;
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
	    parent->getManager(), text, content_var, DEFAULT_FIELD_NAME)};
	return true;
}

static bool addHeading(Logger &logger, Handle<Document> doc,
                       Handle<StructuredEntity> parent, const std::string &text)
{
	// Add the heading.
	Rooted<StructuredEntity> heading = buildStructuredEntity(
	    doc, logger, parent, {"heading"}, "heading", Variant::mapType{}, "");
	if (heading.isNull()) {
		return false;
	}
	// Add its text.
	if (!addText(logger, doc, heading, text)) {
		return false;
	}
	return true;
}

// Only works for non-overlapping annotations!
static bool addAnnotation(Logger &logger, Handle<Document> doc,
                          Handle<StructuredEntity> parent,
                          const std::string &text, const std::string &annoClass)
{
	Manager &mgr = parent->getManager();
	Rooted<Anchor> start{new Anchor(mgr, parent)};
	if (!addText(logger, doc, parent, text)) {
		return false;
	}
	Rooted<Anchor> end{new Anchor(mgr, parent)};
	Rooted<AnnotationEntity> anno =
	    buildAnnotationEntity(doc, logger, {annoClass}, start, end);
	if (anno.isNull()) {
		return false;
	}
	return true;
}

/**
 * This constructs a more advanced book document using not only the book
 * ontology but also headings, emphasis and lists.
 */
static Rooted<Document> constructAdvancedDocument(Manager &mgr, Logger &logger,
                                                  Handle<Ontology> bookDom,
                                                  Handle<Ontology> headingDom,
                                                  Handle<Ontology> listDom,
                                                  Handle<Ontology> emphasisDom)
{
	// Start with the (empty) document.
	Rooted<Document> doc{new Document(mgr, "kant_was_ist_aufklaerung.oxd")};
	doc->referenceOntologys({bookDom, headingDom, listDom, emphasisDom});

	// Add the root.
	Rooted<StructuredEntity> book =
	    buildRootStructuredEntity(doc, logger, {"book"});
	if (book.isNull()) {
		return {nullptr};
	}

	// Add the heading.
	{
		Rooted<StructuredEntity> heading = buildStructuredEntity(
		    doc, logger, book, {"heading"}, "heading", Variant::mapType{}, "");
		if (heading.isNull()) {
			return {nullptr};
		}
		if (!addText(logger, doc, heading, "Beantwortung der Frage: ")) {
			return {nullptr};
		}
		if (!addAnnotation(logger, doc, heading, "Was ist Aufklärung?",
		                   "emph")) {
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
			if (!addAnnotation(logger, doc, p, "Unmündigkeit", "emph")) {
				return {nullptr};
			}
			if (!addText(logger, doc, p,
			             "ist das Unvermögen, sich seines Verstandes ohne "
			             "Leitung eines anderen zu bedienen. ")) {
				return {nullptr};
			}
			if (!addAnnotation(logger, doc, p, "Selbstverschuldet",
			                   "emph")) {
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
			                   "emph")) {
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
