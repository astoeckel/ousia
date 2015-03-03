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
 * @file Ontology.hpp
 *
 * This header contains the class hierarchy of descriptor classes for
 * ontologies.
 * Properly connected instances of these classes with a Ontology node as root
 * describe a semantic Ontology in a formal way. It specifies the allowed (tree)
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
 * <ontology name="book">
 * 	<struct name="book" cardinality="1" isRoot="true">
 * 		<field>
 * 			<childRef ref="book.chapter"/>
 * 			<childRef ref="book.paragraph"/>
 * 		</field>
 * 	</struct>
 * 	<struct name="chapter">
 * 		<field>
 * 			<childRef ref="book.section"/>
 * 			<childRef ref="book.paragraph"/>
 * 		</field>
 * 	</struct>
 * 	<struct name="section">
 * 		<field>
 * 			<childRef ref="book.subsection"/>
 * 			<childRef ref="book.paragraph"/>
 * 		</field>
 * 	</struct>
 * 	<struct name="subsection">
 * 		<field>
 * 			<childRef ref="book.paragraph"/>
 * 		</field>
 * 	</struct>
 * 	<struct name="paragraph" transparent="true">
 * 		<field>
 * 			<childRef ref="book.text"/>
 * 		</field>
 * 	</struct>
 * 	<struct name="text" transparent="true">
 * 		<primitive type="string"/>
 * 	</struct>
 * </ontology>
 * \endcode
 *
 * Note that we define one field as the TREE (meaning the main or default
 * document structure) and one mearly as SUBTREE, relating to supporting
 * information. You are not allowed to define more than one field of type
 * "TREE".
 *
 * The translation to a context free grammar is as follows:
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
 * It is possible to add further fields, like we would in the "headings"
 * ontology to add titles to our structure.
 *
 * \code{.xml}
 * <ontology name="headings">
 *	<import rel="ontology" src="./book_ontology.osxml"/>
 * 	<struct name="heading" cardinality="1" transparent="true">
 * 		<parentRef ref="book.book">
 * 			<field name="heading" isSubtree="true" optional="true"/>
 * 		</parentRef>
 * 		...
 * 		<fieldRef name="book.paragraph.">
 * 	</struct>
 * </ontology>
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
 * They merely specify what kinds of Annotations are allowed within this
 * ontology and which fields or attributes they have. Note that Annotations are
 * allowed to define structured children that manifest e.g. meta information of
 * that Annotation. An example for that would be the "comment" ontology:
 *
 * \code{.xml}
 * <ontology name="comments">
 *	<import rel="ontology" src="./book_ontology.osxml"/>
 *
 *	<annotation name="comment">
 *		<field name="content" isSubtree="true">
 *			<childRef ref="book.paragraph"/>
 *		</field>
 *		<field name="replies" isSubtree="true">
 *			<childRef ref="reply"/>
 *		</field>
 *	</annotation>
 *
 *	<struct name="comment">
 *		<field name="content">
 *			<childRef ref="book.paragraph"/>
 *		</field>
 *		<field name="replies" isSubtree="true">
 *			<childRef ref="reply"/>
 *		</field>
 *		<parentRef ref="book.paragraph">
 *			<fieldRef ref="$default"/>
 *		</parentRef>
 *	</struct>
 *	<struct name="reply">
 *		<field name="content" isSubtree="true">
 *			<childRef ref="book.paragraph"/>
 *		</field>
 *		<field name="replies" isSubtree="true">
 *			<childRef ref="reply"/>
 *		</field>
 *	</struct>
 * </ontology>
 * \endcode
 *
 * Here we have comment annotations, which have a reply tree as sub structure.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_ONTOLOGY_HPP_
#define _OUSIA_MODEL_ONTOLOGY_HPP_

#include <core/common/Whitespace.hpp>
#include <core/managed/ManagedContainer.hpp>
#include <core/RangeSet.hpp>

#include "Node.hpp"
#include "RootNode.hpp"
#include "Syntax.hpp"
#include "Typesystem.hpp"

namespace ousia {

// Forward declarations
class Rtti;
class Descriptor;
class StructuredClass;
class Ontology;

/**
 * Magic field name used to identify the default field. The default field is
 * either the tree field or the only subtree field.
 */
static const std::string DEFAULT_FIELD_NAME = "$default";

/**
 * As mentioned in the description above a FieldDescriptor specifies the
 * StructuredClasses that are allowed as children of a StructuredClass or
 * AnnotationClass. A field may also be primitive, which means that a proper
 * instance of the respective StructuredClass or AnnotationClass must provide
 * accordingly typed content without further descending in the Structure
 * Hierarchy.
 *
 * As an example consider the "text" StructuredClass, which might allow
 * the actual text content. Here is the according XML:
 *
 * \code{.xml}
 * 	<struct name="text" transparent="true">
 * 		<primitive type="string"/>
 * 	</struct>
 * \endcode
 *
 */
class FieldDescriptor : public Node {
	friend Descriptor;

public:
	/**
	 * This enum class contains all possible FieldTypes, meaning either the
	 * main structure beneath this Descriptor (TREE) or supporting structure
	 * (SUBTREE)
	 *
	 * Note that there may be only one TREE field in a descriptor.
	 */
	enum class FieldType { TREE, SUBTREE };

private:
	NodeVector<StructuredClass> children;
	FieldType fieldType;
	Owned<Type> primitiveType;
	bool optional;
	bool primitive;
	TokenDescriptor startToken;
	TokenDescriptor endToken;
	WhitespaceMode whitespaceMode;

protected:
	bool doValidate(Logger &logger) const override;

public:
	/**
	 * This is the constructor for primitive fields.
	 *
	 * @param mgr            is the global Manager instance.
	 * @param parent         is a handle of the Descriptor node that has this
	 *                       FieldDescriptor.
	 * @param primitiveType  is a handle to some Type in some Typesystem of
	 *which
	 *                       one instance is allowed to fill this field.
	 * @param name           is the name of this field.
	 * @param optional       should be set to 'false' is this field needs to be
	 *                       filled in order for an instance of the parent
	 *                       Descriptor to be valid.
	 * @param whitespaceMode the WhitespaceMode to be used when an instance of
	 *                       this FieldDescriptor is parsed.
	 */
	FieldDescriptor(Manager &mgr, Handle<Type> primitiveType,
	                Handle<Descriptor> parent,
	                FieldType fieldType = FieldType::TREE,
	                std::string name = "", bool optional = false,
	                WhitespaceMode whitespaceMode = WhitespaceMode::COLLAPSE);

