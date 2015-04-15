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
 * @file Document.hpp
 *
 * This header contains the class hierarchy of actual document classes. A graph
 * of connected instances of these nodes is a "Document". How the different
 * DocumentEntity instances may be connected within the graph is subject to the
 * specification in the respective Ontology(s) (see also the Ontology.hpp).
 *
 * A Document, from top to bottom, consists of "Document" instance,
 * which "owns" the structural root node of the in-document graph. This might
 * for example be a "book" node of the "book" ontology. That root node in turn
 * has structure nodes as children, which in turn may have children. This
 * constitutes a Structure Tree. Additionally annotations may be attached to
 * Structure Nodes, effectively resulting in a Document Graph instead of a
 * Document Tree (other references may introduce cycles as well).
 *
 * Consider this XML representation of a document using the "book" ontology:
 *
 * \code{.xml}
 * <doc>
 * 	<head>
 * 		<import rel="ontology" src="book_ontology.oxm"/>
 * 		<import rel="ontology" src="emphasized_ontology.oxm"/>
 * 		<alias tag="paragraph" aka="p"/>
 * 	</head>
 * 	<book>
 * 		This might be some introductory text or a dedication. Ideally, of
 * 		course, such elements would be semantically specified as such in
 * 		additional ontologies (or in this one).
 * 		<chapter name="myFirstChapter">
 * 			Here we might have an introduction to the chapter, including some
 * 			overview of the chapters structure.
 * 			<section name="myFirstSection">
 * 				Here we might find the actual section content.
 * 			</section>
 * 			<section name="mySndSection">
 * 				Here we might find the actual section <em>content</em>.
 *
 *
 * 				And there might even be another paragraph.
 * 			</section>
 * 		</chapter>
 * 	</book>
 * </doc>
 * \endcode
 *
 * As can be seen the StructureEntities inherently follow a tree structure that
 * is restricted by the implicit context free grammar of the "book" Ontology
 * definition (e.g. it is not allowed to have a "book" node inside a "section";
 * refer to te Ontology.hpp for more information).
 *
 * Another interesting fact is the special place of AnnotationEntities: They are
 * Defined by start and end Anchors in the text. Note that this allows for
 * overlapping annotations and provides a more intuitive (and semantically
 * sound) handling of such span-like concepts. So the
 *
 * \code{.xml}
 * <em>content</em>
 * \endcode
 *
 * is implicitly expanded to:
 *
 * \code{.xml}
 * <a id="1"/>content<a id="2"/>
 * <emphasized start="1" end="2"/>
 * \endcode
 *
 * Note that the place of an AnnotationEntity within the XML above is not
 * strictly defined. It might as well be placed as a child of the "book" node.
 * In general it is recommended to use the lowest possible place in the
 * StructureTree to include the AnnotationEntity for better readability.
 *
 * Also note that text content like
 *
 * Here we might find the actual section content.
 *
 * is implicitly expanded using transparency to:
 *
 * \code{.xml}
 * <paragraph>
 * 	<text>
 * 		Here we might find the actual section content.
 * 	</text>
 * </paragraph>
 * \endcode
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_DOCUMENT_HPP_
#define _OUSIA_MODEL_DOCUMENT_HPP_

#include <set>

#include <core/managed/ManagedContainer.hpp>
#include <core/common/Variant.hpp>

#include "Node.hpp"
#include "Ontology.hpp"
#include "RootNode.hpp"
#include "Typesystem.hpp"

namespace ousia {

// Forward declarations
class Rtti;
class Document;
class StructureNode;
class StructuredEntity;
class DocumentPrimitive;
class AnnotationEntity;
class Anchor;

/**
 * A DocumentEntity is the common superclass for StructuredEntities and
 * AnnotationEntities. Similarly to DescriptorEntity in the Ontology.hpp it
 * defines that each node in the Document graph may have attributes (in form
 * of a struct Variant), and fields.
 * The fields here are a vector of vectors. The first vector implements all
 * fields while the inner vector contains all children in this field.
 * We provide, however, convenience functions for better access via the field
 * name.
 */
class DocumentEntity {
private:
	/**
	 * this is a rather dirty method that should not be used in other cases:
	 * We store a handle to the Node instance that inherits from
	 * DocumentEntity. This Handle is not registered and would lead to Segfaults
	 * if we could not garantuee that it lives exactly as long as this
	 * DocumentEntity because the handle is for the subclass instance.
	 */
	Handle<Node> subInst;
	Owned<Descriptor> descriptor;
	Variant attributes;
	std::vector<NodeVector<StructureNode>> fields;

	void invalidateSubInstance();

	template <typename Iterator>
	Rooted<Anchor> searchStartAnchorInField(
	    Handle<AnnotationClass> desc, const std::string &name, Iterator begin,
	    Iterator end, std::unordered_set<const DocumentEntity *> &visited);

