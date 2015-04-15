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

#include <core/common/Rtti.hpp>
#include <core/common/RttiBuilder.hpp>

#include "XML.hpp"

namespace ousia {
namespace xml {

void Node::serialize(std::ostream &out, const std::string &doctype, bool pretty)
{
	if (doctype != "") {
		out << doctype;
		if (pretty) {
			out << std::endl;
		}
	}
	doSerialize(out, 0, pretty);
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
			case '\"':
				ss << "&quot;";
				break;
			default:
				ss << c;
		}
	}
	return std::move(ss.str());
}

void Element::doSerialize(std::ostream &out, unsigned int tabdepth, bool pretty)
{
	bool hasText = false;
	if (pretty) {
		// print tabs at the beginning, if we are in pretty mode.
		for (unsigned int t = 0; t < tabdepth; t++) {
			out << '\t';
		}
		/*
		 * if we are in pretty mode we also need to check if we have a text
		 * node as child.
		 * If so this changes our further output processing because of the way
		 * XML treats primitive data: The structure
		 *
		 * \code{.xml}
		 * <Element name="A">
		 *   <Text>content</Text>
		 *   <Text>content2</Text>
		 * </Element>
		 * \endcode
		 *
		 * has to be serialized as
		 *
		 * \code{.xml}
		 * <A>contentcontent2</A>
		 * \endcode
		 *
		 * because otherwise we introduce whitespaces and newlines where no
		 * such things had been before.
		 *
		 * On the other hand the structure
		 *
		 * \code{.xml}
		 * <Element name="A">
		 *   <Element name="B">
		 *     <Text>content</Text>
		 *   </Element>
		 * </Element>
		 * \endcode
		 *
		 * Can be serialized as
		 *
		 * \code{.xml}
		 * <A>
		 *   <B>content</B>
		 * </A>
		 * \endcode
		 *
		 * As last example consider the case
		 *
		 * \code{.xml}
		 * <Element name="A">
		 *   <Element name="B">
		 *     <Text>content</Text>
		 *   </Element>
		 *   <Text>content2</Text>
		 * </Element>
		 * \endcode
		 *
		 * Here the A-Element again has primitive text content, such that we
		 * are not allowed to prettify. It has to be serialized like this:
		 *
		 * \code{.xml}
		 * <A><B>content</B>content2</A>
		 * \endcode
		 *
		 *
		 */
		for (auto n : children) {
			if (n->isa(&RttiTypes::XMLText)) {
				hasText = true;
				break;
			}
		}
	}

	out << '<';
	if (!nspace.empty()) {
		out << nspace << ":";
	}
	out << name;
	for (auto &a : attributes) {
		out << ' ' << a.first << "=\"" << escapePredefinedEntities(a.second)
		    << '\"';
	}
	// if we have no children, we close the tag immediately.
	if (children.size() == 0) {
		out << "/>";
		if (pretty) {
			out << std::endl;
		}
	} else {
		out << ">";
		if (pretty && !hasText) {
			out << std::endl;
		}
		for (auto n : children) {
			n->doSerialize(out, tabdepth + 1, pretty && !hasText);
		}
		if (pretty && !hasText) {
			for (unsigned int t = 0; t < tabdepth; t++) {
				out << '\t';
			}
		}
		out << "</";
		if (!nspace.empty()) {
			out << nspace << ":";
		}
		out << name << ">";
		if (pretty) {
			out << std::endl;
		}
	}
}

void Text::doSerialize(std::ostream &out, unsigned int tabdepth, bool pretty)
{
	out << escapePredefinedEntities(text);
}
}

namespace RttiTypes {
const Rtti XMLNode = RttiBuilder<xml::Node>("XMLNode");
const Rtti XMLElement =
    RttiBuilder<xml::Element>("XMLElement")
        .parent(&XMLNode)
        .composedOf(&XMLNode)
        .property("name", {&RttiTypes::String,
                           {[](const xml::Element *obj) {
	                           return Variant::fromString(obj->getName());
	                       }}});
const Rtti XMLText = RttiBuilder<xml::Text>("XMLText").parent(&XMLNode);
}
}