	/**
	 * This is the constructor for non-primitive fields. You have to provide
	 * children here later on.
	 *
	 * @param mgr            is the global Manager instance.
	 * @param parent         is a handle of the Descriptor node that has this
	 *                       FieldDescriptor.
	 * @param fieldType      is the FieldType of this FieldDescriptor, either
	 *                       TREE for the main or default structure or SUBTREE
	 *                       for supporting structures.
	 * @param name           is the name of this field.
	 * @param optional       should be set to 'false' is this field needs to be
	 *                       filled in order for an instance of the parent
	 *                       Descriptor to be valid.
	 * @param whitespaceMode the WhitespaceMode to be used when an instance of
	 *                       this FieldDescriptor is parsed.
	 */
	FieldDescriptor(Manager &mgr, Handle<Descriptor> parent = nullptr,
	                FieldType fieldType = FieldType::TREE,
	                std::string name = "", bool optional = false,
	                WhitespaceMode whitespaceMode = WhitespaceMode::COLLAPSE);

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
	 * Returns all StructuredClasses whose instances are allowed as children in
	 * the Structure Tree of instances of this field including subclasses of
	 * children, which are allowed directly.
	 *
	 * @return all StructuredClasses whose instances are allowed as children in
	 * the Structure Tree of instances of this field including subclasses of
	 * children, which are allowed directly.
	 */
	NodeVector<StructuredClass> getChildrenWithSubclasses() const;

	/**
	 * Adds a StructuredClass whose instances shall be allowed as children in
	 * the StructureTree of instances of this field.
	 */
	void addChild(Handle<StructuredClass> c)
	{
		invalidate();
		children.push_back(c);
	}

	/**
	 * Adds multiple StructuredClasses whose instances shall be allowed as
	 * children in the StructureTree of instances of this field.
	 */
	void addChildren(const std::vector<Handle<StructuredClass>> &cs)
	{
		invalidate();
		children.insert(children.end(), cs.begin(), cs.end());
	}

	/**
	 * Removes the given StructuredClass from the list of children of this
	 * FieldDescriptor.
	 *
	 * @param c some StructuredClass that shan't be a child of this
	 *          FieldDescriptor anymore.
	 * @return  true if the FieldDescriptor contained this child and false if it
	 *          did not.
	 */
	bool removeChild(Handle<StructuredClass> c);

	/**
	 * Returns the type of this field (not to be confused with the primitive
	 * type of this field).
	 *
	 * @return the type of this field.
	 */
	FieldType getFieldType() const { return fieldType; }
	/**
	 * Sets the type of this field (not to be confused with the primitive type
	 * of this field).
	 *
	 * @param ft is the new type of this field.
	 */
	void setFieldType(const FieldType &ft)
	{
		invalidate();
		fieldType = ft;
	}

	/**
	 * Returns if this field is primitive.
	 *
	 * @return true if and only if this field is primitive.
	 */
	bool isPrimitive() const { return primitive; }

	/**
	 * Returns the primitive type of this field, which is only allowed to be
	 * set if the type of this field is PRIMITIVE.
	 *
	 * @return the primitive type of this field.
	 */
	Rooted<Type> getPrimitiveType() const { return primitiveType; }

	/**
	 * Sets the primitive type of this field, which is only allowed to be
	 * set if the type of this field is PRIMITIVE.
	 *
	 * @param t is the new primitive type of this field-
	 */
	void setPrimitiveType(Handle<Type> t)
	{
		invalidate();
		primitiveType = acquire(t);
	}

	/**
	 * Returns true if and only if this field is optional.
	 *
	 * @return true if and only if this field is optional.
	 */
	bool isOptional() const { return optional; }

	/**
	 * Specifies whether this field shall be optional.
	 *
	 * @param o should be true if and only if this field should be optional.
	 */
	void setOptional(bool o)
	{
		invalidate();
		optional = std::move(o);
	}

