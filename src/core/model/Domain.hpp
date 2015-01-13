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
 * book and one for the actual structure. Consider the following XML:
 *
 * \code{.xml}
 * <domain name="book">
 * 	<structs>
 * 		<struct name="book" cardinality="1" isRoot="true">
 * 			<fields>
 * 				<field>
 * 					<children>
 * 						<child name="book.chapter"/>
 * 						<child name="book.paragraph"/>
 * 					</children>
 * 				</field>
 * 			</fields>
 * 		</struct>
 * 		<struct name="chapter">
 * 			<fields>
 * 				<field>
 * 					<children>
 * 						<child name="book.section"/>
 * 						<child name="book.paragraph"/>
 * 					</children>
 * 				</field>
 * 			</fields>
 * 		</struct>
 * 		<struct name="section">
 * 			<fields>
 * 				<field>
 * 					<children>
 * 						<child name="book.subsection"/>
 * 						<child name="book.paragraph"/>
 * 					</children>
 * 				</field>
 * 			</fields>
 * 		</struct>
 * 		<struct name="subsection">
 * 			<fields>
 * 				<field>
 * 					<children>
 * 						<child name="book.paragraph"/>
 * 					</children>
 * 				</field>
 * 			</fields>
 * 		</struct>
 * 		<struct name="paragraph" transparent="true" role="paragraph">
 * 			<fields>
 * 				<field>
 * 					<children>
 * 						<child name="book.text"/>
 * 					</children>
 * 				</field>
 * 			</fields>
 * 		</struct>
 * 		<struct name="text" transparent="true" role="text">
 * 			<fields>
 * 				<field name="content" type="PRIMITIVE" primitiveType="string"/>
 * 			</fields>
 * 		</struct>
 * 	</structs>
 * </domain>
 * \endcode
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
 * The translation to context free grammars is as follows:
 *
 * \code{.txt}
 * BOOK              := <book> BOOK_TREE </book>
 * BOOK_TREE         := CHAPTER BOOK_TREE | PARAGRAPH BOOK_TREE | epsilon
 * CHAPTER           := <chapter> CHAPTER_TREE </chapter>
 * CHAPTER_TREE      := SECTION CHAPTER_TREE | PARAGRAPH CHAPTER_TREE | epsilon
 * SECTION           := <section> SECTION_TREE </section>
 * SECTION_TREE      := SUBSECTION SECTION_TREE | PARAGRAPH SECTION_TREE |
 *                      epsilon
 * SUBSECTION        := <subsection> SUBSECTION_TREE </subsection>
 * SUBSECTION_TREE   := PARAGRAPH SUBSECTION_TREE | epsilon
 * PARAGRAPH         := <paragraph> PARAGRAPH_CONTENT </paragraph>
 * PARAGRAPH_CONTENT := string
 * \endcode
 *
 * Note that this translation recurs to further nonterminals like SECTION but
 * necessarily produces one "book" terminal. Also note that, in principle,
 * this grammar translation allows for arbitrarily many children instances of
 * the proper StructuredClass. This can be regulated by the "cardinality"
 * property of a StructuredClass.
 *
 * It is possible to add further fields, like we would in the "headings" domain
 * to add titles to our structure.
 *
 * \code{.xml}
 * <domain name="headings">
 * 	<head>
 * 		<import rel="domain" src="book.oxm"/>
 * 	</head>
 * 	<structs>
 * 		<struct name="heading" cardinality="0-1" transparent="true">
 * 			<parents>
 * 				<parent name="book.book">
 * 					<field name="heading" type="SUBTREE"/>
 * 				</parent>
 * 				...
 * 			</parents>
 * 			<fields>
 * 				<fieldRef name="book.paragraph.">
 * 			</fields>
 * 	</structs>
 * </domain>
 * \endcode
 *
 * This would change the context free grammar as follows:
 *
 * \code{.txt}
 * BOOK              := <book> HEADING BOOK_TREE </book>
 * HEADING           := <heading> PARAGRAPH </heading>
 * \endcode
 *
 * AnnotationClasses on the other hand do not specify a context free grammar.
 * They merely specify what kinds of Annotations are allowed within this domain
 * and which fields or attributes they have. Note that Annotations are allowed
 * to define structured children that manifest e.g. meta information of that
 * Annotation. An example for that would be the "comment" domain:
 *
 * \code{.xml}
 * <domain name="comments">
 * 	<head>
 * 		<import rel="domain" src="book.oxm"/>
 * 	</head>
 * 	<annos>
 * 		<anno name="comment">
 * 			<fields>
 * 				<field name="replies" type="SUBTREE">
 * 					<children>
 * 						<child name="reply"/>
 * 					</children>
 * 				</field>
 * 			</fields>
 * 		</anno>
 * 	</annos>
 * 	<structs>
 * 		<struct name="reply">
 * 			<fields>
 * 				<field name="replies" type="SUBTREE">
 * 					<children>
 * 						<child name="reply"/>
 * 					</children>
 * 				</field>
 * 				<field name="content" type="SUBTREE">
 * 					<children>
 * 						<child name="book.paragraph"/>
 * 					</children>
 * 				</field>
 * 			</fields>
 * 		</struct>
 * 	</structs>
 * </domain>
 * \endcode
 *
 * Here we have comment annotations, which have a reply tree as sub structure.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_DOMAIN_HPP_