	Rooted<Anchor> searchStartAnchorDownwards(
	    Handle<AnnotationClass> desc, const std::string &name,
	    std::unordered_set<const DocumentEntity *> &visited);

	Rooted<Anchor> searchStartAnchorUpwards(
	    Handle<AnnotationClass> desc, const std::string &name,
	    const DocumentEntity *child,
	    std::unordered_set<const DocumentEntity *> &visited);

protected:
	bool doValidate(Logger &logger) const;

	std::vector<NodeVector<StructureNode>> &getFields() { return fields; }

public:
	/**
	 * The constructor for a DocumentEntity. Node that this does not inherit
	 * from Node. Therefore we need to have a handle to the subclass Node
	 * instance to create NodeVectors and Owned references.
	 *
	 * @param subInst    is a handle to the subclass instance
	 *                   (e.g. StructuredEntity), such that the fields vectors
	 *                   and the descriptor reference can be obtained.
	 * @param descriptor is the Descriptor for this DocumentEntity, which will
	 *                   transformed to an Owned reference of the given owner.
	 * @param attributes is a Map Variant adhering to the attribute StructType
	 *                   in the given descriptor.
	 */
	DocumentEntity(Handle<Node> subInst, Handle<Descriptor> descriptor,
	               Variant attributes);

	/**
	 * Returns the Descriptor for this DocumentEntity.
	 *
	 * @return the Descriptor for this DocumentEntity.
	 */
	Rooted<Descriptor> getDescriptor() const { return descriptor; }

	/**
	 * Sets the Descriptor for this DocumentEntity.
	 *
	 * @param d is the new Descriptor for this DocumentEntity.
	 */
	void setDescriptor(Handle<Descriptor> d);

	/**
	 * Returns a Map Variant adhering to the attribute StructType in the given
	 * descriptor.
	 *
	 * @return a Map Variant adhering to the attribute StructType in the given
	 * descriptor.
	 */
	Variant getAttributes() const { return attributes; }

	/**
	 * Sets the attributes for this DocumentEntity. Attributes are set as a Map
	 * variant.
	 *
	 * @param a is a Map variant containing the attributes for this
	 *          DocumentEntity.
	 */
	void setAttributes(const Variant &a);

	/**
	 * Returns a vector of NodeVectors, containing the StructureNode instances
	 * stored within the fields.
	 *
	 * @return a vector containing a NodeVector for each field.
	 */
	const std::vector<NodeVector<StructureNode>> &getFields() const { return fields; }

	/**
	 * This returns the vector of entities containing all members of the field
	 * with the given name.
	 *
	 * If the name is unknown an exception is thrown.
	 *
	 * @param fieldName is the name of a field as specified in the
	 *                  FieldDescriptor in the Ontology description.
	 * @return          a NodeVector of all StructuredEntities in that field.
	 */
	const NodeVector<StructureNode> &getField(
	    const std::string &fieldName = DEFAULT_FIELD_NAME) const;

	/**
	 * This returns the vector of entities containing all members of the field
	 * with the given FieldDescriptor.
	 *
	 * If the FieldDescriptor does not belong to the Descriptor of this node
	 * an exception is thrown.
	 *
	 * @param fieldDescriptor is a FieldDescriptor defined in the Descriptor for
	 *                        this DocumentEntity.
	 * @return                a NodeVector of all StructuredEntities in that
	 *                        field.
	 */
	const NodeVector<StructureNode> &getField(
	    Handle<FieldDescriptor> fieldDescriptor) const;

	/**
	 * This returns the vector of entities containing all members of the field
	 * with the given index.
	 *
	 * If the index is out of bounds an exception is thrown.
	 *
	 * @param idx       is the index of a field as specified in the
	 *                  FieldDescriptor in the Ontology description.
	 * @return          a NodeVector of all StructuredEntities in that field.
	 */
	const NodeVector<StructureNode> &getField(size_t idx) const;

	/**
	 * This adds a StructureNode to the field with the given index.
	 *
	 * This method also changes the parent of the newly added StructureNode if
	 * it is not set to this DocumentEntity already and removes it from the
	 * old parent.
	 *
	 * @param s         is the StructureNode that shall be added.
	 * @param fieldIdx  is the index of a field as specified in the
	 *                  FieldDescriptor in the Ontology description.
	 */
	void addStructureNode(Handle<StructureNode> s, size_t fieldIdx);

	/**
	 * This adds a StructureNode to the field with the given name.
	 *
	 * If the name is unknown an exception is thrown.
	 *
	 * This method also changes the parent of the newly added StructureNode if
	 * it is not set to this DocumentEntity already and removes it from the
	 * old parent.
	 *
	 * @param s         is the StructureNode that shall be added.
	 * @param fieldName is the name of a field as specified in the
	 *                  FieldDescriptor in the Ontology description.
	 */
	void addStructureNode(Handle<StructureNode> s,
	                      const std::string &fieldName = DEFAULT_FIELD_NAME);