	/**
	 * This tries to construct the shortest possible path of this Descriptor
	 * to the given child Descriptor. Note that this method has the problem that
	 * an empty return path does NOT strictly imply that no such path could
	 * be constructed: We also return an empty vector if the given
	 * Descriptor is a direct child. Therefore we also return a bool value
	 * indicating that the path is valid or not.
	 *
	 * Implicitly this does a breadth-first search on the graph of
	 * StructuredClasses that are transparent. It also takes care of cycles.
	 *
	 * @param childDescriptor is a supposedly valid child Descriptor of this
	 *                        Descriptor.
	 * @return                a tuple containing a path of FieldDescriptors and
	 *                        StructuredClasses between this Descriptor and the
	 *                        input Descriptor and a bool value indicating if
	 *                        the construction was successful.
	 *
	 */
	std::pair<NodeVector<Node>, bool> pathTo(
	    Handle<StructuredClass> childDescriptor, Logger &logger) const;
	/**
	 * This tries to construct the shortest possible path of this Descriptor
	 * to the given FieldDescriptor. Note that this method has the problem that
	 * an empty return path does NOT strictly imply that no such path could
	 * be constructed: We also return an empty vector if the given
	 * FieldDescriptor is a direct child. Therefore we also return a bool value
	 * indicating that the path is valid or not.
	 *
	 *
	 * Implicitly this does a breadth-first search on the graph of
	 * StructuredClasses that are transparent. It also takes care of cycles.
	 *
	 * @param field is a FieldDescriptor that may be allowed as child of this
	 *              Descriptor.
	 * @return      a path of FieldDescriptors and StructuredClasses between
	 *              this Descriptor and the input FieldDescriptor or an empty
	 *              vector if no such path could be constructed.
	 */
	NodeVector<Node> pathTo(Handle<FieldDescriptor> field,
	                        Logger &logger) const;

	/**
	 * Returns a vector of all TREE fields that are allowed as structure tree
	 * children of an instance of this Descriptor. This also makes use of
	 * transparency.
	 * The list is sorted by the number of transparent elements that have to be
	 * constructed to arrive at the respective FieldDescriptor.
	 *
	 * @return a vector of all TREE fields that are allowed as structure tree
	 *         children of an instance of this Descriptor.
	 */
	NodeVector<FieldDescriptor> getDefaultFields() const;

	/**
	 * Returns the name of this FieldDescriptor or the default field name
	 * if the name is empty.
	 *
	 * @return the name of this FieldDescriptor or the default field name
	 * if the name is empty.
	 */
	std::string getNameOrDefaultName()
	{
		std::string name = getName();
		if (name.empty()) {
			return DEFAULT_FIELD_NAME;
		} else {
			return std::move(name);
		}
	}

	/**
	 * Returns a pointer to the start TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor starts.
	 *
	 * Note that this does not invalidate the FieldDescriptor. So use with
	 * care.
	 *
	 * @return a pointer to the start TokenDescriptor.
	 */
	TokenDescriptor *getStartTokenPointer() { return &startToken; }

	/**
	 * Returns a copy of the start TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor starts.
	 *
	 * @return a copy of the start TokenDescriptor.
	 */
	TokenDescriptor getStartToken() const { return startToken; }

	/**
	 * Sets the start TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor starts.
	 *
	 * @param st the new start TokenDescriptor.
	 */
	void setStartToken(TokenDescriptor st)
	{
		invalidate();
		startToken = st;
	}

	/**
	 * Returns a pointer to the end TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor ends.
	 *
	 * @return a pointer to the end TokenDescriptor.
	 */
	TokenDescriptor *getEndTokenPointer() { return &endToken; }

	/**
	 * Returns a copy of the end TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor ends.
	 *
	 * @return a copy of the end TokenDescriptor.
	 */
	TokenDescriptor getEndToken() const { return endToken; }

	/**
	 * Sets the end TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor ends.
	 *
	 * @param e the new end TokenDescriptor.
	 */
	void setEndToken(TokenDescriptor e)
	{
		invalidate();
		endToken = e;
	}

	/**
	 * Returns the WhitespaceMode to be used when an instance of this
	 * FieldDescriptor is parsed.
	 *
	 * @return the WhitespaceMode to be used when an instance of this
	 * FieldDescriptor is parsed.
	 */
	WhitespaceMode getWhitespaceMode() const { return whitespaceMode; }

	/**
	 * Sets the WhitespaceMode to be used when an instance of this
	 * FieldDescriptor is parsed.
	 *
	 * @param wm the WhitespaceMode to be used when an instance of this
	 * FieldDescriptor is parsed.
	 */
	WhitespaceMode setWhitespaceMode(WhitespaceMode wm)
	{
		return whitespaceMode = wm;
	}

	/**
	 * Returns the SyntaxDescriptor for this FieldDescriptor.
	 *
	 * @return the SyntaxDescriptor for this FieldDescriptor.
	 */
	SyntaxDescriptor getSyntaxDescriptor(ssize_t depth = -1)
	{
		SyntaxDescriptor stx{startToken.id, endToken.id, Tokens::Empty,
		                     const_cast<FieldDescriptor *>(this), depth};
		return stx;
	}

	/**
	 * Returns a vector of SyntaxDescriptors, one for each Descriptor
	 * (StructuredClasses, AnnotationClasses or FieldDescriptors) that is
	 * permitted as child of this FieldDescriptor. This also makes use
	 * of transparency.
	 *
	 * @return a vector of SyntaxDescriptors, one for each Descriptor that is
	 *         permitted as child of this FieldDescriptor
	 */
	std::vector<SyntaxDescriptor> getPermittedTokens() const;
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
 * In XML terms the difference between primitive fields and attributes can be
 * explained as the difference between node attributes and node children.
 * Consider the XML
 *
 * \code{.xml}
 * <A key="value">
 *   <key>value</key>
 * </A>
 * \endcode
 *
 * key="value" inside the A-node would be an attribute, while
 * \code{.xml}
 *   <key>value</key>
 * \endcode
 * would be a primitive field. While equivalent in XML the semantics are
 * different: An attribute describes indeed attributes, features of one single
 * node whereas a primitive field describes the _content_ of a node.
 *
 */
class Descriptor : public Node {
	friend FieldDescriptor;

private:
	Owned<StructType> attributesDescriptor;
	NodeVector<FieldDescriptor> fieldDescriptors;
	TokenDescriptor startToken;
	TokenDescriptor endToken;

