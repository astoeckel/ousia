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
 * This implements a Demo HTML output for the following ontologies:
 * * book
 * * headings
 * * emphasis
 * * lists
 *
 * @author Benjamin Paassen - bpaassen@techfak.uni-bielefeld.de
 */
#ifndef _OUSIA_HTML_DEMO_OUTPUT_HPP_
#define _OUSIA_HTML_DEMO_OUTPUT_HPP_

#include <ostream>

#include <core/model/Document.hpp>
#include <core/XML.hpp>

namespace ousia {
namespace html {

class DemoHTMLTransformer {

public:
	/**
	 * This writes a HTML representation of the given document to the given
	 * output stream. Note that this method lacks the generality of our Ousia
	 * approach with respect to two important points:
	 * 1.) It hardcodes the dependency to a certain set of ontologies in the C++
	 *     code.
	 * 2.) It does not use the proposed pipeline of first copying the document
	 *     graph, then attaching style attributes and then transforming it to a
	 *     specific output format but does all of these steps at once.
	 * 3.) It does not use different transformers for the different ontologies but
	 *     does all transformations at once.
	 * Therefore this is not an adequate model of our algorithms but only a
	 * Demo.
	 *
	 * @param doc    is a Document using concepts of the book, headings,
	 *               emphasis and lists ontologies but no other.
	 * @param out    is the output stream the data shall be written to.
	 * @param logger is a logger instances for errors.
	 * @param pretty is a flag that manipulates whether newlines and tabs are
	 *               used.
	 */
	void writeHTML(Handle<Document> doc, std::ostream &out, Logger& logger, bool pretty = true);
};
}
}

#endif