	/**
	 * This adds multiple StructureNodes to the field with the given name.
	 *
	 * If the name is unknown an exception is thrown.
	 *
	 * This method also changes the parent of each newly added StructureNode if
	 * it is not set to this DocumentEntity already and removes it from the
	 * old parent.
	 *
	 * @param ss        are the StructureNodes that shall be added.
	 * @param fieldName is the name of a field as specified in the
	 *                  FieldDescriptor in the Ontology description.
	 */
	void addStructureNodes(const std::vector<Handle<StructureNode>> &ss,
	                       const std::string &fieldName = DEFAULT_FIELD_NAME);

	/**
	 * This removes a StructureNode from the field with the given index.
	 *
	 * This method also changes the parent of the removed StructureNode to null.
	 *
	 * @param s         is the StructureNode that shall be removed.
	 * @param fieldIdx  is the index of a field as specified in the
	 *                  FieldDescriptor in the Ontology description.
	 * @return          true if this StructureNode was a child here and false if
	 *                  if was not found.
	 */
	bool removeStructureNodeFromField(Handle<StructureNode> s, size_t fieldIdx);

	/**
	 * This removes a StructureNode from the field with the given name.
	 *
	 * If the name is unknown an exception is thrown.
	 *
	 * This method also changes the parent of the removed StructureNode to null.
	 *
	 * @param s         is the StructureNode that shall be removed.
	 * @param fieldName is the name of a field as specified in the
	 *                  FieldDescriptor in the Ontology description.
	 * @return          true if this StructureNode was a child here and false if
	 *                  if was not found.
	 */
	bool removeStructureNodeFromField(
	    Handle<StructureNode> s,
	    const std::string &fieldName = DEFAULT_FIELD_NAME);

	/**
	 * This adds a StructureNode to the field with the given FieldDescriptor.
	 *
	 * If the FieldDescriptor does not belong to the Descriptor of this node
	 * an exception is thrown.
	 *
	 * This method also changes the parent of the newly added StructureNode if
	 * it is not set to this DocumentEntity already and removes it from the
	 * old parent.
	 *
	 * @param s               is the StructureNode that shall be added.
	 * @param fieldDescriptor is a FieldDescriptor defined in the Descriptor for
	 *                        this DocumentEntity.
	 */
	void addStructureNode(Handle<StructureNode> s,
	                      Handle<FieldDescriptor> fieldDescriptor);

	/**
	 * This adds multiple StructureNodes to the field with the given
	 * FieldDescriptor.
	 *
	 * If the FieldDescriptor does not belong to the Descriptor of this node
	 * an exception is thrown.
	 *
	 * This method also changes the parent of each newly added StructureNode if
	 * it is not set to this DocumentEntity already and removes it from the
	 * old parent.
	 *
	 * @param ss              are the StructureNodes that shall be added.
	 * @param fieldDescriptor is a FieldDescriptor defined in the Descriptor for
	 *                        this DocumentEntity.
	 */
	void addStructureNodes(const std::vector<Handle<StructureNode>> &ss,
	                       Handle<FieldDescriptor> fieldDescriptor);

	/**
	 * This removes a StructureNode from the field with the given
	 * FieldDescriptor.
	 *
	 * This method also changes the parent of the removed StructureNode to null.
	 *
	 * @param s         is the StructureNode that shall be removed.
	 * @param fieldDescriptor is a FieldDescriptor defined in the Descriptor for
	 *                        this DocumentEntity.
	 * @return          true if this StructureNode was a child here and false if
	 *                  if was not found.
	 */
	bool removeStructureNodeFromField(Handle<StructureNode> s,
	                                  Handle<FieldDescriptor> fieldDescriptor);

	/**
	 * This removes a StructureNode from this DocumentEntity. It iterates
	 * through all fields to find it.
	 *
	 * This method also changes the parent of the removed StructureNode to null.
	 *
	 * @param s is the StructureNode that shall be removed.
	 * @return  true if this StructureNode was a child here and false if if was
	 *          not found.
	 */
	bool removeStructureNode(Handle<StructureNode> s);

	/**
	 * This creates a new StructuredEntity as child of this DocumentEntity.
	 *
	 * @param descriptor is the StructuredClass of this StructuredEntity.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   StructuredEntity. It is empty per default.
	 * @param fieldName  is the name of the field, where the newly created
	 *                   StructuredEntity shall be added to this DocumentEntity.
	 * @param name       is some name for this StructuredEntity that may be used
	 *                   for later reference. It is empty per default.
	 * @return           the newly created StructuredEntity.
	 */
	Rooted<StructuredEntity> createChildStructuredEntity(
	    Handle<StructuredClass> descriptor,
	    Variant attributes = Variant::mapType{},
	    const std::string &fieldName = DEFAULT_FIELD_NAME,
	    std::string name = "");

