
#include "XML.hpp"

namespace ousia {
namespace xml {

void Element::serialize(std::ostream& out, unsigned int tabdepth)
{
	for (unsigned int t = 0; t < tabdepth; t++) {
		out << '\t';
	}
	out << '<' << name;
	for (auto &a : attributes) {
		out << ' ' << a.first << "=\"" << a.second << '\"';
	}
	out << ">\n";
	for (auto &n : children) {
		n->serialize(out, tabdepth + 1);
	}
	for (unsigned int t = 0; t < tabdepth; t++) {
		out << '\t';
	}
	out << "</" << name << ">\n";
}

void Text::serialize(std::ostream& out, unsigned int tabdepth)
{
	for (unsigned int t = 0; t < tabdepth; t++) {
		out << '\t';
	}
	out << text << '\n';
}
}
}
