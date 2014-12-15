/*
    Ousía
    Copyright (C) 2014, 2015  Benjamin Paaßen, Andreas Stöckel

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
 * @file Domain.hpp
 *
 * This header contains the class hierarchy of descriptor classes for domains.
 * Properly connected instances of these classes with a Domain node as root
 * describe a semantic Domain in a formal way. It specifies the allowed (tree)
 * structure of a document by means of StructuredClasses as well as the allowed
 * Annotations by means of AnnotationClasses.
 *
 * The Structure Description contained in the hierarchy of StructuredClasses is
 * equivalent to a context free grammar of a special form. We introduce the
 * terms "StructuredClass" and "FieldDescriptor".
 * On the top level you would start with a StructuredClass, say "book", which
 * in turn might contain two FieldDescriptors, one for the meta data of ones
 * book and one for the actual structure. Consider the following (simplified)
 * XML notation (TODO: Use a non-simplified notation as soon as the format is
 * clear.)
 *
 * <StructuredClass name="book">
 *   <FieldDescriptor name="structure", type="TREE", optional="false">
 *     <children>
 *       Here we would reference the possible child classes, e.g. section,
 *       paragraph, etc.
 *     </children>
 *   </FieldDescriptor>
 *   <FieldDescriptor name="meta", type="SUBTREE", optional="true">
 *     <children>
 *       Here we would reference the possible child classes for meta,
 *       information, e.g. authors, date, version, etc.
 *     </children>
 *   </FieldDescriptor>
 * </StructuredClass>
 *
 * Note that we define one field as the TREE (meaning the main or default
 * document structure) and one mearly as SUBTREE, relating to supporting
 * information. You are not allowed to define more than one field of type
 * "TREE". Accordingly for each StructuredClass in the main TREE there must be
 * at least one possible primitive child or one TREE field. Otherwise the
 * grammar would be nonterminal. For SUBTREE fields no children may define a
 * TREE field and at least one permitted child must exist, either primitive or
 * as another StructuredClass.
 *
 * The translation to context free grammars is roughly as follows:
 *
 * BOOK           := book BOOK_STRUCTURE BOOK_META
 * BOOK_STRUCTURE := SECTION BOOK_STRUCTURE | PARAGRAPH BOOK_STRUCTURE | epsilon
 * BOOK_META      := AUTHOR BOOK_META | DATE BOOK_META
 *
 * Note that this translation recurs to further nonterminals like SECTION but
 * necessarily produces one "book" terminal. Also note that, in principle,
 * this grammar translation allows for arbitrarily many children instances of
 * the proper StructuredClass. This can be regulated by the "cardinality"
 * property of a StructuredClass.
 *
 * AnnotationClasses on the other hand do not specify a context free grammar.
 * They merely specify what kinds of Annotations are allowed within this domain
 * and which fields or attributes they have. Note that Annotations are allowed
 * to define structured children that manifest e.g. meta information of that
 * Annotation.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_DOMAIN_HPP_
#define _OUSIA_MODEL_DOMAIN_HPP_

#include <core/ManagedContainers.hpp>
#include <core/Node.hpp>

namespace ousia {
namespace model {

class StructuredClass;
class Descriptor;

/**
 * As mentioned in the description above a FieldDescriptor specifies the
 * StructuredClasses that are allowed as children of a StructuredClass or
 * AnnotationClass. A field may also be primitive, which means that a proper
 * instance of the respective StructuredClass or AnnotationClass must provide
 * accordingly typed content without further descending in the Structure
 * Hierarchy.
 *
 * As an example consider the "paragraph" StructuredClass, which might allow
 * the actual text content. Here is the according simplified XML (TODO: replace
 * with a non-simplified version as soon as the XML syntax is clear.)
 *
 * <StructuredClass name="paragraph">
 *   <FieldDescriptor name="text", type="PRIMITIVE", optional="false",
 *                    primitiveType="string"/>
 * </StructuredClass>
 *
 * Accordingly the primitiveType field of a FieldDescriptor may only be
 * defined if the type is set to "PRIMITIVE". If the type is something else
 * at least one child must be defined and the primitiveType remains in an
 * undefined state.
 */
class FieldDescriptor : public Node {
public:
	/**
	 * This enum class contains all possible FieldTypes, meaning either the
	 * main structure beneath this Descritor (TREE), supporting structure
	 * (SUBTREE) or a primitive terminal (PRIMITIVE).
	 *
	 * Note the following rules (which are also mentioned above):
	 * 1.) There may be only one TREE field in a Descriptor.
	 * 2.) Each TREE field must allow for at least one child, which in turn has
	 *     either a TREE field or a PRIMITIVE field.
	 * 3.) SUBTREE fields may not allow for children with TREE fields.
	 * 4.) SUBTREE fields must allow for at least one child with another SUBTREE
	 *     or PRIMITIVE field.
	 */
	enum class FieldType { TREE, SUBTREE, PRIMITIVE };

private:
	ManagedVector<StructuredClass> children;
	FieldType fieldType;
	Owned<Type> primitiveType;

public:
	const bool optional;