	/**
	 * This creates a new StructuredEntity as child of this DocumentEntity.
	 *
	 * @param descriptor is the StructuredClass of this StructuredEntity.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   StructuredEntity. It is empty per default.
	 * @param fieldIdx   is the index of the field, where the newly created
	 *                   StructuredEntity shall be added to this DocumentEntity.
	 * @param name       is some name for this StructuredEntity that may be used
	 *                   for later reference. It is empty per default.
	 * @return           the newly created StructuredEntity.
	 */
	Rooted<StructuredEntity> createChildStructuredEntity(
	    Handle<StructuredClass> descriptor, size_t fieldIdx,
	    Variant attributes = Variant::mapType{}, std::string name = "");

	/**
	 * Creates a new DocumentPrimitive as child of this DocumentEntity.
	 *
	 * @param content   is a Variant containing the content of this
	 *                  DocumentPrimitive. The Type of this Variant is
	 *                  specified at the parents Descriptor for the given
	 *                  fieldName.
	 * @param fieldName is the name of the field, where the newly created
	 *                  StructuredEntity shall be added to this DocumentEntity.
	 * @return          the newly created DocumentPrimitive.
	 */
	Rooted<DocumentPrimitive> createChildDocumentPrimitive(
	    Variant content, const std::string &fieldName = DEFAULT_FIELD_NAME);

	/**
	 * Creates a new DocumentPrimitive as child of this DocumentEntity.
	 *
	 * @param fieldIdx  is the index of the field, where the newly created
	 *                  StructuredEntity shall be added to this DocumentEntity.
	 * @param content   is a Variant containing the content of this
	 *                  DocumentPrimitive. The Type of this Variant is
	 *                  specified at the parents Descriptor for the given
	 *                  fieldName.
	 * @return          the newly created DocumentPrimitive.
	 */
	Rooted<DocumentPrimitive> createChildDocumentPrimitive(Variant content,
	                                                       size_t fieldIdx);

	/**
	 * Creates a new Anchor as child of this DocumentEntity.
	 *
	 * @param fieldName is the name of the field, where the newly created
	 *                  Anchor shall be added to this DocumentEntity.
	 * @return          the newly created Anchor.
	 */
	Rooted<Anchor> createChildAnchor(
	    const std::string &fieldName = DEFAULT_FIELD_NAME);

	/**
	 * Creates a new Anchor as child of this DocumentEntity.
	 *
	 * @param fieldIdx  is the index of the field, where the newly created
	 *                  Anchor shall be added to this DocumentEntity.
	 * @return          the newly created Anchor.
	 */
	Rooted<Anchor> createChildAnchor(size_t fieldIdx);

	/**
	 * Does an inverse depth first search starting at this DocumentEntity to
	 * find a child Anchor element that matches the given seach criteria.
	 * The search will not cross SUBTREE to TREE field boundaries and will not
	 * leave AnnotationEntities upwards. If no Anchor is found a nullptr is
	 * returned. AnnotationEntities which already have an end Anchor won't be
	 * returned.
	 *
	 * @param desc is the AnnotationClass of the AnnotationEntity whose
	 *             start Anchor you are looking for.
	 * @param name is the AnnotationEntities name.
	 * @return     the start Anchor or a nullptr if no Anchor could be found.
	 */
	Rooted<Anchor> searchStartAnchor(size_t fieldIdx,
	                                 Handle<AnnotationClass> desc = nullptr,
	                                 const std::string &name = "");
};

/**
 * The DocumentNode class is a dummy class for being capable of selecting
 * all nodes in the document graph e.g. when resolving element names.
 */
class DocumentNode : public Node {
public:
	using Node::Node;
};

/**
 * A StructureNode is a Node of the StructureTree of the document. This is a
 * common superclass for StructuredEntity, Anchor and DocumentPrimitive.
 */
class StructureNode : public DocumentNode {
	friend DocumentEntity;

protected:
	bool doValidate(Logger &logger) const override;

public:
	/**
	 * Constructor for a StructureNode in the StructureTree.
	 */
	StructureNode(Manager &mgr, std::string name, Handle<Node> parent,
	              const std::string &fieldName);

	/**
	 * Constructor for a StructureNode in the StructureTree.
	 */
	StructureNode(Manager &mgr, std::string name, Handle<Node> parent,
	              size_t fieldIdx);