#define _OUSIA_MODEL_DOMAIN_HPP_

#include <core/managed/ManagedContainer.hpp>
#include <core/RangeSet.hpp>

#include "Node.hpp"
#include "Typesystem.hpp"

namespace ousia {

// Forward declarations
class RttiType;
template <class T>
class Rtti;

namespace model {

// Forward declarations
class Descriptor;
class StructuredClass;
class Domain;

/**
 * As mentioned in the description above a FieldDescriptor specifies the
 * StructuredClasses that are allowed as children of a StructuredClass or
 * AnnotationClass. A field may also be primitive, which means that a proper
 * instance of the respective StructuredClass or AnnotationClass must provide
 * accordingly typed content without further descending in the Structure
 * Hierarchy.
 *
 * As an example consider the "paragraph" StructuredClass, which might allow
 * the actual text content. Here is the according XML:
 *
 * 		<struct name="paragraph" transparent="true" role="paragraph">
 * 			<fields>
 * 				<field>
 * 					<children>
 * 						<child name="book.text"/>
 * 					</children>
 * 				</field>
 * 			</fields>
 * 		</struct>
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
	NodeVector<StructuredClass> children;
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
	 * @param parent        is a handle of the Descriptor node that has this
	 *                      FieldDescriptor.
	 * @param primitiveType is a handle to some Type in some Typesystem of which
	 *                      one instance is allowed to fill this field.
	 * @param name          is the name of this field.
	 * @param optional      should be set to 'false' is this field needs to be
	 *                      filled in order for an instance of the parent
	 *                      Descriptor to be valid.
	 */
	FieldDescriptor(Manager &mgr, Handle<Descriptor> parent,
	                Handle<Type> primitiveType, std::string name = "",
	                bool optional = false)
	    : Node(mgr, std::move(name), parent),
	      children(this),
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
	 * @param parent        is a handle of the Descriptor node that has this
	 *                      FieldDescriptor.
	 * @param fieldType     is the FieldType of this FieldDescriptor, either
	 *                      TREE for the main or default structure or SUBTREE
	 *                      for supporting structures.
	 * @param name          is the name of this field.
	 * @param optional      should be set to 'false' is this field needs to be
	 *                      filled in order for an instance of the parent
	 *                      Descriptor to be valid.
	 */
	FieldDescriptor(Manager &mgr, Handle<Descriptor> parent,
	                FieldType fieldType = FieldType::TREE,
	                std::string name = "", bool optional = false)
	    : Node(mgr, std::move(name), parent),
	      children(this),
	      fieldType(fieldType),
	      optional(optional)
	{
	}

	/**
	 * Returns a const reference to the NodeVector of StructuredClasses whose
	 * instances are allowed as children in the StructureTree of instances of
	 * this field.
	 *
	 * @return a const reference to the NodeVector of StructuredClasses whose
	 * instances are allowed as children in the StructureTree of instances of
	 * this field.
	 */
	const NodeVector<StructuredClass> &getChildren() const { return children; }

	/**
	 * Adds a StructuredClass whose instances shall be allowed as children in
	 * the StructureTree of instances of this field.
	 */
	void addChild(Handle<StructuredClass> c) { children.push_back(c); }

	/**
	 * Adds multiple StructuredClasses whose instances shall be allowed as
	 * children in the StructureTree of instances of this field.
	 */
	void addChildren(const std::vector<Handle<StructuredClass>> &cs)
	{
		children.insert(children.end(), cs.begin(), cs.end());
	}

	FieldType getFieldType() const { return fieldType; }

	bool isPrimitive() const { return fieldType == FieldType::PRIMITIVE; }

	Rooted<Type> getPrimitiveType() const { return primitiveType; }
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
 * TODO: What about optional attributes?
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
	NodeVector<FieldDescriptor> fieldDescriptors;