	bool addAndSortFieldDescriptor(Handle<FieldDescriptor> fd, Logger &logger);

protected:
	void doResolve(ResolutionState &state) override;

	bool doValidate(Logger &logger) const override;

public:
	Descriptor(Manager &mgr, std::string name, Handle<Ontology> ontology)
	    : Node(mgr, std::move(name), ontology),
	      attributesDescriptor(acquire(new StructType(mgr, "", nullptr))),
	      fieldDescriptors(this)
	{
	}

	/**
	 * Returns a reference to the StructType that specifies the attribute keys
	 * as well as value ontologies for this Descriptor.
	 *
	 * @return a reference to the StructType that specifies the attribute keys
	 *         as well as value ontologies for this Descriptor.
	 */
	Rooted<StructType> getAttributesDescriptor() const
	{
		return attributesDescriptor;
	}

	/**
	 * Returns the NodeVector of all FieldDescriptors of this Descriptor.
	 *
	 * @return the NodeVector of all FieldDescriptors of this Descriptor.
	 */
	virtual NodeVector<FieldDescriptor> getFieldDescriptors() const
	{
		return fieldDescriptors;
	}

	/**
	 * Returns the index of the FieldDescriptor with the given name or -1 if no
	 * such FieldDescriptor was found.
	 *
	 * @param name the name of a FieldDescriptor.

	 * @return     the index of the FieldDescriptor with the given name or -1 if
	 *             no such FieldDescriptor was found.
	 */
	ssize_t getFieldDescriptorIndex(
	    const std::string &name = DEFAULT_FIELD_NAME) const;
	/**
	 * Returns the index of the given FieldDescriptor or -1 of the given
	 * FieldDescriptor is not registered at this Descriptor.
	 *
	 * @param fd a FieldDescriptor.

	 * @return   the index of the given FieldDescriptor or -1 of the given
	 *           FieldDescriptor is not registered at this Descriptor.
	 */
	ssize_t getFieldDescriptorIndex(Handle<FieldDescriptor> fd) const;
	/**
	 * Returns the FieldDescriptor with the given name.
	 *
	 * @param name the name of a FieldDescriptor.

	 * @return     the FieldDescriptor with the given name or a nullptr if no
	 *             such FieldDescriptor was found.
	 */
	Rooted<FieldDescriptor> getFieldDescriptor(
	    const std::string &name = DEFAULT_FIELD_NAME) const;

	/**
	 * This returns true if this Descriptor has a FieldDescriptor with the
	 * given name.
	 *
	 * @param name the name of a FieldDescriptor.

	 * @return     true if this Descriptor has a FieldDescriptor with the given
	 *             name
	 */
	bool hasField(const std::string &fieldName = DEFAULT_FIELD_NAME) const
	{
		return getFieldDescriptorIndex(fieldName) != -1;
	}

	/**
	 * Adds the given FieldDescriptor to this Descriptor. This also sets the
	 * parent of the given FieldDescriptor if it is not set yet.
	 *
	 * @param fd is a FieldDescriptor.
	 * @return   returns true if the given FieldDescriptor was not added at the
	 *           end one place before because a TREE field already existed and
	 *           the TREE field has to be at the end.
	 */
	bool addFieldDescriptor(Handle<FieldDescriptor> fd, Logger &logger);

	/**
	 * Adds the given FieldDescriptor to this Descriptor. This also sets the
	 * parent of the given FieldDescriptor if it is not set to this Descriptor
	 * already and removes it from the old parent Descriptor.
	 *
	 * @param fd is a FieldDescriptor.
	 * @return   returns true if the given FieldDescriptor was not added at the
	 *           end one place before because a TREE field already existed and
	 *           the TREE field has to be at the end.
	 */
	bool moveFieldDescriptor(Handle<FieldDescriptor> fd, Logger &logger);

	/**
	 * Copies a FieldDescriptor that belongs to another Descriptor to this
	 * Descriptor.
	 *
	 * @param fd some FieldDescriptor belonging to another Descriptor.
	 * @return   returns true if the given FieldDescriptor was not added at the
	 *           end one place before because a TREE field already existed and
	 *           the TREE field has to be at the end.
	 */
	bool copyFieldDescriptor(Handle<FieldDescriptor> fd, Logger &logger);

	/**
	 * Removes the given FieldDescriptor from this Descriptor. This also sets
	 * the parent of the given FieldDescriptor to null.
	 *
	 * @param fd is a FieldDescriptor.
	 * @return   true if the FieldDescriptor was removed and false if this
	 *           Descriptor did not have the given FieldDescriptor as child.
	 */
	bool removeFieldDescriptor(Handle<FieldDescriptor> fd);

	/**
	 * This creates a new primitive FieldDescriptor and adds it to this
	 * Descriptor.
	 *
	 * @param primitiveType is a handle to some Type in some Typesystem of which
	 *                      one instance is allowed to fill this field.
	 * @param name          is the name of this field.
	 * @param optional      should be set to 'false' is this field needs to be
	 *                      filled in order for an instance of the parent
	 *                      Descriptor to be valid.
	 *
	 * @return              the newly created FieldDescriptor and a bool
	 *                      indicating whether the order of FieldDescriptors had
	 *                      to be changed for the TREE field to be in the last
	 *                      spot.
	 */
	std::pair<Rooted<FieldDescriptor>, bool> createPrimitiveFieldDescriptor(
	    Handle<Type> primitiveType, Logger &logger,
	    FieldDescriptor::FieldType fieldType = FieldDescriptor::FieldType::TREE,
	    std::string name = "", bool optional = false);