	/**
	 * Constructor for an empty StructureNode.
	 */
	StructureNode(Manager &mgr, std::string name = "",
	              Handle<Node> parent = nullptr);
};

/**
 * A StructuredEntity is an instance of a StructuredClass. For more
 * information please refer to the header documentation above.
 */
class StructuredEntity : public StructureNode, public DocumentEntity {
	friend Document;

private:
	bool transparent = false;

protected:
	void doResolve(ResolutionState &state) override;
	bool doValidate(Logger &logger) const override;

public:
	/**
	 * Constructor for a StructuredEntity in the Structure Tree.
	 *
	 * @param mgr        is the Manager instance.
	 * @param parent     is the parent DocumentEntity of this StructuredEntity
	 *                   in the DocumentTree. Note that this StructuredEntity
	 *                   will automatically register itself as child of this
	 *                   parent.
	 * @param descriptor is the StructuredClass of this StructuredEntity.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   StructuredEntity. It is empty per default.
	 * @param fieldName  is the name of the field in the parent DocumentEntity
	 *                   where this StructuredEntity shall be added.
	 * @param name       is some name for this StructuredEntity that may be used
	 *                   for later reference. It is empty per default.
	 */
	StructuredEntity(Manager &mgr, Handle<Node> parent,
	                 Handle<StructuredClass> descriptor,
	                 Variant attributes = Variant::mapType{},
	                 const std::string &fieldName = DEFAULT_FIELD_NAME,
	                 std::string name = "")
	    : StructureNode(mgr, std::move(name), parent, fieldName),
	      DocumentEntity(this, descriptor, std::move(attributes))
	{
	}
	/**
	 * Constructor for a StructuredEntity in the Structure Tree.
	 *
	 * @param mgr        is the Manager instance.
	 * @param parent     is the parent DocumentEntity of this StructuredEntity
	 *                   in the DocumentTree. Note that this StructuredEntity
	 *                   will automatically register itself as child of this
	 *                   parent.
	 * @param descriptor is the StructuredClass of this StructuredEntity.
	 * @param fieldIdx   is the index of the field in the parent DocumentEntity
	 *                   where this StructuredEntity shall be added.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   StructuredEntity. It is empty per default.
	 * @param name       is some name for this StructuredEntity that may be used
	 *                   for later reference. It is empty per default.
	 */
	StructuredEntity(Manager &mgr, Handle<Node> parent,
	                 Handle<StructuredClass> descriptor, size_t fieldIdx,
	                 Variant attributes = Variant::mapType{},
	                 std::string name = "")
	    : StructureNode(mgr, std::move(name), parent, fieldIdx),
	      DocumentEntity(this, descriptor, std::move(attributes))
	{
	}

	/**
	 * Constructor for a StructuredEntity at the document root.
	 *
	 * @param mgr        is the Manager instance.
	 * @param parent     is the parent Document of this StructuredEntity. Note
	 *                   that this StructuredEntity will automatically register
	 *                   itself as child of this Document.
	 * @param descriptor is the StructuredClass of this StructuredEntity.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   StructuredEntity. It is empty per default.
	 * @param name       is some name for this StructuredEntity that may be used
	 *                   for later reference. It is empty per default.
	 */
	StructuredEntity(Manager &mgr, Handle<Document> doc,
	                 Handle<StructuredClass> descriptor,
	                 Variant attributes = Variant::mapType{},
	                 std::string name = "");

	/**
	 * Constructor for an empty StructuredEntity that is not yet connected.
	 *
	 * @param mgr        is the Manager instance.
	 * @param parent     is the parent Document of this StructuredEntity. Note
	 *                   that this StructuredEntity will automatically register
	 *                   itself as child of this Document.
	 * @param descriptor is the StructuredClass of this StructuredEntity.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   StructuredEntity. It is empty per default.
	 * @param name       is some name for this StructuredEntity that may be used
	 *                   for later reference. It is empty per default.
	 */
	StructuredEntity(Manager &mgr, Handle<Node> parent = nullptr,
	                 Handle<StructuredClass> descriptor = nullptr,
	                 Variant attributes = Variant::mapType{},
	                 std::string name = "");

	/**
	 * Returns true if and only if this element was created using transparency/
	 * if and only if this is an implicit element.
	 *
	 * @return true if and only if this element was created using transparency.
	 */
	bool isTransparent() const { return transparent; }

	/**
	 * @param trans true if and only if this element was created using
	 *              transparency/if and only if this is an implicit element.
	 */
	void setTransparent(bool trans) { transparent = trans; }
};

/**
 * This is a wrapper for primitive types (Variants) inside the document graph.
 * The most straightforward example for this is the actual document text, e.g.
 * inside a paragraph. In that case this would represent a mere string.
 */
class DocumentPrimitive : public StructureNode {
private:
	Variant content;

public:
	/**
	 * Constructor for a DocumentPrimitive.
	 *
	 * @param mgr       is the Manager instance.
	 * @param parent    is the parent DocumentEntity of this DocumentPrimitive
	 *                  in the DocumentTree. Note that this DocumentPrimitive
	 *                  will automatically register itself as child of this
	 *                  parent.
	 * @param content   is a Variant containing the content of this
	 *                  DocumentPrimitive. The Type of this Variant is
	 *                  specified at the parents Descriptor for the given
	 *                  fieldName.
	 * @param fieldName is the name of the field in the parent DocumentEntity
	 *                  where this DocumentPrimitive shall be added.
	 */
	DocumentPrimitive(Manager &mgr, Handle<Node> parent, Variant content,
	                  const std::string &fieldName = DEFAULT_FIELD_NAME)
	    : StructureNode(mgr, "", parent, fieldName), content(content)
	{
	}

