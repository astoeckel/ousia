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

/**
 * @file DemoOutput.hpp
 *
 * This implements a Demo HTML output for the following domains:
 * * book
 * * headings
 * * emphasis
 * * lists
 *
 * @author Benjamin Paassen - bpaassen@techfak.uni-bielefeld.de
 */
#ifndef _OUSIA_HTML_DEMO_OUTPUT_HPP_
#define _OUSIA_HTML_DEMO_OUTPUT_HPP_

#include <map>
#include <ostream>

#include <core/model/Document.hpp>
#include <core/XML.hpp>

namespace ousia {
namespace html {

typedef std::map<std::string, Rooted<model::AnnotationEntity>> AnnoMap;

class DemoHTMLTransformer {
private:
	/**
	 * This transforms a section-like entity, namely book, section
	 * and subsection, to an XHTML element, including its header. For the
	 * children of the default field the respective transform function is
	 * called recursively.
	 */
	Rooted<xml::Element> transformSection(Handle<xml::Element> parent,
	                                      Handle<model::StructuredEntity> sec,
	                                      AnnoMap &startMap, AnnoMap &endMap);
	/**
	 * This transforms a list entity, namely ul and ol to an XHTML element.
	 * For each item, the transformParagraph function is called.
	 */
	Rooted<xml::Element> transformList(Handle<xml::Element> parent,
	                                   Handle<model::StructuredEntity> list,
	                                   AnnoMap &startMap, AnnoMap &endMap);
	/**
	 * This transforms a paragraph-like entity, namely heading, item and
	 * paragraph, to an XHTML element including the text and the anchors
	 * contained. For anchor handling we require the AnnoMaps.
	 */
	Rooted<xml::Element> transformParagraph(Handle<xml::Element> parent,
	                                        Handle<model::StructuredEntity> par,
	                                        AnnoMap &startMap, AnnoMap &endMap);

public:
	/**
	 * This writes a HTML representation of the given document to the given
	 * output stream. Note that this method lacks the generality of our Ousia
	 * approach with respect to two important points:
	 * 1.) It hardcodes the dependency to a certain set of domains in the C++
	 *     code.
	 * 2.) It does not use the proposed pipeline of first copying the document
	 *     graph, then attaching style attributes and then transforming it to a
	 *     specific output format but does all of these steps at once.
	 * 3.) It does not use different transformers for the different domains but
	 *     does all transformations at once.
	 * Therefore this is not an adequate model of our algorithms but only a
	 * Demo.
	 *
	 * @param doc is a Document using concepts of the book, headings, emphasis
	 *            and lists domains but no other.
	 * @param out is the output stream the data shall be written to.
	 */
	void writeHTML(Handle<model::Document> doc, std::ostream &out);
};
}
}

#endif
