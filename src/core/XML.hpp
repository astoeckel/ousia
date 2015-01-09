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
 * @file XML.hpp
 *
 * This header provides XML classes to build an XML tree as well as functions
 * to serialize that XMl tree to text. We do not support the full XML
 * specification (like described here: http://www.w3.org/TR/REC-xml/ ) but only
 * a small subset. This subset is defined by the following context-free grammar:
 *
 * NODE       := ELEMENT | string
 * ELEMENT    := START NODES END
 * NODES      := NODE NODES | epsilon
 * START      := < name ATTRIBUTES >
 * ATTRIBUTES := ATTRIBUTE ATTRIBUTES | epsilon
 * ATTRIBUTE  := key = "value"
 * END        := </ name >
 *
 * where the Axiom of a document is "Element". Note that we accept only a
 * singular root element and no primitive text at root level. Attributes are
 * key-value pairs of strings. Start and end tag name have to match.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */
#ifndef _OUSIA_XML_HPP_
#define _OUSIA_XML_HPP_

#include <map>
#include <ostream>
#include <vector>

#include <core/managed/Managed.hpp>
#include <core/managed/ManagedContainer.hpp>

namespace ousia {
namespace xml {

class Element;

/**
 * Node is the common super-class of actual elements (tag-bounded) and text.
 * It specifies the pure virtual serialize() function that the subclasses
 * implement.
 */
class Node : public Managed {
private:
	Owned<Element> parent;

public:
	Node(Manager &mgr, Handle<Element> parent)
	    : Managed(mgr), parent(acquire(parent)){};

	/**
	 * This method writes an XML prolog and the XML representing the current
	 * node, including all children, to the given output stream.
	 * @param out     is the output stream the serialized data shall be
	 *                written to.
	 * @param doctype enables you to add a prefix after the XML prolog
	 *                specifying the doctype.
	 */
	void serialize(std::ostream &out, const std::string &doctype = "");
	/**
	 * This method just writes the XML representation of this node to the
	 * output stream, without the XML prolog.
	 *
	 * @param out      the output stream the serialized data shall be written
	 *                 to.
	 * @param tabdepth the current tabdepth for prettier output.
	 */
	virtual void doSerialize(std::ostream &out, unsigned int tabdepth) = 0;

	/**
	 * @return the parent XML element of this node.
	 */
	Rooted<Element> getParent() const { return parent; }
};

/**
 * An element in XML is defined as by the W3C:
 *
 * http://www.w3.org/TR/REC-xml/#sec-starttags
 *
 * For as an element necessarily has a name. It may have key-value pairs as
 * attributes, where each key is unique (which is enforced by std::map).
 * Additionally it might have other Nodes as children.
 */
class Element : public Node {
public:
	const std::string name;
	std::map<std::string, std::string> attributes;
	ManagedVector<Node> children;

	Element(Manager &mgr, Handle<Element> parent, std::string name)
	    : Node(mgr, parent), name(std::move(name))
	{
	}

	Element(Manager &mgr, Handle<Element> parent, std::string name,
	        std::map<std::string, std::string> attributes)
	    : Node(mgr, parent),
	      name(std::move(name)),
	      attributes(std::move(attributes))
	{
	}

	/**
	 * This writes the following to the output stream:
	 * * The start tag of this element including name and attributes
	 * * The serialized data of all children as ordered by the vector.
	 * * The end tag of this element.
	 *
	 */
	void doSerialize(std::ostream &out, unsigned int tabdepth) override;
};

class Text : public Node {
public:
	const std::string text;

	Text(Manager &mgr, Handle<Element> parent, std::string text)
	    : Node(mgr, parent), text(std::move(text))
	{
	}

	/**
	 * This just writes the text to the output.
	 *
	 */
	void doSerialize(std::ostream &out, unsigned int tabdepth) override;
};
}
}
#endif