	/**
	 * Constructor for a DocumentPrimitive.
	 *
	 * @param mgr       is the Manager instance.
	 * @param parent    is the parent DocumentEntity of this DocumentPrimitive
	 *                  in the DocumentTree. Note that this DocumentPrimitive
	 *                  will automatically register itself as child of this
	 *                  parent.
	 * @param content   is a Variant containing the content of this
	 *                  DocumentPrimitive. The Type of this Variant is
	 *                  specified at the parents Descriptor for the given
	 *                  fieldName.
	 * @param fieldIdx  is the index of the field in the parent DocumentEntity
	 *                  where this DocumentPrimitive shall be added.
	 */
	DocumentPrimitive(Manager &mgr, Handle<Node> parent, Variant content,
	                  size_t fieldIdx)
	    : StructureNode(mgr, "", parent, fieldIdx), content(content)
	{
	}

	/**
	 * Returns the content of this DocumentPrimitive as a const reference.
	 *
	 * @return the content of this DocumentPrimitive.
	 */
	const Variant &getContent() const { return content; }

	/**
	 * Returns the content of this DocumentPrimitive.
	 *
	 * @return the content of this DocumentPrimitive.
	 */
	Variant &getContent() { return content; }

	/**
	 * Sets the content of this DocumentPrimitive to the given Variant.
	 *
	 * @param c is the new content of this DocumentPrimitive.
	 */
	void setContent(const Variant &c)
	{
		invalidate();
		content = c;
	}
};

/**
 * An Anchor is an elementary StructureNode without any children that
 * marks a point in the text content of the document that can later be
 * referenced by an AnnotationEntity as it start and end point.
 * Please refer to the AnnotationEntity documentation for more information.
 */
class Anchor : public StructureNode {
private:
	Owned<AnnotationEntity> annotation;

protected:
	bool doValidate(Logger &logger) const override;

public:
	/**
	 * Constructor for Anchor.
	 *
	 * @param mgr       is the Manager instance.
	 * @param parent    is the parent of this Anchor in the Structure Tree (!),
	 *                  not the AnnotationEntity that references this Anchor.
	 *                  Note that this Anchor will automatically register itself
	 *                  as child of the given parent.
	 * @param fieldName is the name of the field in the parent DocumentEntity
	 *                  where this Anchor shall be added.
	 */
	Anchor(Manager &mgr, Handle<Node> parent,
	       const std::string &fieldName = DEFAULT_FIELD_NAME)
	    : StructureNode(mgr, "", parent, fieldName)
	{
	}

	/**
	 * Constructor for Anchor.
	 *
	 * @param mgr       is the Manager instance.
	 * @param parent    is the parent of this Anchor in the Structure Tree (!),
	 *                  not the AnnotationEntity that references this Anchor.
	 *                  Note that this Anchor will automatically register itself
	 *                  as child of the given parent.
	 * @param fieldIdx  is the index of the field in the parent DocumentEntity
	 *                  where this Anchor shall be added.
	 */
	Anchor(Manager &mgr, Handle<Node> parent, size_t fieldIdx)
	    : StructureNode(mgr, "", parent, fieldIdx)
	{
	}

	/**
	 * Returns the AnnotationEntity this Anchor belongs to.
	 *
	 * @return the AnnotationEntity this Anchor belongs to.
	 */
	Rooted<AnnotationEntity> getAnnotation() const { return annotation; }

	/**
	 * Sets the AnnotationEntity this Anchor belongs to. If this Anchor belonged
	 * to an AnnotationEntity before already, this reference is removed. This
	 * also sets the start/end reference of the new AnnotationEntity this Anchor
	 * shall belong to.
	 *
	 * @param anno  the new AnnotationEntity this Anchor shall belong to.
	 * @param start true if this Anchor should be added as start anchor, false
	 *              if it should be added as end Anchor.
	 */
	void setAnnotation(Handle<AnnotationEntity> anno, bool start);

	/**
	 * Returns true if and only if this Anchor is the start Anchor of the
	 * AnnotationEntity it belongs to. Note that this will return false also if
	 * no AnnotationEntity is set yet. So isStart() == false and isEnd() ==
	 * false is possible and occurs if and only if getAnnotation() == nullptr.
	 *
	 * @return true if and only if this Anchor is the start Anchor of the
	 *              AnnotationEntity it belongs to.
	 */
	bool isStart() const;