	/**
	 * This creates a new primitive FieldDescriptor and adds it to this
	 * Descriptor.
	 *
	 * @param fieldType     is the FieldType of this FieldDescriptor, either
	 *                      TREE for the main or default structure or SUBTREE
	 *                      for supporting structures.
	 * @param name          is the name of this field.
	 * @param optional      should be set to 'false' is this field needs to be
	 *                      filled in order for an instance of the parent
	 *                      Descriptor to be valid.
	 *
	 * @return              the newly created FieldDescriptor and a bool
	 *                      indicating whether the order of FieldDescriptors had
	 *                      to be changed for the TREE field to be in the last
	 *                      spot.
	 */
	std::pair<Rooted<FieldDescriptor>, bool> createFieldDescriptor(
	    Logger &logger,
	    FieldDescriptor::FieldType fieldType = FieldDescriptor::FieldType::TREE,
	    std::string name = "", bool optional = false);

	/**
	 * This tries to construct the shortest possible path of this Descriptor
	 * to the given child Descriptor. As an example consider the book ontology
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
	 * Implicitly this does a breadth-first search on the graph of
	 * StructuredClasses that are transparent. It also takes care of cycles.
	 *
	 * @param childDescriptor is a supposedly valid child Descriptor of this
	 *                        Descriptor.
	 * @return                either a path of FieldDescriptors and
	 *                        StructuredClasses between this Descriptor and
	 *                        the input StructuredClass or an empty vector if
	 *                        no such path can be constructed.
	 *
	 */
	NodeVector<Node> pathTo(Handle<StructuredClass> childDescriptor,
	                        Logger &logger) const;
	/**
	 * This tries to construct the shortest possible path of this Descriptor
	 * to the given FieldDescriptor. Note that this method has the problem that
	 * an empty return path does NOT strictly imply that no such path could
	 * be constructed: We also return an empty vector if the given
	 * FieldDescriptor is a direct child. Therefore we also return a bool value
	 * indicating that the path is valid or not.
	 *
	 *
	 * Implicitly this does a breadth-first search on the graph of
	 * StructuredClasses that are transparent. It also takes care of cycles.
	 *
	 * @param field is a FieldDescriptor that may be allowed as child of this
	 *              Descriptor.
	 * @return      returns a tuple containing a path of FieldDescriptors and
	 *              StructuredClasses between this Descriptor and the input
	 *              FieldDescriptor and a bool value indicating if the
	 *              construction was successful.
	 */
	std::pair<NodeVector<Node>, bool> pathTo(Handle<FieldDescriptor> field,
	                                         Logger &logger) const;

	/**
	 * Returns a vector of all TREE fields that are allowed as structure tree
	 * children of an instance of this Descriptor. This also makes use of
	 * transparency.
	 * The list is sorted by the number of transparent elements that have to be
	 * constructed to arrive at the respective FieldDescriptor.
	 *
	 * @return a vector of all TREE fields that are allowed as structure tree
	 *         children of an instance of this Descriptor.
	 */
	NodeVector<FieldDescriptor> getDefaultFields() const;

	/**
	 * Returns a vector of all StructuredClasses that are allowed as children
	 * of an instance of this Descriptor in the structure tree. This also makes
	 * use of transparency.
	 * The list is sorted by the number of transparent elements that have to be
	 * constructed to arrive at the respective FieldDescriptor.
	 *
	 * @return a vector of all StructuredClasses that are allowed as children
	 *         of an instance of this Descriptor in the structure tree.
	 */
	NodeVector<StructuredClass> getPermittedChildren() const;

	/**
	 * Returns a pointer to the start TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor starts.
	 *
	 * @return a pointer to the start TokenDescriptor.
	 */
	TokenDescriptor *getStartTokenPointer() { return &startToken; }

	/**
	 * Returns a copy of the start TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor starts.
	 *
	 * @return a copy of the start TokenDescriptor.
	 */
	TokenDescriptor getStartToken() const { return startToken; }

	/**
	 * Sets the start TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor starts.
	 *
	 * @param st the new start TokenDescriptor.
	 */
	void setStartToken(TokenDescriptor st)
	{
		invalidate();
		startToken = st;
	}

	/**
	 * Returns a pointer to the end TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor ends.
	 *
	 * @return a pointer to the end TokenDescriptor.
	 */
	TokenDescriptor *getEndTokenPointer() { return &endToken; }

	/**
	 * Returns a copy of the end TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor ends.
	 *
	 * @return a copy of the end TokenDescriptor.
	 */
	TokenDescriptor getEndToken() const { return endToken; }

	/**
	 * Sets the end TokenDescriptor. This Token is used as a
	 * signifier during parsing that an instance of this FieldDescriptor ends.
	 *
	 * @param e the new end TokenDescriptor.
	 */
	void setEndToken(TokenDescriptor e)
	{
		invalidate();
		endToken = e;
	}

	/**
	 * Returns the SyntaxDescriptor for this Descriptor.
	 *
	 * @return the SyntaxDescriptor for this Descriptor.
	 */
	virtual SyntaxDescriptor getSyntaxDescriptor(ssize_t depth = -1)
	{
		SyntaxDescriptor stx{startToken.id, endToken.id, Tokens::Empty,
		                     const_cast<Descriptor *>(this), depth};
		return stx;
	}

