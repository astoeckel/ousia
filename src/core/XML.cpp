
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

void Element::doSerialize(std::ostream &out, unsigned int tabdepth, bool pretty)
{
	if (pretty) {
		for (unsigned int t = 0; t < tabdepth; t++) {
			out << '\t';
		}
	}
	out << '<' << name;
	for (auto &a : attributes) {
		out << ' ' << a.first << "=\"" << a.second << '\"';
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
	out << text;
	if (pretty) {
		out << '\n';
	}
}
}

namespace RttiTypes {
const Rtti<xml::Node> XMLNode = RttiBuilder("XMLNode");
const Rtti<xml::Element> XMLElement =
    RttiBuilder("XMLElement").parent(&XMLNode).composedOf(&XMLNode);
const Rtti<xml::Text> XMLText = RttiBuilder("XMLText").parent(&XMLNode);
}
}