	bool continuePath(Handle<StructuredClass> target,
	                  std::vector<Rooted<Node>> &path) const;

protected:
	void continueResolve(ResolutionState &state) override;

public:
	Descriptor(Manager &mgr, std::string name, Handle<Domain> domain,
	           // TODO: What would be a wise default value for attributes?
	           Handle<StructType> attributesDescriptor)
	    : Node(mgr, std::move(name), domain),
	      attributesDescriptor(acquire(attributesDescriptor)),
	      fieldDescriptors(this)
	{
	}

	/**
	 * Returns a reference to the StructType that specifies the attribute keys
	 * as well as value domains for this Descriptor.
	 *
	 * @return a reference to the StructType that specifies the attribute keys
	 *         as well as value domains for this Descriptor.
	 */
	Rooted<StructType> getAttributesDescriptor() const
	{
		return attributesDescriptor;
	}
	/**
	 * Returns a const reference to the NodeVector of all FieldDescriptors of
	 * this Descriptor.
	 *
	 * @return a const reference to the NodeVector of all FieldDescriptors of
	 * this Descriptor.
	 */
	const NodeVector<FieldDescriptor> &getFieldDescriptors() const
	{
		return fieldDescriptors;
	}

	/**
	 * Adds a FieldDescriptor to this Descriptor.
	 */
	void addFieldDescriptor(Handle<FieldDescriptor> fd)
	{
		fieldDescriptors.push_back(fd);
	}

	/**
	 * Adds multiple FieldDescriptors to this Descriptor.
	 */
	void addFieldDescriptors(const std::vector<Handle<FieldDescriptor>> &fds)
	{
		fieldDescriptors.insert(fieldDescriptors.end(), fds.begin(), fds.end());
	}

	/**
	 * This tries to construct the shortest possible path of this Descriptor
	 * to the given child Descriptor. As an example consider the book domain
	 * from above.
	 *
	 * First consider the call book->pathTo(chapter). This is an easy example:
	 * Our path just contains a reference to the default field of book, because
	 * a chapter may be directly added to the main field of book.
	 *
	 * Second consider the call book->pathTo(text). This is somewhat more
	 * complicated, but it is still a valid request, because we can construct
	 * the path: {book_main_field, paragraph, paragraph_main_field}.
	 * This is only valid because paragraph is transparent.
	 *
	 * What about the call book->pathTo(section)? This will lead to an empty
	 * return path (= invalid). We could, of course, in principle construct
	 * a path between book and section (via chapter), but chapter is not
	 * transparent. Therefore that path is not allowed.
	 *
	 * @param childDescriptor is a supposedly valid child Descriptor of this
	 *                        Descriptor.
	 * @return                either a path of FieldDescriptors and
	 *                        StructuredClasses between this Descriptor and
	 *                        the input StructuredClass or an empty vector if
	 *                        no such path can be constructed.
	 *
	 */
	std::vector<Rooted<Node>> pathTo(
	    Handle<StructuredClass> childDescriptor) const;
};

typedef RangeSet<size_t> Cardinality;

/**
 * A StructuredClass specifies nodes in the StructureTree of a document that
 * implements this domain. For more information on the StructureTree please
 * consult the Header documentation above.
 *
 * Note that a StructuredClass may "invade" an existing Domain description by
 * defining itself as a viable child in one existing field. Consider the
 * example of the "heading" domain from the header documentation again:
 *
 * <domain name="headings">
 * 	<head>
 * 		<import rel="domain" src="book.oxm"/>
 * 	</head>
 * 	<structs>
 * 		<struct name="heading" cardinality="0-1" transparent="true">
 * 			<parents>
 * 				<parent name="book.book">
 * 					<field name="heading" type="SUBTREE"/>
 * 				</parent>
 * 				...
 * 			</parents>
 * 			<fields>
 * 				<fieldRef name="book.paragraph.">
 * 			</fields>
 * 	</structs>
 * </domain>
 *
 * The "parent" construct allows to "invade" another domain.
 *
 * This does indeed interfere with an existing domain and one must carefully
 * craft such parent references to not create undesired side effects. However
 * they provide the most convenient mechanism to extend existing domains
 * without having to rewrite them.
 *
 * Another important factor is the 'transparent' flag. Transparent
 * StructureClasses may be implicitly constructed in the document graph.
 * If we go back to our example a user would (without transparency) have to
 * explicitly declare:
 *
 * <book>
 *   <section>
 *     <paragraph>Text.</paragraph>
 *   </section>
 * </book>
 *
 * But in our mind the document

 * <book>
 *   <section>
 *     Text.
 *   </section>
 * </book>
 *
 * Is already sufficiently specific. We can infer that a paragraph should be
 * wrapped around "Text.". Therefore we set the 'transparent' flag of the
 * "paragraph" StructuredClass to true. Please note that such inferences
 * become increasingly complicated when children of transparent
 * StructuredClasses are allowed to be transparent as well. So use with care.
 *
 * Finally we allow StructuredClasses to inherit attributes of other
 * StructuredClasses. Inheritance also implies that instance of the inheriting
 * class can be used wherever an instance of the inherited class is allowed.
 * Inheritance therefore also goes for fields. TODO: What is the specification
 * for field inheritance? Is the child allowed to specify children at all?
 * Is that interpreted as overriding the parent fields or constructing a union?
 * What about the cardinality?
 */
class StructuredClass : public Descriptor {
private:
	const Cardinality cardinality;
	Owned<StructuredClass> isa;
	NodeVector<FieldDescriptor> parents;

public:
	const bool transparent;
	// TODO: Is it possible to have root=true and cardinality other than 1?
	// This also refers to the question in Document.hpp: Is it possible to have
	// more than 1 root?
	const bool root;

