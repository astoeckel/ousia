
#include "XML.hpp"

namespace ousia {
namespace xml {

void Node::serialize(std::ostream &out, const std::string &doctype)
{
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	if (doctype != "") {
		out << doctype << "\n";
	}
	doSerialize(out, 0);
}

void Element::doSerialize(std::ostream &out, unsigned int tabdepth)
{
	for (unsigned int t = 0; t < tabdepth; t++) {
		out << '\t';
	}
	out << '<' << name;
	for (auto &a : attributes) {
		out << ' ' << a.first << "=\"" << a.second << '\"';
	}
	// if we have no children, we close the tag immediately.
	if (children.size() == 0) {
		out << "/>\n";
	} else {
		out << ">\n";
		for (auto &n : children) {
			n->doSerialize(out, tabdepth + 1);
		}
		for (unsigned int t = 0; t < tabdepth; t++) {
			out << '\t';
		}
		out << "</" << name << ">\n";
	}
}

void Text::doSerialize(std::ostream &out, unsigned int tabdepth)
{
	for (unsigned int t = 0; t < tabdepth; t++) {
		out << '\t';
	}
	out << text << '\n';
}
}
}