	// TODO: What about the name of default fields?
	/**
	 * This is the constructor for primitive fields. The type is automatically
	 * set to "PRIMITIVE".
	 *
	 * @param mgr           is the global Manager instance.
	 * @param name          is the name of this field.
	 * @param parent        is a handle of the Descriptor node that has this
	 *                      FieldDescriptor.
	 * @param primitiveType is a handle to some Type in some Typesystem of which
	 *                      one instance is allowed to fill this field.
	 * @param optional      should be set to 'false' is this field needs to be
	 *                      filled in order for an instance of the parent
	 *                      Descriptor to be valid.
	 */
	FieldDescriptor(Manager &mgr, std::string name, Handle<Descriptor> parent,
	                Handle<Type> primitiveType, bool optional)
	    : Node(mgr, std::move(name), parent),
	      fieldType(FieldType::PRIMITIVE),
	      primitiveType(acquire(primitiveType)),
	      optional(optional)
	{
	}

	/**
	 * This is the constructor for non-primitive fields. You have to provide
	 * children here.
	 *
	 * @param mgr           is the global Manager instance.
	 * @param name          is the name of this field.
	 * @param parent        is a handle of the Descriptor node that has this
	 *                      FieldDescriptor.
	 * @param type          is the FieldType of this FieldDescriptor, either
	 *                      TREE for the main or default structure or SUBTREE
	 *                      for supporting structures.
	 * @param optional      should be set to 'false' is this field needs to be
	 *                      filled in order for an instance of the parent
	 *                      Descriptor to be valid.
	 */
	FieldDescriptor(Manager &mgr, std::string name, Handle<Descriptor> parent,
	                FieldType type, ManagedVector<StructuredClass> children,
	                bool optional)
	    : Node(mgr, std::move(name), parent),
	      fieldType(type),
	      children(children),
	      optional(optional)
	// TODO: What would be a wise initialization of the primitiveType?
	{
	}

	// TODO: Is returning a ManagedVector alright?
	ManagedVector<StructuredClass> &getChildren() { return children; }

	FieldType getFieldType() { return type; }

	bool isPrimitive() { return type == FieldType::PRIMITIVE; }

	Rooted<Type> getPrimitiveType() { return primitiveType; }
};

/**
 * This is a super class for StructuredClasses and AnnotationClasses and is,
 * in itself, not supposed to be instantiated. It defines that both, Annotations
 * and StructuredEntities, may have attributes and fields. For more information
 * on fields please have a look at the header documentation as well as the
 * documentation of the FieldDescriptor class.
 *
 * Attributes are primitive content stored in a key-value fashion. Therefore
 * the attribute specification of a descriptor is done by referencing an
 * appropriate StructType that contains all permitted keys and value types.
 *
 * TODO: What aout optional attributes?
 *
 * In XML terms the difference between primitive fields and attributes can be
 * explained as the difference between node attributes and node children.
 * Consider the XML
 *
 * <A key="value">
 *   <key>value</key>
 * </A>
 *
 * key="value" inside the A-node would be an attribute, while <key>value</key>
 * would be a primitive field. While equivalent in XML the semantics are
 * different: An attribute describes indeed attributes, features of one single
 * node whereas a primitive field describes the _content_ of a node.
 *
 */
class Descriptor : public Node {
private:
	Owned<StructType> attributesDescriptor;
	ManagedVector<FieldDescriptor> fieldDescriptors;

public:
	Descriptor(Manager &mgr, std::string name, Handle<Node> parent,
	           // TODO: What would be a wise default value for attributes?
	           Handle<StructType> attributesDescriptor,
	           ManagedVector<FieldDescriptor> fieldDescriptors)
	    : Node(mgr, std::move(name), parent),
	      attributesDescriptor(attributesDescriptor),
	      fieldDescriptors(fieldDescriptors)
	{
	}

	Rooted<StructType> getAttributesDescriptor()
	{
		return attributesDescriptor;
	}

	// TODO: Is returning a ManagedVector alright?
	ManagedVector<FieldDescriptor> getFieldDescriptors()
	{
		return fieldDescriptors;
	}
};
}
}

#endif /* _OUSIA_MODEL_DOMAIN_HPP_ */

