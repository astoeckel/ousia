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

#include <stack>

#include <core/common/Exceptions.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Variant.hpp>

#include "DemoOutput.hpp"

namespace ousia {
namespace html {

typedef std::stack<Rooted<AnnotationEntity>> AnnoStack;

static bool canHandleAnchor(Handle<Anchor> a)
{
	std::string annoClassName = a->getAnnotation()->getDescriptor()->getName();
	return annoClassName == "emph" || annoClassName == "strong";
}

static Rooted<xml::Element> openAnnotation(Manager &mgr, AnnoStack &opened,
                                           Handle<AnnotationEntity> entity,
                                           Handle<xml::Element> current,
                                           bool stackOnly)
{
	// we push the newly opened entity on top of the stack.
	opened.push(entity);
	if (stackOnly) {
		return nullptr;
	}
	// get the elment name
	std::string elemName = entity->getDescriptor()->getName();
	// emphasized has to be shortened
	if (elemName == "emph") {
		elemName = "em";
	}
	// create the new XML element representing the annotation
	Rooted<xml::Element> tmp{new xml::Element{mgr, current, elemName}};
	current->addChild(tmp);
	// and return it.
	return tmp;
}

static Rooted<xml::Element> transformAnchor(Manager &mgr, Handle<Anchor> a,
                                            Handle<xml::Element> current,
                                            Logger &logger, AnnoStack &opened,
                                            bool stackOnly)
{
	// check if this is a start Anchor.
	if (a->isStart()) {
		// if we have a start anchor, we open an annotation element.
		current =
		    openAnnotation(mgr, opened, a->getAnnotation(), current, stackOnly);
		// check if this is an end Anchor.
	} else if (a->isEnd()) {
		/*
		 * Now it gets somewhat interesting: We have to close all
		 * tags that started after the one that is closed now and
		 * re-open them afterwards. So we create a lokal stack to
		 * temporarily store all AnnotationEntities that need to
		 * be re-opened.
		 */
		AnnoStack tmp;
		if (opened.empty()) {
			// if we have no opened entities left, that is a
			// malformed document.
			logger.error("An unopened entity was closed!", *a);
			return current;
		}
		Rooted<AnnotationEntity> closed = opened.top();
		current = current->getParent();
		opened.pop();
		while (closed != a->getAnnotation()) {
			/*
			 * We implicitly close tags by climbing up the XML tree
			 * until we are at the right element.
			 */
			current = current->getParent();
			tmp.push(closed);
			if (opened.empty()) {
				// if we have no opened entities left, that is a
				// malformed document.
				logger.error("An unopened entity was closed!", *a);
				return current;
			}
			closed = opened.top();
			opened.pop();
		}
		// At this point we have closed all necessary entities. Now we
		// need to re-open some of them.
		while (!tmp.empty()) {
			closed = tmp.top();
			tmp.pop();
			current = openAnnotation(mgr, opened, closed, current, stackOnly);
		}
	}
	// otherwise it is a disconnected Anchor and we can ignore it.
	return current;
}

/**
 * Reopens all Annotations in the given AnnoStack but does not manipulate the
 * original stack. The input argument is a copy.
 * @return the innermost opened element.
 */
static Rooted<xml::Element> reOpenAnnotations(Manager &mgr, AnnoStack opened,
                                              Handle<xml::Element> parent)
{
	AnnoStack tmp;
	while (!opened.empty()) {
		tmp.push(opened.top());
		opened.pop();
	}
	Rooted<xml::Element> current = parent;
	while (!tmp.empty()) {
		Rooted<AnnotationEntity> closed = tmp.top();
		tmp.pop();
		current = openAnnotation(mgr, opened, closed, current, false);
	}
	return current;
}

static Rooted<xml::Element> transformParagraph(Manager &mgr,
                                               Handle<xml::Element> parent,
                                               Handle<StructuredEntity> par,
                                               Logger &logger,
                                               AnnoStack &opened)
{
	// create the p Element
	Rooted<xml::Element> p{new xml::Element{mgr, parent, "p"}};

	// check if we have a heading.
	if (par->getDescriptor()->hasField("heading") &&
	    par->getField("heading").size() > 0) {
		Handle<StructuredEntity> heading =
		    par->getField("heading")[0].cast<StructuredEntity>();
		// put the heading in a strong xml::Element.
		Rooted<xml::Element> strong{new xml::Element{mgr, p, "strong"}};
		p->addChild(strong);
		// extract the heading text, enveloped in a paragraph Element.
		// in this case we use an empy annotation stack because annotations do
		// not extend on subtree fields.
		AnnoStack emptyStack;
		Rooted<xml::Element> h_content =
		    transformParagraph(mgr, strong, heading, logger, emptyStack);
		// We omit the paragraph Element and add the children directly to the
		// heading Element
		for (auto &n : h_content->getChildren()) {
			strong->addChild(n);
		}
	}
	// reopen all annotations.
	Rooted<xml::Element> current = reOpenAnnotations(mgr, opened, p);
	// transform paragraph children to XML as well
	for (auto &n : par->getField()) {
		if (n->isa(&RttiTypes::Anchor)) {
			Rooted<Anchor> a = n.cast<Anchor>();
			if (canHandleAnchor(a)) {
				current =
				    transformAnchor(mgr, a, current, logger, opened, false);
			}
			continue;
		}
		// if this is not an anchor, we can only handle text.
		if (!n->isa(&RttiTypes::StructuredEntity)) {
			continue;
		}
		Handle<StructuredEntity> t = n.cast<StructuredEntity>();

		std::string childDescriptorName = t->getDescriptor()->getName();
		if (childDescriptorName == "text") {
			Handle<DocumentPrimitive> primitive =
			    t->getField()[0].cast<DocumentPrimitive>();
			std::string text_content = primitive->getContent().asString();
			current->addChild(new xml::Text(mgr, current, text_content));
		}
	}
	// at this point we implicitly close all annotations that are left opened.
	// they will be reopened in the next paragraph.
	return p;
}

static Rooted<xml::Element> transformList(Manager &mgr,
                                          Handle<xml::Element> parent,
                                          Handle<StructuredEntity> list,
                                          Logger &logger, AnnoStack &opened)
{
	// create the list Element, which is either ul or ol (depends on descriptor)
	std::string listclass = list->getDescriptor()->getName();
	Rooted<xml::Element> l{new xml::Element{mgr, parent, listclass}};
	// iterate through list items.
	for (auto &it : list->getField()) {
		if (it->isa(&RttiTypes::Anchor)) {
			Rooted<Anchor> a = it.cast<Anchor>();
			if (canHandleAnchor(a)) {
				// just put the entity on the AnnoStack, but do not open it
				// explicitly. That will be done inside the next paragraph.
				transformAnchor(mgr, a, l, logger, opened, true);
			}
			continue;
		}
		Handle<StructuredEntity> item = it.cast<StructuredEntity>();
		std::string itDescrName = item->getDescriptor()->getName();
		if (itDescrName == "item") {
			// create the list item.
			Rooted<xml::Element> li{new xml::Element{mgr, l, "li"}};
			l->addChild(li);
			// extract the item text, enveloped in a paragraph Element.
			Rooted<xml::Element> li_content =
			    transformParagraph(mgr, li, item, logger, opened);
			// We omit the paragraph Element and add the children directly to
			// the list item
			for (auto &n : li_content->getChildren()) {
				li->addChild(n);
			}
		}
	}
	return l;
}

/**
 * This is just for easier internal handling.
 */
enum class SectionType { BOOK, CHAPTER, SECTION, SUBSECTION, NONE };

static SectionType getSectionType(const std::string &name)
{
	if (name == "book") {
		return SectionType::BOOK;
	} else if (name == "chapter") {
		return SectionType::CHAPTER;
	} else if (name == "section") {
		return SectionType::SECTION;
	} else if (name == "subsection") {
		return SectionType::SUBSECTION;
	} else {
		return SectionType::NONE;
	}
}

static Rooted<xml::Element> transformSection(Manager &mgr,
                                             Handle<xml::Element> parent,
                                             Handle<StructuredEntity> section,
                                             Logger &logger, AnnoStack &opened)
{
	// check the section type.
	const std::string secclass = section->getDescriptor()->getName();
	SectionType type = getSectionType(secclass);
	if (type == SectionType::NONE) {
		// if the input node is no section, we ignore it.
		return {nullptr};
	}
	// create a section tag containing the sections content.
	Rooted<xml::Element> sec{
	    new xml::Element{mgr, parent, "section", {{"class", secclass}}}};
	// check if we have a heading.
	if (section->getDescriptor()->hasField("heading") &&
	    section->getField("heading").size() > 0) {
		Handle<StructuredEntity> heading =
		    section->getField("heading")[0].cast<StructuredEntity>();
		std::string headingclass;
		switch (type) {
			case SectionType::BOOK:
				headingclass = "h1";
				break;
			case SectionType::CHAPTER:
				headingclass = "h2";
				break;
			case SectionType::SECTION:
				headingclass = "h3";
				break;
			case SectionType::SUBSECTION:
				headingclass = "h4";
				break;
			case SectionType::NONE:
				// this can not happen;
				break;
		}
		Rooted<xml::Element> h{new xml::Element{mgr, sec, headingclass}};
		sec->addChild(h);
		// extract the heading text, wrapped in a paragraph Element.
		// in this case we use an empy annotation stack because annotations do
		// not extend on subtree fields.
		AnnoStack emptyStack;
		Rooted<xml::Element> h_content =
		    transformParagraph(mgr, h, heading, logger, emptyStack);
		// We omit the paragraph element and add the children directly to the
		// heading Element
		for (auto &n : h_content->getChildren()) {
			h->addChild(n);
		}
	}

	// Then we get all the children.
	for (auto &n : section->getField()) {
		if (n->isa(&RttiTypes::Anchor)) {
			Rooted<Anchor> a = n.cast<Anchor>();
			if (canHandleAnchor(a)) {
				// just put the entity on the AnnoStack, but do not open it
				// explicitly. That will be done inside the next paragraph.
				transformAnchor(mgr, a, sec, logger, opened, true);
			}
			continue;
		}
		if (!n->isa(&RttiTypes::StructuredEntity)) {
			continue;
		}
		Handle<StructuredEntity> s = n.cast<StructuredEntity>();
		/*
		 * Strictly speaking this is the wrong mechanism, because we would have
		 * to make an "isa" call here because we can not rely on our knowledge
		 * that paragraphs can only be paragraphs or lists. There would have
		 * to be a listener structure of transformations that check if they can
		 * transform this specific node.
		 */
		const std::string childDescriptorName = s->getDescriptor()->getName();
		Rooted<xml::Element> child;
		if (childDescriptorName == "paragraph") {
			child = transformParagraph(mgr, sec, s, logger, opened);
		} else if (childDescriptorName == "ul" || childDescriptorName == "ol") {
			child = transformList(mgr, sec, s, logger, opened);
		} else {
			child = transformSection(mgr, sec, s, logger, opened);
		}
		if (!child.isNull()) {
			sec->addChild(child);
		}
	}
	return sec;
}

void DemoHTMLTransformer::writeHTML(Handle<Document> doc, std::ostream &out,
                                    Logger &logger, bool pretty)
{
	// validate the document.
	if (!doc->validate(logger)) {
		return;
	}

	Manager &mgr = doc->getManager();
	// initialize an empty annotation Stack.
	AnnoStack opened;
	// Create an XML object tree for the document first.
	Rooted<xml::Element> html{new xml::Element{mgr, {nullptr}, "html"}};
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
	// add some stylish styles
	Rooted<xml::Element> style{
	    new xml::Element{mgr, head, "style", {{"type", "text/css"}}}};
	head->addChild(style);
	Rooted<xml::Text> css{new xml::Text{mgr, style,
	                                    "body { font-family: 'CMU Serif', "
	                                    "serif;}\n p { text-align: justify; "
	                                    "hyphens: auto; }"}};
	style->addChild(css);

	// add the body Element
	Rooted<xml::Element> body{new xml::Element{mgr, html, "body"}};
	html->addChild(body);

	// So far was the "preamble". No we have to get to the document content.

	// extract the book root node.
	Rooted<StructuredEntity> root = doc->getRoot();
	if (root->getDescriptor()->getName() != "book") {
		throw OusiaException("The given documents root is no book node!");
	}
	// transform the book node.
	Rooted<xml::Element> book =
	    transformSection(mgr, body, root, logger, opened);
	// add it as child to the body node.
	body->addChild(book);

	// After the content has been transformed, we serialize it.
	html->serialize(out, "<!DOCTYPE html>", pretty);
}
}
}