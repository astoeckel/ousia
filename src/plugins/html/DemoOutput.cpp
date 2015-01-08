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
	    mgr, "html", {{"xlmns", "http://www.w3.org/1999/xhtml"}}}};
	// add the head Element
	Rooted<xml::Element> head{new xml::Element{mgr, "head"}};
	html->children.push_back(head);
	// add the meta element.
	Rooted<xml::Element> meta{
	    new xml::Element{mgr,
	                     "meta",
	                     {{"http-equiv", "Content-Type"},
	                      {"content", "text/html; charset=utf-8"}}}};
	head->children.push_back(meta);
	// add the title Element with Text
	Rooted<xml::Element> title{new xml::Element{mgr, "title"}};
	head->children.push_back(title);
	title->children.push_back(
	    new xml::Text(mgr, "Test HTML Output for " + doc->getName()));
	// add the body Element
	Rooted<xml::Element> body{new xml::Element{mgr, "body"}};
	html->children.push_back(body);

	// So far was the "preamble". No we have to get to the document content.

	// extract the book root node.
	Rooted<model::StructuredEntity> root = doc->getRoot();
	if (root->getDescriptor()->getName() != "book") {
		throw OusiaException("The given documents root is no book node!");
	}
	// transform the book node.
	Rooted<xml::Element> book = transformSection(root);
	// add it as child to the body node.
	body->children.push_back(book);

	// After the content has been transformed, we serialize it.
	html->serialize(
	    out,
	    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
	    "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
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
    Handle<model::StructuredEntity> section)
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
	    new xml::Element{mgr, "div", {{"class", secclass}}}};
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
		Rooted<xml::Element> h{new xml::Element{mgr, headingclass}};
		sec->children.push_back(h);
		// extract the heading text, enveloped in a paragraph Element.
		Rooted<xml::Element> h_content = transformParagraph(heading);
		// We omit the paragraph Element and add the children directly to the
		// heading Element
		for (auto &n : h_content->children) {
			h->children.push_back(n);
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
			child = transformParagraph(n);
		} else if (childDescriptorName == "ul" || childDescriptorName == "ol") {
			child = transformList(n);
		} else {
			child = transformSection(n);
		}
		if (!child.isNull()) {
			sec->children.push_back(child);
		}
	}
	return sec;
}

Rooted<xml::Element> DemoHTMLTransformer::transformParagraph(
    Handle<model::StructuredEntity> par)
{
	Manager &mgr = par->getManager();
	// create the p Element
	Rooted<xml::Element> p{new xml::Element{mgr, "p"}};

	// check if we have a heading.
	if (par->hasField("heading") && par->getField("heading").size() > 0) {
		Rooted<model::StructuredEntity> heading = par->getField("heading")[0];
		// put the heading in a strong xml::Element.
		Rooted<xml::Element> strong{new xml::Element{mgr, "strong"}};
		p->children.push_back(strong);
		// extract the heading text, enveloped in a paragraph Element.
		Rooted<xml::Element> h_content = transformParagraph(heading);
		// We omit the paragraph Element and add the children directly to the
		// heading Element
		for (auto &n : h_content->children) {
			strong->children.push_back(n);
		}
	}

	// transform paragraph children to XML as well
	for (auto &n : par->getField()) {
		std::string childDescriptorName = n->getDescriptor()->getName();
		if (childDescriptorName == "text") {
			Handle<model::DocumentPrimitive> primitive =
			    n->getField()[0].cast<model::DocumentPrimitive>();
			if (primitive.isNull()) {
				throw OusiaException("Text field is not primitive!");
			}
			p->children.push_back(
			    new xml::Text(mgr, primitive->getContent().asString()));
		}
		// TODO: Handle non-text content
	}
	return p;
}

Rooted<xml::Element> DemoHTMLTransformer::transformList(
    Handle<model::StructuredEntity> list)
{
	Manager &mgr = list->getManager();
	// create the list Element, which is either ul or ol (depends on descriptor)
	std::string listclass = list->getDescriptor()->getName();
	Rooted<xml::Element> l{new xml::Element{mgr, listclass}};
	// iterate through list items.
	for (auto &item : list->getField()) {
		std::string itDescrName = item->getDescriptor()->getName();
		if (itDescrName == "item") {
			// create the list item.
			Rooted<xml::Element> li{new xml::Element{mgr, "li"}};
			l->children.push_back(li);
			// extract the item text, enveloped in a paragraph Element.
			Rooted<xml::Element> li_content = transformParagraph(item);
			// We omit the paragraph Element and add the children directly to
			// the list item
			for (auto &n : li_content->children) {
				li->children.push_back(n);
			}
		}
	}
	return l;
}
}
}