	/**
	 * Returns true if and only if this Anchor is the end Anchor of the
	 * AnnotationEntity it belongs to. Note that this will return false also if
	 * no AnnotationEntity is set yet. So isStart() == false and isEnd() ==
	 * false is possible and occurs if and only if getAnnotation() == nullptr.
	 *
	 * @return true if and only if this Anchor is the end Anchor of the
	 *              AnnotationEntity it belongs to.
	 */
	bool isEnd() const;
};

/**
 * An AnnotationEntity is a span-like instance that is not bound by the elements
 * of the Structure Tree. An annotation may very well overlap and cross the
 * limits of StructureEntities. A typical example for AnnotationEntities are
 * the markups "emphasized" and "strong". In HTML like markup languages these
 * concepts are handeled as structure elements, like this:
 *
 * \code{.xml}
 * <em>emphasized</em> <em><strong>and</strong></em> <strong>strong</strong>
 * \endcode
 *
 * which is neither intuitive nor semantically sound. Therefore we take the
 * approach of anchoring the Annotation entities in the text like this:
 *
 * \code{.xml}
 * <Anchor id=1/>emphasized <Anchor id=2/>and<Anchor id=3/> strong<Anchor id=4/>
 * <AnnotationEntity class="emphasized" start=1 end=3/>
 * <AnnotationEntity class="strong" start=2 end=4/>
 * \endcode
 *
 * Which signifies that indeed the text "emphasized and" is emphasized, not
 * the two text exerpts "emphasized" and "and" separately.
 *
 */
class AnnotationEntity : public DocumentNode, public DocumentEntity {
	friend DocumentEntity;
	friend Document;

private:
	Owned<Anchor> start;
	Owned<Anchor> end;

protected:
	void doResolve(ResolutionState &state) override;
	bool doValidate(Logger &logger) const override;

public:
	/**
	 * The constructor for an AnnotationEntity.
	 *
	 * @param mgr        is the Manager instance.
	 * @param parent     is the Document this AnnotationEntity is part of. The
	 *                   constructor will automatically register this
	 *                   AnnotationEntity at that document.
	 * @param descriptor is the AnnotationClass of this AnnotationEntity.
	 * @param start      is the start Anchor of this AnnotationEntity. It has to
	 *                   be part of the Document given as parent.
	 * @param end        is the end Anchor of this Annotationentity. It has to
	 *                   be part of the Document given as parent.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   AnnotationEntity. It is empty per default.
	 * @param name       is some name for this AnnotationEntity that might be
	 *                   used for references later on. It is empty per default.
	 */
	AnnotationEntity(Manager &mgr, Handle<Document> parent = nullptr,
	                 Handle<AnnotationClass> descriptor = nullptr,
	                 Handle<Anchor> start = nullptr,
	                 Handle<Anchor> end = nullptr,
	                 Variant attributes = Variant::mapType{},
	                 std::string name = "");

	/**
	 * Returns the start Anchor of this AnnotationEntity.
	 *
	 * @return the start Anchor of this AnnotationEntity.
	 */
	Rooted<Anchor> getStart() const { return start; }

	/**
	 * Returns the end Anchor of this AnnotationEntity.
	 *
	 * @return the end Anchor of this AnnotationEntity.
	 */
	Rooted<Anchor> getEnd() const { return end; }

	/**
	 * Sets the start Anchor of this AnnotationEntity.
	 *
	 * @param s is the new start Anchor for this AnnotationEntity.
	 */
	void setStart(Handle<Anchor> s);

	/**
	 * Sets the end Anchor of this AnnotationEntity.
	 *
	 * @param e is the new end Anchor for this AnnotationEntity.
	 */
	void setEnd(Handle<Anchor> e);
};

/**
 * A Document is mainly a wrapper for the Root structure node of the Document
 * Graph. It also references the ontologies that have been used within this
 * document and the AnnotationEntities that span over Anchors in this Document.
 */
class Document : public RootNode {
private:
	// TODO: Might there be several roots? E.g. metadata?
	Owned<StructuredEntity> root;
	NodeVector<AnnotationEntity> annotations;
	NodeVector<Ontology> ontologies;
	NodeVector<Typesystem> typesystems;

protected:
	void doResolve(ResolutionState &state) override;
	bool doValidate(Logger &logger) const override;
	void doReference(Handle<Node> node) override;
	RttiSet doGetReferenceTypes() const override;

public:
	/**
	 * This sets up an empty document.
	 *
	 * @param mgr  is the Manager instance.
	 * @param name is a name for this Document.
	 */
	Document(Manager &mgr, std::string name)
	    : RootNode(mgr, std::move(name), nullptr),
	      annotations(this),
	      ontologies(this),
	      typesystems(this)
	{
	}

	/**
	 * Sets the root StructuredEntity of this Document. This also sets the
	 * parent of the given StructuredEntity if it is not set to this Document
	 * already.
	 */
	void setRoot(Handle<StructuredEntity> root);

	/**
	 * Returns the root StructuredEntity of this Document.
	 *
	 * @return the root StructuredEntity of this Document.
	 */
	Rooted<StructuredEntity> getRoot() const { return root; }

