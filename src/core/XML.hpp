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

// Forward declarations
class Rtti;

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
	 * This method writes an XML doctype and the XML representing the current
	 * node, including all children, to the given output stream.
	 * @param out     is the output stream the serialized data shall be
	 *                written to.
	 * @param doctype enables you to add a prefix specifying the doctype.
	 * @param pretty is a flag that manipulates whether newlines and tabs are
	 *               used.
	 */
	void serialize(std::ostream &out,
	               const std::string &doctype = "<?xml version=\"1.0\"?>",
	               bool pretty = true);
	/**
	 * This method just writes the XML representation of this node to the
	 * output stream.
	 *
	 * @param out      the output stream the serialized data shall be written
	 *                 to.
	 * @param tabdepth the current tabdepth for prettier output.
	 * @param pretty is a flag that manipulates whether newlines and tabs are
	 *               used.
	 */
	virtual void doSerialize(std::ostream &out, unsigned int tabdepth,
	                         bool pretty) = 0;

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
private:
	ManagedVector<Node> children;
	std::map<std::string, std::string> attributes;
	const std::string nspace;
	const std::string name;

public:
	Element(Manager &mgr, Handle<Element> parent, std::string name,
	        std::map<std::string, std::string> attributes = {},
	        std::string nspace = "")
	    : Node(mgr, parent),
	      children(this),
	      attributes(std::move(attributes)),
	      nspace(std::move(nspace)),
	      name(std::move(name))
	{
	}

	/**
	 * This writes the following to the output stream:
	 * * The start tag of this element including name and attributes
	 * * The serialized data of all children as ordered by the vector.
	 * * The end tag of this element.
	 *
	 */
	void doSerialize(std::ostream &out, unsigned int tabdepth,
	                 bool pretty) override;

	const ManagedVector<Node> &getChildren() const { return children; }

	void addChild(Handle<Node> child) { children.push_back(child); }

	void addChildren(std::vector<Handle<Node>> c)
	{
		children.insert(children.end(), c.begin(), c.end());
	}

	const std::string &getNamespace() const { return nspace; }

	const std::string &getName() const { return name; }

	const std::map<std::string, std::string> &getAttributes() const
	{
		return attributes;
	}

	std::map<std::string, std::string> &getAttributes() { return attributes; }
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
	void doSerialize(std::ostream &out, unsigned int tabdepth,
	                 bool pretty) override;
};
}

namespace RttiTypes {
extern const Rtti XMLNode;
extern const Rtti XMLElement;
extern const Rtti XMLText;
}
}
#endif