	/**
	 * Returns a vector of SyntaxDescriptors, one for each Descriptor
	 * (StructuredClasses, AnnotationClasses or FieldDescriptors) that is
	 * permitted as child of this Descriptor. This also makes use
	 * of transparency.
	 *
	 * @return a vector of SyntaxDescriptors, one for each Descriptor that is
	 *         permitted as child of this Descriptor.
	 */
	std::vector<SyntaxDescriptor> getPermittedTokens() const;
};
/*
 * TODO: We should discuss Cardinalities one more time. Is it smart to define
 * cardinalities independent of context? Should we not have at least have the
 * possibility to define it context-dependently?
 */

/**
 * A StructuredClass specifies nodes in the StructureTree of a document that
 * implements this ontology. For more information on the StructureTree please
 * consult the Header documentation above.
 *
 * Note that a StructuredClass may "invade" an existing Ontology description by
 * defining itself as a viable child in one existing field. Consider the
 * example of the "heading" ontology from the header documentation again:
 *
 * \code{.xml}
 * <ontology name="headings">
 * 	<head>
 * 		<import rel="ontology" src="book.oxm"/>
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
 * </ontology>
 * \endcode
 *
 * The "parent" construct allows to "invade" another ontology.
 *
 * This does indeed interfere with an existing ontology and one must carefully
 * craft such parent references to not create undesired side effects. However
 * they provide the most convenient mechanism to extend existing ontologies
 * without having to rewrite them.
 *
 * Another important factor is the 'transparent' flag. Transparent
 * StructureClasses may be implicitly constructed in the document graph.
 * If we go back to our example a user would (without transparency) have to
 * explicitly declare:
 *
 * \code{.xml}
 * <book>
 *   <section>
 *     <paragraph>Text.</paragraph>
 *   </section>
 * </book>
 * \endcode
 *
 * But in our mind the document
 *
 * \code{.xml}
 * <book>
 *   <section>
 *     Text.
 *   </section>
 * </book>
 * \endcode
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
 * Inheritance therefore also goes for fields.
 */
class StructuredClass : public Descriptor {
	friend Ontology;

private:
	const Variant cardinality;
	Owned<StructuredClass> superclass;
	NodeVector<StructuredClass> subclasses;
	bool transparent;
	bool root;
	TokenDescriptor shortToken;

	/**
	 * Helper method for getFieldDescriptors.
	 */
	Rooted<FieldDescriptor> gatherFieldDescriptors(
	    NodeVector<FieldDescriptor> &current,
	    std::unordered_set<const StructuredClass *> &visited,
	    std::set<std::string> &overriddenFields, bool hasTREE) const;

protected:
	bool doValidate(Logger &logger) const override;

public:
	/**
	 * The constructor for a StructuredClass.
	 *
	 * @param mgr                  is the current Manager.
	 * @param name                 is the name of the StructuredClass.
	 * @param ontology             is the Ontology this StructuredClass belongs
	 *                             to.
	 * @param cardinality          specifies how often an element of this type
	 *                             may occur at a specific point in the
	 *                             StructureTree. For example: A document should
	 *                             have at least one author. This is set to *
	 *                             per default, meaning that any number of
	 *                             of instances is valid, including zero.
	 * @param superclass           references a parent StructuredClass. Please
	 *                             look for more information on inheritance in
	 *                             the class documentation above. The default is
	 *                             a null reference, meaning no super class.
	 *                             The constructor automatically registers this
	 *                             class as a subclass at the super class.
	 * @param transparent          specifies whether this StructuredClass is
	 *                             transparent. For more information on
	 *                             transparency please refer to the class
	 *                             documentation above. The default is false.
	 * @param root                 specifies whether this StructuredClass is
	 *                             allowed to be at the root of a Document.
	 */
	StructuredClass(Manager &mgr, std::string name,
	                Handle<Ontology> ontology = nullptr,
	                Variant cardinality = Cardinality::any(),
	                Handle<StructuredClass> superclass = nullptr,
	                bool transparent = false, bool root = false);

	/**
	 * Returns the Cardinality of this StructuredClass (as a RangeSet).
	 *
	 * @return the Cardinality of this StructuredClass (as a RangeSet).
	 */
	const Variant &getCardinality() const { return cardinality; }

	/**
	 * Returns the superclass of this StructuredClass. This is not the same as
	 * the parents in the Structure Tree!
	 *
	 * @return the superclass of this StructuredClass.
	 */
	Rooted<StructuredClass> getSuperclass() const { return superclass; }

	/**
	 * Sets the superclass of this StructuredClass. This is not the same as
	 * the parents in the Structure Tree!
	 *
	 * This will also register this class as a subclass at the given superclass
	 * and unregister it at the previous superclass.
	 *
	 * It will also set the parent for this Descriptors AttributesDescriptor.
	 *
	 * @param sup    some StructuredClass that shall be the new superclass of
	 *               this StructuredClass.
	 * @param logger is some logger. Errors during setting the parent for this
	 *               Descriptors AttributesDescriptor will be written into this
	 *               logger.
	 */
	void setSuperclass(Handle<StructuredClass> sup, Logger &logger);

	/**
	 * Returns true if this class is a subclass of the given class. It does not
	 * return true if the other class is equal to the given class.
	 *
	 * @param c is another class that might or might not be a superclass of this
	 *          one
	 * @return  true if this class is a subclass of the given class.
	 *
	 */
	bool isSubclassOf(Handle<StructuredClass> c) const;

	/**
	 * Returns the StructuredClasses that are subclasses of this class. This
	 * is the inverted version of isa, meaning: each class c that has a isa
	 * relationship to this class is part of the returned vector.
	 *
	 * Note that the order of subclasses is not strictly defined.
	 *
	 * You are not allowed to add subclasses directly to the vector. When you
	 * construct a new StructuredClass with a non-empty isa-handle it will
	 * automatically register as subclass at the super class.
	 *
	 * @return the StructuredClasses that are subclasses of this class.
	 */
	const NodeVector<StructuredClass> &getSubclasses() const
	{
		return subclasses;
	}