	/**
	 * This creates a new StructuredEntity and adds it as root to this Document.
	 *
	 * @param descriptor is the StructuredClass of this StructuredEntity.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   StructuredEntity. It is empty per default.
	 * @param name       is some name for this StructuredEntity that may be used
	 *                   for later reference. It is empty per default.
	 *
	 * @return           the newly constructed StructuredEntity.
	 */
	Rooted<StructuredEntity> createRootStructuredEntity(
	    Handle<StructuredClass> descriptor,
	    Variant attributes = Variant::mapType{}, std::string name = "");

	/**
	 * Returns a const reference to the NodeVector of AnnotationEntities that
	 * span over Anchors in this Documents structure.
	 *
	 * @return a const reference to the NodeVector of AnnotationEntities that
	 *         span over Anchors in this Documents structure.
	 */
	const NodeVector<AnnotationEntity> &getAnnotations() const
	{
		return annotations;
	}

	/**
	 * Adds an AnnotationEntity to this Document. This also sets the parent
	 * of the given AnnotationEntity if it is not set to this Document already
	 * and removes it from the old Document.
	 *
	 * @param a is some AnnotationEntity
	 */
	void addAnnotation(Handle<AnnotationEntity> a);

	/**
	 * Adds multiple AnnotationEntities to this Document. This also sets the
	 * parent of each given AnnotationEntity if it is not set to this Document
	 * already and removes it from the old Document.
	 *
	 * @param as is a vector of AnnotationEntities.
	 */
	void addAnnotations(const std::vector<Handle<AnnotationEntity>> &as);

	/**
	 * Removes an AnnotationEntity from this Document. This also sets the parent
	 * of the given AnnotationEntity to null.
	 *
	 * @param a is some AnnotationEntity.
	 * @return  true if the given AnnotationEntity was removed and false if this
	 *          Document did not have the given AnnotationEntity as child.
	 */
	bool removeAnnotation(Handle<AnnotationEntity> a);
	/**
	 * Creates a new AnnotationEntity as child of this Document.
	 *
	 * @param descriptor is the AnnotationClass of this AnnotationEntity.
	 * @param start      is the start Anchor of this AnnotationEntity. It has to
	 *                   be part of this Document.
	 * @param end        is the end Anchor of this Annotationentity. It has to
	 *                   be part of this Document.
	 * @param attributes is a Map Variant containing attribute fillings for this
	 *                   AnnotationEntity. It is empty per default.
	 * @param name       is some name for this AnnotationEntity that might be
	 *                   used for references later on. It is empty per default.
	 *
	 * @return           the newly constructed AnnotationEntity.
	 */
	Rooted<AnnotationEntity> createChildAnnotation(
	    Handle<AnnotationClass> descriptor, Handle<Anchor> start,
	    Handle<Anchor> end, Variant attributes = Variant::mapType{},
	    std::string name = "");

	/**
	 * Returns a const reference to the NodeVector of ontologies that are used
	 * within this Document.
	 *
	 * @return a const reference to the NodeVector of ontologies that are used
	 * within this Document.
	 */
	const NodeVector<Ontology> &getOntologies() const { return ontologies; }

	/**
	 * Adds a Ontology reference to this Document.
	 */
	void referenceOntology(Handle<Ontology> d)
	{
		invalidate();
		ontologies.push_back(d);
	}

	/**
	 * Adds multiple Ontology references to this Document.
	 */
	void referenceOntologys(const std::vector<Handle<Ontology>> &d)
	{
		invalidate();
		ontologies.insert(ontologies.end(), d.begin(), d.end());
	}

	/**
	 * Returns a const reference to the NodeVector of referenced Typesystems.
	 *
	 * @return a const reference to the NodeVector of referenced Typesystems.
	 */
	const NodeVector<Typesystem> getTypesystems() const { return typesystems; }

	/**
	 * Adds a Typesystem reference to this Document.
	 */
	void referenceTypesystem(Handle<Typesystem> d)
	{
		invalidate();
		typesystems.push_back(d);
	}

	/**
	 * Adds multiple Typesystem references to this Document.
	 */
	void referenceTypesystems(const std::vector<Handle<Typesystem>> &d)
	{
		invalidate();
		typesystems.insert(typesystems.end(), d.begin(), d.end());
	}

	/**
	 * Returns true if and only if the given StructureNode is part of this
	 * document, meaning that there is a path of parent references in the
	 * Structure Tree leading from the given StructureNode to this Document.
	 *
	 * @param s is some StructureNode.
	 * @return  true if and only if the given StructureNode is part of this
	 *          document.
	 */
	bool hasChild(Handle<StructureNode> s) const;
};

namespace RttiTypes {
extern const Rtti Document;
extern const Rtti DocumentNode;
extern const Rtti DocumentEntity;
extern const Rtti AnnotationEntity;
extern const Rtti StructureNode;
extern const Rtti StructuredEntity;
extern const Rtti DocumentPrimitive;
extern const Rtti Anchor;
}
}

#endif /* _OUSIA_MODEL_DOCUMENT_HPP_ */