	/**
	 * The constructor for a StructuredClass.
	 *
	 * @param mgr                  is the current Manager.
	 * @param name                 is the name of the StructuredClass.
	 * @param domain               is the Domain this StructuredClass belongs
	 *                             to.
	 * @param cardinality          specifies how often an element of this type
	 *                             may occur at a specific point in the
	 *                             StructureTree. For example: A document should
	 *                             have at least one author.
	 * @param attributesDescriptor is a StructType that specifies the attribute
	 *                             keys as well as value domains for this
	 *                             Descriptor.
	 * @param isa                  references a parent StructuredClass. Please
	 *                             look for more information on inheritance in
	 *                             the class documentation above. The default is
	 *                             a null reference, meaning no parent class.
	 * @param transparent          specifies whether this StructuredClass is
	 *                             transparent. For more information on
	 *                             transparency please refer to the class
	 *                             documentation above. The default is false.
	 */
	StructuredClass(Manager &mgr, std::string name, Handle<Domain> domain,
	                const Cardinality &cardinality,
	                Handle<StructType> attributesDescriptor = nullptr,
	                // TODO: What would be a wise default value for isa?
	                Handle<StructuredClass> isa = nullptr,
	                bool transparent = false, bool root = false)
	    : Descriptor(mgr, std::move(name), domain, attributesDescriptor),
	      cardinality(cardinality),
	      isa(acquire(isa)),
	      parents(this),
	      transparent(transparent),
	      root(root)
	{
	}

	/**
	 * Returns the Cardinality of this StructuredClass (as a RangeSet).
	 *
	 * @return the Cardinality of this StructuredClass (as a RangeSet).
	 */
	const Cardinality &getCardinality() const { return cardinality; }

	/**
	 * Returns the parent of this StructuredClass in the class inheritance
	 * hierarchy (!). This is not the same as the parents in the Structure Tree!
	 *
	 * @return the parent of this StructuredClass in the class inheritance
	 * hierarchy (!).
	 */
	Rooted<StructuredClass> getIsA() const { return isa; }

	/**
	 * Returns a const reference to the NodeVector of FieldDescriptors that
	 * should allow an instance of this StructuredClass as child in the
	 * Structure Tree. This enables you to "invade" other domains as described
	 * in the StructuredClass documentation.
	 *
	 * @return a const reference to the NodeVector of FieldDescriptors that
	 * should allow an instance of this StructuredClass as child in the
	 * Structure Tree.
	 */
	const NodeVector<FieldDescriptor> &getParents() const { return parents; }

	/**
	 * Adds a FieldDescriptor that should allow an instance of this
	 * StructuredClass as a child in the Structure Tree.
	 */
	void addParent(Handle<FieldDescriptor> p) { parents.push_back(p); }

