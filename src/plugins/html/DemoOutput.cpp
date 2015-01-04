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

#include "DemoOutput.hpp"

namespace ousia {
namespace html {

void DemoHTMLTransformer::writeHTML(Handle<model::Document> doc,
                                    std::ostream &out)
{
	// write preamble
	out << "<?xml version=\" 1.0 \"?>\n";
	out << "<html>\n";
	out << "\t<head>\n";
	out << "\t\t<title>Test HTML Output for " << doc->getName() << "</title>\n";
	out << "\t</head>\n";
	out << "\t<body>\n";

	// look for the book root node.
	Rooted<model::StructuredEntity> root = doc->getRoot();
	if (root->getDescriptor()->getName() != "book") {
		throw OusiaException("The given documents root is no book node!");
	}
	// write it to HTML.
	writeSection(root, out);
	// write end
	out << "\t</body>\n";
	out << "</html>\n";
}

/**
 * This is just for easier internal handling.
 */
enum class SectionType { BOOK, CHAPTER, SECTION, SUBSECTION, NONE };

SectionType getSectionType(const std::string &name)
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

void DemoHTMLTransformer::writeSection(Handle<model::StructuredEntity> sec,
                                       std::ostream &out)
{
	// check the section type.
	SectionType type = getSectionType(sec->getDescriptor()->getName());
	if (type == SectionType::NONE) {
		// if the input node is no section, we ignore it.
		return;
	}
	// check if we have a heading.
	if (sec->hasField("heading")) {
		Rooted<model::StructuredEntity> heading = sec->getField("heading")[0];
		out << "\t\t";
		switch (type) {
			case SectionType::BOOK:
				out << "<h1>";
				break;
			case SectionType::CHAPTER:
				out << "<h2>";
				break;
			case SectionType::SECTION:
				out << "<h3>";
				break;
			case SectionType::SUBSECTION:
				out << "<h4>";
				break;
			case SectionType::NONE:
				// this can not happen;
				break;
		}
		// the second field marks the heading. So let's write it.
		writeParagraph(heading, out, false);
		// close the heading tag.
		switch (type) {
			case SectionType::BOOK:
				out << "</h1>";
				break;
			case SectionType::CHAPTER:
				out << "</h2>";
				break;
			case SectionType::SECTION:
				out << "</h3>";
				break;
			case SectionType::SUBSECTION:
				out << "</h4>";
				break;
			case SectionType::NONE:
				// this can not happen;
				break;
		}
		out << "\n";
	}

	// then write the section content recursively.
	NodeVector<model::StructuredEntity> mainField = sec->getField();
	for (auto &n : mainField) {
		/*
		 * Strictly speaking this is the wrong mechanism, because we would have
		 * to make an "isa" call here because we can not rely on our knowledge
		 * that paragraphs can only be paragraphs or lists. There would have
		 * to be a listener structure of transformations that check if they can
		 * transform this specific node.
		 */
		std::string childDescriptorName = n->getDescriptor()->getName();
		if (childDescriptorName == "paragraph") {
			writeParagraph(n, out);
			// TODO: Implement
			//		} else if(childDescriptorName == "ul"){
			//			writeList(n, out);
		} else {
			writeSection(n, out);
		}
	}
}

void DemoHTMLTransformer::writeParagraph(Handle<model::StructuredEntity> par,
                                         std::ostream &out, bool writePTags)
{
	// validate descriptor.
	if (par->getDescriptor()->getName() != "paragraph") {
		throw OusiaException("Expected paragraph!");
	}
	// check if we have a heading.
	if (par->hasField("heading")) {
		Rooted<model::StructuredEntity> heading = par->getField("heading")[0];
		// start the heading tag
		out << "\t\t<h5>";
		// the second field marks the heading. So let's write it.
		writeParagraph(heading, out, false);
		// close the heading tag.
		out << "</h5>\n";
	}
	// write start tag
	if (writePTags) {
		out << "\t\t<p>";
	}
	// write content
	// TODO: What about emphasis?
	for (auto &text : par->getField()) {
		if (text->getDescriptor()->getName() != "text") {
			throw OusiaException("Expected text!");
		}
		Handle<model::DocumentPrimitive> primitive =
		    text->getField()[0].cast<model::DocumentPrimitive>();
		if (primitive.isNull()) {
			throw OusiaException("Text field is not primitive!");
		}
		out << primitive->getContent().asString();
	}
	// write end tag
	if (writePTags) {
		out << "</p>\n";
	}
}
}
}
