/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#include <sstream>
#include <stack>

#include <core/common/Exceptions.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Variant.hpp>

#include "DemoOutput.hpp"

namespace ousia {
namespace html {

void DemoHTMLTransformer::writeHTML(Handle<model::Document> doc,
                                    std::ostream &out)
{
	Manager &mgr = doc->getManager();
	// Create an XML object tree for the document first.
	Rooted<xml::Element> html{new xml::Element{
	    mgr, {nullptr}, "html", {{"xlmns", "http://www.w3.org/1999/xhtml"}}}};
	// add the head Element
	Rooted<xml::Element> head{new xml::Element{mgr, html, "head"}};
	html->addChild(head);
	// add the meta element.
	Rooted<xml::Element> meta{
	    new xml::Element{mgr,
	                     head,
	                     "meta",
	                     {{"http-equiv", "Content-Type"},
	                      {"content", "text/html; charset=utf-8"}}}};
	head->addChild(meta);
	// add the title Element with Text
	Rooted<xml::Element> title{new xml::Element{mgr, head, "title"}};
	head->addChild(title);
	title->addChild(
	    new xml::Text(mgr, title, "Test HTML Output for " + doc->getName()));
	// add the body Element
	Rooted<xml::Element> body{new xml::Element{mgr, html, "body"}};
	html->addChild(body);

	// So far was the "preamble". No we have to get to the document content.

	// build the start and end map for annotation processing.
	AnnoMap startMap;
	AnnoMap endMap;
	for (auto &a : doc->getAnnotations()) {
		// we assume uniquely IDed annotations, which should be checked in the
		// validation process.
		startMap.emplace(a->getStart()->getName(), a);
		endMap.emplace(a->getEnd()->getName(), a);
	}

	// extract the book root node.
	Rooted<model::StructuredEntity> root = doc->getRoot();
	if (root->getDescriptor()->getName() != "book") {
		throw OusiaException("The given documents root is no book node!");
	}
	// transform the book node.
	Rooted<xml::Element> book = transformSection(body, root, startMap, endMap);
	// add it as child to the body node.
	body->addChild(book);

	// After the content has been transformed, we serialize it.
	html->serialize(out, "<!DOCTYPE html>");
}

/**
 * This is just for easier internal handling.
 */
enum class SectionType { BOOK, SECTION, SUBSECTION, NONE };

SectionType getSectionType(const std::string &name)
{
	if (name == "book") {
		return SectionType::BOOK;
	} else if (name == "section") {
		return SectionType::SECTION;
	} else if (name == "subsection") {
		return SectionType::SUBSECTION;
	} else {
		return SectionType::NONE;
	}
}

Rooted<xml::Element> DemoHTMLTransformer::transformSection(
    Handle<xml::Element> parent, Handle<model::StructuredEntity> section,
    AnnoMap &startMap, AnnoMap &endMap)
{
	Manager &mgr = section->getManager();
	// check the section type.
	const std::string secclass = section->getDescriptor()->getName();
	SectionType type = getSectionType(secclass);
	if (type == SectionType::NONE) {
		// if the input node is no section, we ignore it.
		return {nullptr};
	}
	// create a div tag containing the sections content.
	Rooted<xml::Element> sec{
	    new xml::Element{mgr, parent, "div", {{"class", secclass}}}};
	// check if we have a heading.
	if (section->hasField("heading") &&
	    section->getField("heading").size() > 0) {
		Rooted<model::StructuredEntity> heading =
		    section->getField("heading")[0];
		std::string headingclass;
		switch (type) {
			case SectionType::BOOK:
				headingclass = "h1";
				break;
			case SectionType::SECTION:
				headingclass = "h2";
				break;
			case SectionType::SUBSECTION:
				headingclass = "h3";
				break;
			case SectionType::NONE:
				// this can not happen;
				break;
		}
		Rooted<xml::Element> h{new xml::Element{mgr, sec, headingclass}};
		sec->addChild(h);
		// extract the heading text, enveloped in a paragraph Element.
		Rooted<xml::Element> h_content =
		    transformParagraph(h, heading, startMap, endMap);
		// We omit the paragraph Element and add the children directly to the
		// heading Element
		for (auto &n : h_content->getChildren()) {
			h->addChild(n);
		}
	}

	// Then we get all the children.
	NodeVector<model::StructuredEntity> mainField = section->getField();
	for (auto &n : mainField) {
		/*
		 * Strictly speaking this is the wrong mechanism, because we would have
		 * to make an "isa" call here because we can not rely on our knowledge
		 * that paragraphs can only be paragraphs or lists. There would have
		 * to be a listener structure of transformations that check if they can
		 * transform this specific node.
		 */
		const std::string childDescriptorName = n->getDescriptor()->getName();
		Rooted<xml::Element> child;
		if (childDescriptorName == "paragraph") {
			child = transformParagraph(sec, n, startMap, endMap);
		} else if (childDescriptorName == "ul" || childDescriptorName == "ol") {
			child = transformList(sec, n, startMap, endMap);
		} else {
			child = transformSection(sec, n, startMap, endMap);
		}
		if (!child.isNull()) {
			sec->addChild(child);
		}
	}
	return sec;
}

Rooted<xml::Element> DemoHTMLTransformer::transformList(
    Handle<xml::Element> parent, Handle<model::StructuredEntity> list,
    AnnoMap &startMap, AnnoMap &endMap)
{
	Manager &mgr = list->getManager();
	// create the list Element, which is either ul or ol (depends on descriptor)
	std::string listclass = list->getDescriptor()->getName();
	Rooted<xml::Element> l{new xml::Element{mgr, parent, listclass}};
	// iterate through list items.
	for (auto &item : list->getField()) {
		std::string itDescrName = item->getDescriptor()->getName();
		if (itDescrName == "item") {
			// create the list item.
			Rooted<xml::Element> li{new xml::Element{mgr, l, "li"}};
			l->addChild(li);
			// extract the item text, enveloped in a paragraph Element.
			Rooted<xml::Element> li_content =
			    transformParagraph(li, item, startMap, endMap);
			// We omit the paragraph Element and add the children directly to
			// the list item
			for (auto &n : li_content->getChildren()) {
				li->addChild(n);
			}
		}
	}
	return l;
}

typedef model::AnnotationEntity::Anchor Anchor;
typedef std::stack<Rooted<model::AnnotationEntity>> AnnoStack;

static Rooted<xml::Element> openAnnotation(
    Manager &mgr, AnnoStack &opened, Handle<model::AnnotationEntity> entity,
    Handle<xml::Element> current)
{
	// we push the newly opened entity on top of the stack.
	opened.push(entity);
	// get the elment name
	std::string elemName = entity->getDescriptor()->getName();
	// emphasized has to be shortened
	if (elemName == "emphasized") {
		elemName = "em";
	}
	// create the new XML element representing the annotation
	Rooted<xml::Element> tmp{new xml::Element{mgr, current, elemName}};
	current->addChild(tmp);
	// and return it.
	return tmp;
}

static std::string escapePredefinedEntities(const std::string &input)
{
	std::stringstream ss;
	for (const char &c : input) {
		switch (c) {
			case '<':
				ss << "&lt;";
				break;
			case '>':
				ss << "&gt;";
				break;
			case '&':
				ss << "&amp;";
				break;
			case '\'':
				ss << "&apos;";
				break;
			case '\"':
				ss << "&quot;";
				break;
			default:
				ss << c;
		}
	}
	return std::move(ss.str());
}

Rooted<xml::Element> DemoHTMLTransformer::transformParagraph(
    Handle<xml::Element> parent, Handle<model::StructuredEntity> par,
    AnnoMap &startMap, AnnoMap &endMap)
{
	Manager &mgr = par->getManager();
	// create the p Element
	Rooted<xml::Element> p{new xml::Element{mgr, parent, "p"}};

	// check if we have a heading.
	if (par->hasField("heading") && par->getField("heading").size() > 0) {
		Rooted<model::StructuredEntity> heading = par->getField("heading")[0];
		// put the heading in a strong xml::Element.
		Rooted<xml::Element> strong{new xml::Element{mgr, p, "strong"}};
		p->addChild(strong);
		// extract the heading text, enveloped in a paragraph Element.
		Rooted<xml::Element> h_content =
		    transformParagraph(strong, heading, startMap, endMap);
		// We omit the paragraph Element and add the children directly to the
		// heading Element
		for (auto &n : h_content->getChildren()) {
			strong->addChild(n);
		}
	}

	// transform paragraph children to XML as well
	/*
	 * We need a stack of AnnotationEntities that are currently open.
	 * In principle we wouldn't, because the nested structure of XML elements
	 * provides a stack-like structure anyways, but we need to have a mapping of
	 * XML tags to AnnotationEntities, which is implicitly provided by this
	 * stack.
	 */
	AnnoStack opened;
	// this is a handle for our current XML element for annotation handling.
	Rooted<xml::Element> current = p;
	for (auto &n : par->getField()) {
		if (n->isa(typeOf<Anchor>())) {
			// check if this is a start Anchor.
			// here we assume, again, that the ids/names of anchors are unique.
			auto it = startMap.find(n->getName());
			if (it != startMap.end()) {
				// if we have a start anchor, we open an annotation element.
				current = openAnnotation(mgr, opened, it->second, current);
				continue;
			}
			// check if this is an end Anchor.
			auto it2 = endMap.find(n->getName());
			if (it2 != endMap.end()) {
				/*
				 * Now it gets somewhat interesting: We have to close all
				 * tags that started after the one that is closed now and
				 * re-open them afterwards. So we create a lokal stack to
				 * temporarily store all AnnotationEntities that need to
				 * be re-opened.
				 */
				AnnoStack tmp;
				Rooted<model::AnnotationEntity> closed = opened.top();
				opened.pop();
				while (closed->getEnd()->getName() != n->getName()) {
					/*
					 * We implicitly do close tags by climbing up the XML tree
					 * until we are at the right element.
					 */
					current = current->getParent();
					tmp.push(closed);
					if (opened.empty()) {
						// if we have no opened entities left, that is a
						// malformed document.
						throw OusiaException("An unopened entity was closed!");
					}
					closed = opened.top();
					opened.top();
				}
				// At this point we have closed all necessary entities. Now we
				// need to re-open some of them.
				while (!tmp.empty()) {
					closed = tmp.top();
					tmp.pop();
					current = openAnnotation(mgr, opened, closed, current);
				}
			}
			continue;
		}
		// if this is not an anchor, we can only handle text.
		std::string childDescriptorName = n->getDescriptor()->getName();
		if (childDescriptorName == "text") {
			Handle<model::DocumentPrimitive> primitive =
			    n->getField()[0].cast<model::DocumentPrimitive>();
			if (primitive.isNull()) {
				throw OusiaException("Text field is not primitive!");
			}
			// here we need to do some escaping with the string content.
			std::string escaped =
			    escapePredefinedEntities(primitive->getContent().asString());
			current->addChild(new xml::Text(mgr, current, escaped));
		}
	}
	return p;
}
}
}