	/**
	 * Adds multiple FieldDescriptors that should allow an instance of this
	 * StructuredClass as a child in the Structure Tree.
	 */
	void addParents(const std::vector<Handle<FieldDescriptor>> &ps)
	{
		parents.insert(parents.end(), ps.begin(), ps.end());
	}
};

/**
 * An AnnotationClass defines allowed Annotations. For more information on
 * Annotations please refer to the Document.hpp.
 *
 * This class has no special properties and is in essence just a Descriptor.
 */
class AnnotationClass : public Descriptor {
public:
	/**
	 * The constructor for a new AnnotationClass. Note that you have to add
	 * the FieldDescriptors to it later on.
	 *
	 * @param mgr                  is the Manager instance.
	 * @param name                 is a name for this AnnotationClass that will
	 *                             be used for later references to this
	 *                             AnnotationClass.
	 * @param domain               is the Domain this AnnotationClass belongs
	 *to.
	 * @param attributesDescriptor is a StructType that specifies the attribute
	 *                             keys as well as value domains for this
	 *                             Descriptor.
	 */
	AnnotationClass(Manager &mgr, std::string name, Handle<Domain> domain,
	                // TODO: What would be a wise default value for attributes?
	                Handle<StructType> attributesDescriptor)
	    : Descriptor(mgr, std::move(name), domain, attributesDescriptor)
	{
	}
};

/**
 * A Domain node specifies which StructuredClasses and which AnnotationClasses
 * are part of this domain. TODO: Do we want to be able to restrict Annotations
 * to certain Structures?
 */
class Domain : public Node {
private:
	NodeVector<StructuredClass> structuredClasses;
	NodeVector<AnnotationClass> annotationClasses;
	// TODO: Is it wise to attach the type systems here? If not: What would be
	// a good alternative.
	NodeVector<Typesystem> typesystems;

protected:
	void continueResolve(ResolutionState &state) override;

public:
	/**
	 * The constructor for a new domain. Note that this is an empty Domain and
	 * still has to be filled with StructuredClasses and AnnotationClasses.
	 *
	 * @param mgr  is the Manager instance.
	 * @param sys  is the SystemTypesystem instance.
	 * @param name is a name for this domain which will be used for later
	 *             references to this Domain.
	 */
	Domain(Manager &mgr, Handle<SystemTypesystem> sys, std::string name)
	    // TODO: Can a domain have a parent?
	    : Node(mgr, std::move(name), nullptr),
	      structuredClasses(this),
	      annotationClasses(this),
	      typesystems(this, std::vector<Handle<Typesystem>>{sys})
	{
	}

	/**
	 * Returns a const reference to the NodeVector of StructuredClasses that are
	 * part of this Domain.
	 *
	 * @return a const reference to the NodeVector of StructuredClasses that are
	 * part of this Domain.
	 */
	const NodeVector<StructuredClass> &getStructureClasses() const
	{
		return structuredClasses;
	}

	/**
	 * Adds a StructuredClass to this Domain.
	 */
	void addStructuredClass(Handle<StructuredClass> s)
	{
		structuredClasses.push_back(s);
	}

	/**
	 * Adds multiple StructuredClasses to this Domain.
	 */
	void addStructuredClasses(const std::vector<Handle<StructuredClass>> &ss)
	{
		structuredClasses.insert(structuredClasses.end(), ss.begin(), ss.end());
	}

	/**
	 * Returns a const reference to the NodeVector of AnnotationClasses that are
	 * part of this Domain.
	 *
	 * @return a const reference to the NodeVector of AnnotationClasses that are
	 * part of this Domain.
	 */
	const NodeVector<AnnotationClass> &getAnnotationClasses() const
	{
		return annotationClasses;
	}

	/**
	 * Adds an AnnotationClass to this Domain.
	 */
	void addAnnotationClass(Handle<AnnotationClass> a)
	{
		annotationClasses.push_back(a);
	}

	/**
	 * Adds multiple AnnotationClasses to this Domain.
	 */
	void addAnnotationClasses(const std::vector<Handle<AnnotationClass>> &as)
	{
		annotationClasses.insert(annotationClasses.end(), as.begin(), as.end());
	}

	/**
	 * Returns a const reference to the NodeVector of TypeSystems that are
	 * references in this Domain.
	 *
	 * @return a const reference to the NodeVector of TypeSystems that are
	 * references in this Domain.
	 */
	const NodeVector<Typesystem> &getTypesystems() const { return typesystems; }

	/**
	 * Adds a Typesystem reference to this Domain.
	 */
	void addTypesystem(Handle<Typesystem> t) { typesystems.push_back(t); }

	/**
	 * Adds multiple Typesystem references to this Domain.
	 */
	void addTypesystems(const std::vector<Handle<Typesystem>> &ts)
	{
		typesystems.insert(typesystems.end(), ts.begin(), ts.end());
	}
};
}

namespace RttiTypes {

extern const Rtti<model::FieldDescriptor> FieldDescriptor;
extern const Rtti<model::Descriptor> Descriptor;
extern const Rtti<model::StructuredClass> StructuredClass;
extern const Rtti<model::AnnotationClass> AnnotationClass;
extern const Rtti<model::Domain> Domain;
}
}

#endif /* _OUSIA_MODEL_DOMAIN_HPP_ */

