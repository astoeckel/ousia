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
#include <core/common/TypedRttiBuilder.hpp>

#include "XML.hpp"

namespace ousia {
namespace xml {

void Node::serialize(std::ostream &out, const std::string &doctype, bool pretty)
{
	if (doctype != "") {
		out << doctype;
		if (pretty) {
			out << '\n';
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

void Element::doSerialize(std::ostream &out, unsigned int tabdepth, bool pretty)
{
	if (pretty) {
		for (unsigned int t = 0; t < tabdepth; t++) {
			out << '\t';
		}
	}
	out << '<' << name;
	for (auto &a : attributes) {
		out << ' ' << a.first << "=\"" << escapePredefinedEntities(a.second)
		    << '\"';
	}
	// if we have no children, we close the tag immediately.
	if (children.size() == 0) {
		out << "/>";
		if (pretty) {
			out << '\n';
		}
	} else {
		out << ">";
		if (pretty) {
			out << '\n';
		}
		for (auto &n : children) {
			n->doSerialize(out, tabdepth + 1, pretty);
		}
		if (pretty) {
			for (unsigned int t = 0; t < tabdepth; t++) {
				out << '\t';
			}
		}
		out << "</" << name << ">";
		if (pretty) {
			out << '\n';
		}
	}
}

void Text::doSerialize(std::ostream &out, unsigned int tabdepth, bool pretty)
{
	if (pretty) {
		for (unsigned int t = 0; t < tabdepth; t++) {
			out << '\t';
		}
	}
	out << escapePredefinedEntities(text);
	if (pretty) {
		out << '\n';
	}
}
}

namespace RttiTypes
{
	const Rtti<xml::Node> XMLNode = RttiBuilder("XMLNode");
	const Rtti<xml::Element> XMLElement =
	    TypedRttiBuilder<xml::Element>("XMLElement")
	        .parent(&XMLNode)
	        .composedOf(&XMLNode)
	        .property("name", {RttiTypes::String,
	                           {[](const xml::Element *obj) {
		                           return Variant::fromString(obj->name);
		                       }}});
	const Rtti<xml::Text> XMLText = RttiBuilder("XMLText").parent(&XMLNode);
}
}
