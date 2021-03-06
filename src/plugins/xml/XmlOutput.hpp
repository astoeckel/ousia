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
 * @file XmlOutput.hpp
 *
 * This provices an Output generator to serialize any given document to XML.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */
#ifndef _OUSIA_XML_OUTPUT_HPP_
#define _OUSIA_XML_OUTPUT_HPP_

#include <ostream>

#include <core/resource/ResourceManager.hpp>
#include <core/model/Document.hpp>
#include <core/XML.hpp>

namespace ousia {
namespace xml {

class XmlTransformer {

public:
	/**
	 * This writes an XML serialization of the given document to the given
	 * output stream. The serialization is  equivalent to the input XML format,
	 * safe for the ontology references. TODO: Can we change this? If so: how?
	 *
	 * @param doc    is some Document.
	 * @param out    is the output stream the XML serialization of the document
	 *               shall be written to.
	 * @param logger is the logger errors shall be written to.
	 * @param resMgr is the ResourceManager to locate the ontologies and
	 *               typesystems that were imported in this document.
	 * @param pretty is a flag that manipulates whether newlines and tabs are
	 *               used.
	 * @param flat   if this flag is set the result will be a 'standalone'
	 *               version of the document including serialized versions of
	 *               all referenced ontologies and typesystems.
	 */
	void writeXml(Handle<Document> doc, std::ostream &out, Logger &logger,
	              ResourceManager &resMgr, bool pretty = true,
	              bool flat = false);
};
}
}
#endif