	/**
	 * Adds a subclass to this StructuredClass. This also calls setSuperclass
	 * on the given subclass.
	 *
	 * @param sc is some StructuredClass.
	 * @param logger is some logger. Errors during setting the parent for the
	 *               new subclasses AttributesDescriptor will be written into
	 *               this logger.
	 */
	void addSubclass(Handle<StructuredClass> sc, Logger &logger);

	/**
	 * Removes a subclass from this StructuredClass. This also calls
	 * setSuperclass(nullptr) on the given subclass.
	 *
	 * @param sc is some StructuredClass.
	 * @param logger is some logger. Errors during setting the parent for the
	 *               removed subclasses AttributesDescriptor will be written
	 *               into this logger.
	 */
	void removeSubclass(Handle<StructuredClass> sc, Logger &logger);

	/**
	 * Returns a NodeVector of all FieldDescriptors of
	 * this StructuredClass. This also merges the FieldDescriptors directly
	 * belonging to this StructuredClass with all FieldDescritptors of its
	 * Superclass (and so on recurvively). The order of field descriptors is
	 * as follows:
	 * 1.) non-overridden SUBTREE FieldDescriptors of super classes.
	 * 2.) SUBTREE FieldDescriptors of this class.
	 * 3.) TREE FieldDescriptor (either inherited from super class or direct)
	 *
	 * @return a NodeVector of all FieldDescriptors of this StructuredClass.
	 */
	NodeVector<FieldDescriptor> getFieldDescriptors() const override;

	bool isTransparent() const { return transparent; }

	void setTransparent(bool t)
	{
		invalidate();
		transparent = std::move(t);
	}

	bool hasRootPermission() const { return root; }

	void setRootPermission(bool r)
	{
		invalidate();
		root = std::move(r);
	}

	/**
	 * Returns a pointer to the short TokenDescriptor. During parsing an
	 * occurence of this token will be translated to an empty instance of this
	 * StructuredClass.
	 *
	 * @return a pointer to the short TokenDescriptor.
	 */
	TokenDescriptor *getShortTokenPointer() { return &shortToken; }

	/**
	 * Returns a copy of the short TokenDescriptor. During parsing an
	 * occurence of this token will be translated to an empty instance of this
	 * StructuredClass.
	 *
	 * @return a copy of the short TokenDescriptor.
	 */
	TokenDescriptor getShortToken() const { return shortToken; }

	/**
	 * Sets the short TokenDescriptor. During parsing an
	 * occurence of this token will be translated to an empty instance of this
	 * StructuredClass.
	 *
	 * @param s the new short TokenDescriptor.
	 */
	void setShortToken(TokenDescriptor s)
	{
		invalidate();
		shortToken = s;
	}

	/**
	 * Returns the SyntaxDescriptor for this StructuredClass.
	 *
	 * @return the SyntaxDescriptor for this StructuredClass.
	 */
	SyntaxDescriptor getSyntaxDescriptor(ssize_t depth = -1) override
	{
		SyntaxDescriptor stx{getStartToken().id, getEndToken().id,
		                     shortToken.id, const_cast<StructuredClass *>(this),
		                     depth};
		return stx;
	}
};

/**
 * An AnnotationClass defines allowed Annotations. For more information on
 * Annotations please refer to the Document.hpp.
 *
 * This class has no special properties and is in essence just a Descriptor.
 */
class AnnotationClass : public Descriptor {
	friend Ontology;

public:
	/**
	 * The constructor for a new AnnotationClass. Note that you have to add
	 * the FieldDescriptors to it later on.
	 *
	 * @param mgr                  is the Manager instance.
	 * @param name                 is a name for this AnnotationClass that will
	 *                             be used for later references to this
	 *                             AnnotationClass.
	 * @param ontology             is the Ontology this AnnotationClass belongs
	 *                             to.
	 */
	AnnotationClass(Manager &mgr, std::string name, Handle<Ontology> ontology);
};

/**
 * A Ontology node specifies which StructuredClasses and which AnnotationClasses
 * are part of this ontology. TODO: Do we want to be able to restrict
 * Annotations
 * to certain Structures?
 */
class Ontology : public RootNode {
	friend StructuredClass;
	friend AnnotationClass;

private:
	NodeVector<StructuredClass> structuredClasses;
	NodeVector<AnnotationClass> annotationClasses;
	NodeVector<Typesystem> typesystems;
	NodeVector<Ontology> ontologies;

protected:
	void doResolve(ResolutionState &state) override;
	bool doValidate(Logger &logger) const override;
	void doReference(Handle<Node> node) override;
	RttiSet doGetReferenceTypes() const override;

public:
	/**
	 * The constructor for a new ontology. Note that this is an empty Ontology
	 * and still has to be filled with StructuredClasses and AnnotationClasses.
	 *
	 * @param mgr  is the Manager instance.
	 * @param name is a name for this ontology which will be used for later
	 *             references to this Ontology.
	 */
	Ontology(Manager &mgr, std::string name = "")
	    : RootNode(mgr, std::move(name), nullptr),
	      structuredClasses(this),
	      annotationClasses(this),
	      typesystems(this),
	      ontologies(this)
	{
	}

	/**
	 * The constructor for a new ontology. Note that this is an empty Ontology
	 * and still has to be filled with StructuredClasses and AnnotationClasses.
	 *
	 * @param mgr  is the Manager instance.
	 * @param sys  is the SystemTypesystem instance.
	 * @param name is a name for this ontology which will be used for later
	 *             references to this Ontology.
	 */
	Ontology(Manager &mgr, Handle<SystemTypesystem> sys, std::string name = "")
	    : Ontology(mgr, std::move(name))
	{
		referenceTypesystem(sys);
	}

	/**
	 * Creates a new Ontology and returns it.
	 *
	 * @param mgr  is the Manager instance.
	 * @param name is a name for this ontology which will be used for later
	 *             references to this Ontology.
	 */
	static Rooted<Ontology> createEmptyOntology(Manager &mgr, std::string name)
	{
		return Rooted<Ontology>{new Ontology(mgr, std::move(name))};
	}

	/**
	 * Returns a const reference to the NodeVector of StructuredClasses that are
	 * part of this Ontology.
	 *
	 * @return a const reference to the NodeVector of StructuredClasses that are
	 * part of this Ontology.
	 */
	const NodeVector<StructuredClass> &getStructureClasses() const
	{
		return structuredClasses;
	}
	/**
	 * Adds a StructuredClass to this ontology. This also sets the parent of the
	 * given StructuredClass if it is not set to this Ontology already and
	 * removes it from the old Ontology.
	 *
	 * @param s is some StructuredClass.
	 */
	void addStructuredClass(Handle<StructuredClass> s);

	/**
	 * Removes a StructuredClass from this ontology. This also sets the parent
	 * of the given StructuredClass to null.
	 *
	 * @param s is some StructuredClass.
	 * @return  true if the given StructuredClass was removed and false if this
	 *          Ontology did not have the given StructuredClass as child.
	 */
	bool removeStructuredClass(Handle<StructuredClass> s);

	/**
	 * This creates a new StructuredClass and appends it to this Ontology.
	 *
	 * @param name                 is the name of the StructuredClass.
	 * @param cardinality          specifies how often an element of this type
	 *                             may occur at a specific point in the
	 *                             StructureTree. For example: A document should
	 *                             have at least one author. This is set to *
	 *                             per default, meaning that any number of
	 *                             of instances is valid, including zero.
	 * @param superclass           references a parent StructuredClass. Please
	 *                             look for more information on inheritance in
	 *                             the class documentation above. The default is
	 *                             a null reference, meaning no super class.
	 *                             The constructor automatically registers this
	 *                             class as a subclass at the super class.
	 * @param transparent          specifies whether this StructuredClass is
	 *                             transparent. For more information on
	 *                             transparency please refer to the class
	 *                             documentation above. The default is false.
	 * @param root                 specifies whether this StructuredClass is
	 *                             allowed to be at the root of a Document.
	 *
	 * @return                     the newly created StructuredClass.
	 */
	Rooted<StructuredClass> createStructuredClass(
	    std::string name, Variant cardinality = Cardinality::any(),
	    Handle<StructuredClass> superclass = nullptr, bool transparent = false,
	    bool root = false);

	/**
	 * Returns a const reference to the NodeVector of AnnotationClasses that are
	 * part of this Ontology.
	 *
	 * @return a const reference to the NodeVector of AnnotationClasses that are
	 * part of this Ontology.
	 */
	const NodeVector<AnnotationClass> &getAnnotationClasses() const
	{
		return annotationClasses;
	}
	/**
	 * Adds an AnnotationClass to this ontology. This also sets the parent of
	 * the given AnnotationClass if it is not set to this Ontology already and
	 * removes it from the old Ontology.
	 *
	 * @param a is some AnnotationClass.
	 */
	void addAnnotationClass(Handle<AnnotationClass> a);

	/**
	 * Removes a AnnotationClass from this ontology. This also sets the parent
	 * of the given AnnotationClass to null.
	 *
	 * @param a is some AnnotationClass.
	 * @return  true if the given AnnotationClass was removed and false if this
	 *          Ontology did not have the given AnnotationClass as child.
	 */
	bool removeAnnotationClass(Handle<AnnotationClass> a);

	/**
	 * This creates a new AnnotationClass and appends it to this Ontology.
	 *
	 * @param name                 is a name for this AnnotationClass that will
	 *                             be used for later references to this
	 *                             AnnotationClass.
	 */
	Rooted<AnnotationClass> createAnnotationClass(std::string name);

	/**
	 * Returns a const reference to the NodeVector of TypeSystems that are
	 * references in this Ontology.
	 *
	 * @return a const reference to the NodeVector of TypeSystems that are
	 * references in this Ontology.
	 */
	const NodeVector<Typesystem> &getTypesystems() const { return typesystems; }

	/**
	 * Adds a Typesystem reference to this Ontology.
	 */
	void referenceTypesystem(Handle<Typesystem> t) { typesystems.push_back(t); }

	/**
	 * Adds multiple Typesystem references to this Ontology.
	 */
	void referenceTypesystems(const std::vector<Handle<Typesystem>> &ts)
	{
		typesystems.insert(typesystems.end(), ts.begin(), ts.end());
	}

	/**
	 * Adds a Ontology reference to this Ontology.
	 */
	void referenceOntology(Handle<Ontology> d) { ontologies.push_back(d); }

	/**
	 * Adds multiple Ontology references to this Ontology.
	 */
	void referenceOntologys(const std::vector<Handle<Ontology>> &ds)
	{
		ontologies.insert(ontologies.end(), ds.begin(), ds.end());
	}

	/**
	 * Returns all TokenDescriptors of classes and fields in this Ontology.
	 *
	 * @return all TokenDescriptors of classes and fields in this Ontology.
	 */
	std::vector<TokenDescriptor *> getAllTokenDescriptors() const;
};

namespace RttiTypes {

extern const Rtti FieldDescriptor;
extern const Rtti Descriptor;
extern const Rtti StructuredClass;
extern const Rtti AnnotationClass;
extern const Rtti Ontology;
}
}

#endif /* _OUSIA_MODEL_ONTOLOGY_HPP_ */
