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
 * specification in the respective Domain(s) (see also the Domain.hpp).
 *
 * A Document, from top to bottom, consists of "Document" instance,
 * which "owns" the structural root node of the in-document graph. This might
 * for example be a "book" node of the "book" domain. That root node in turn has
 * structure nodes as children, which in turn may have children. This
 * constitutes a Structure Tree. Additionally annotations may be attached to
 * Structure Nodes, effectively resulting in a Document Graph instead of a
 * Document Tree (other references may introduce cycles as well).
 *
 * Consider this simplified XML representation of a document (TODO: Use
 * non-simplified XML as soon as possible):
 *
 * <Document implements="book">
 *   <StructureEntity class="book">
 *     <StructureEntity class="section">
 *       <DocumentPrimitive>
 *         This is some text with some <Anchor id="1"/>emphasized and
 *         <Anchor id="2"/>strong<Anchor id="3"/> text.
 *       </DocumentPrimitive>
 *       <AnnotationEntity class="emphasized" start="1", end="3"/>
 *       <AnnotationEntity class="strong" start="2", end="3"/>
 *     </StructureEntity>
 *   </StructureEntity>
 * </Document>
 *
 * As can be seen the StructureEntities inherently follow a tree structure that
 * is restricted by the implicit context free grammar of the "book" Domain
 * definition (e.g. it is not allowed to have a "book" node inside a "section";
 * refer to te Domain.hpp for more information).
 *
 * Another interesting fact is the special place of AnnotationEntities: They are
 * Defined by start and end Anchors in the text. Note that this allows for
 * overlapping annotations and provides a more intuitive (and semantically
 * sound) handling of such span-like concepts.
 * Note that the place of an AnnotationEntity within the XML above is not
 * strictly defined. It might as well be placed as a child of the "book" node.
 * In general it is recommended to use the lowest possible place in the
 * StructureTree to include the AnnotationEntity for better readability.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_DOCUMENT_HPP_
#define _OUSIA_MODEL_DOCUMENT_HPP_

#include <core/managed/ManagedContainer.hpp>
#include <core/Node.hpp>
#include <core/common/Variant.hpp>

#include "Domain.hpp"
#include "Typesystem.hpp"

namespace ousia {
namespace model {

class StructuredEntity;
class AnnotationEntity;
class Document;

/**
 * A DocumentEntity is the common superclass for StructuredEntities and
 * AnnotationEntities. Similarly to DescriptorEntity in the Domain.hpp it
 * defines that each node in the Document graph may have attributes (in form
 * of a struct Variant), and fields.
 * The fields here are a vector of vectors. The first vector implements all
 * fields while the inner vector contains all children in this field.
 * We provide, however, convenience functions for better access via the field
 * name.
 *
 */
class DocumentEntity : public Node {
private:
	Owned<Descriptor> descriptor;
	const Variant attributes;
	std::vector<NodeVector<StructuredEntity>> fields;

	int getFieldDescriptorIndex(const std::string &fieldName);

public:
	DocumentEntity(Manager &mgr, Handle<Node> parent,
	               Handle<Descriptor> descriptor, Variant attributes,
	               std::string name = "")
	    : Node(mgr, std::move(name), parent),
	      descriptor(acquire(descriptor)),
	      attributes(std::move(attributes))
	{
		// TODO: Validation at construction time?
		// insert empty vectors for each field.
		if (!descriptor.isNull()) {
			for (size_t f = 0; f < descriptor->getFieldDescriptors().size();
			     f++) {
				fields.push_back(NodeVector<StructuredEntity>(this));
			}
		}
	}

	Rooted<Descriptor> getDescriptor() const { return descriptor; }

	Variant getAttributes() const { return attributes; }

	/**
	 * This allows a direct manipulation of the internal data structure of a
	 * DocumentEntity and is not recommended. TODO: Delete this?
	 */
	std::vector<NodeVector<StructuredEntity>> &getFields() { return fields; }

	/**
	 * This returns true if there is a FieldDescriptor in the Descriptor for
	 * this DocumentEntity which has the given name. If an empty name is
	 * given it is assumed that the 'default' FieldDescriptor is referenced,
	 * where 'default' means either:
	 * 1.) The only TREE typed FieldDescriptor (if present) or
	 * 2.) the only FieldDescriptor (if only one is specified).
	 *
	 * @param fieldName is the name of a field as specified in the
	 *                  FieldDescriptor in the Domain description.
	 * @return true if this FieldDescriptor exists.
	 */
	bool hasField(const std::string &fieldName = "")
	{
		return getFieldDescriptorIndex(fieldName) != -1;
	}

	/**
	 * This returns the vector of entities containing all members of the field
	 * for which the FieldDescriptor has the specified name. If an empty name is
	 * given it is assumed that the 'default' FieldDescriptor is referenced,
	 * where 'default' means either:
	 * 1.) The only TREE typed FieldDescriptor (if present) or
	 * 2.) the only FieldDescriptor (if only one is specified).
	 *
	 * Note that the output of this method might well be ambigous: If no
	 * FieldDescriptor matches the given name an empty NodeVector is
	 * returned. This is also the case, however, if there are no members for an
	 * existing field. Therefore it is recommended to additionally check the
	 * output of "hasField" or use the version of this method with
	 * a FieldDescriptor as input.
	 *
	 * @param fieldName is the name of the field as specified in the
	 *                  FieldDescriptor in the Domain description.
	 * @param res       is a NodeVector reference where the result will be
	 *                  stored. After using this method the reference will
	 *                  either refer to all StructuredEntities in that field. If
	 *                  the field is unknown or if no members exist in that
	 *                  field yet, the NodeVector will be empty.
	 */
	void getField(NodeVector<StructuredEntity> &res,
	              const std::string &fieldName = "");

	/**
	 * This returns the vector of entities containing all members of the field
	 * with the given FieldDescriptor.
	 *
	 * If the FieldDescriptor does not belong to the Descriptor of this node
	 * an exception is thrown.
	 *
	 * @param fieldDescriptor is a FieldDescriptor defined in the Descriptor for
	 *                        this DocumentEntity.
	 * @return a NodeVector of all StructuredEntities in that field.
	 */
	NodeVector<StructuredEntity> &getField(
	    Rooted<FieldDescriptor> fieldDescriptor);
};

/**
 * A StructuredEntity is a node in the Structure Tree of a document. For more
 * information please refer to the header documentation above.
 */
class StructuredEntity : public DocumentEntity {
private:
	NodeVector<AnnotationEntity> annotations;

public:
	StructuredEntity(Manager &mgr, Handle<Node> parent,
	                 Handle<StructuredClass> descriptor, Variant attributes,
	                 std::string name = "")
	    : DocumentEntity(mgr, parent, descriptor, std::move(attributes),
	                     std::move(name)),
	      annotations(this)
	{
	}

	NodeVector<AnnotationEntity> &getAnnotations() { return annotations; }

	/**
	 * This builds the root StructuredEntity for the given document. It
	 * automatically appends the newly build entity to the given document.
	 *
	 * @param document   is the document this entity shall be build for. The
	 *                   resulting entity will automatically be appended to that
	 *                   document. Also the manager of that document will be
	 *                   used to register the new node.
	 * @param domains    are the domains that are used to find the
	 *                   StructuredClass for the new node. The domains will be
	 *                   searched in the given order.
	 * @param className  is the name of the StructuredClass.
	 * @param attributes are the attributes of the new node in terms of a Struct
	 *                   variant (empty per default).
	 * @param name       is the name of this StructuredEntity (empty per
	 *                   default).
	 * @return           the newly created StructuredEntity or a nullptr if some
	 *                   input handle was empty or the given domains did not
	 *                   contain a StructuredClass with the given name.
	 */
	static Rooted<StructuredEntity> buildRootEntity(
	    Handle<Document> document, std::vector<Handle<Domain>> domains,
	    const std::string &className, Variant attributes = Variant(),
	    std::string name = "");

	/**
	 * This builds a StructuredEntity as child of the given DocumentEntity. It
	 * automatically appends the newly build entity to its parent.
	 *
	 * @param parent     is the parent DocumentEntity. The newly constructed
	 *                   StructuredEntity will automatically be appended to it.
	 * @param domains    are the domains that are used to find the
	 *                   StructuredClass for the new node. The domains will be
	 *                   searched in the given order.
	 * @param className  is the name of the StructuredClass.
	 * @param fieldName  is the name of the field where the newly constructed
	 *                   StructuredEntity shall be appended.
	 * @param attributes are the attributes of the new node in terms of a Struct
	 *                   variant (empty per default).
	 * @param name       is the name of this StructuredEntity (empty per
	 *                   default).
	 *
	 * @return           the newly created StructuredEntity or a nullptr if some
	 *                   input handle was empty or the given domains did not
	 *                   contain a StructuredClass with the given name.
	 */
	static Rooted<StructuredEntity> buildEntity(
	    Handle<DocumentEntity> parent, std::vector<Handle<Domain>> domains,
	    const std::string &className, const std::string &fieldName = "",
	    Variant attributes = Variant(), std::string name = "");
};

/**
 * This is a wrapper for primitive types (Variants) inside the document graph.
 * The most straightforward example for this is the actual document text, e.g.
 * inside a paragraph. In that case this would represent a mere string.
 */
class DocumentPrimitive : public StructuredEntity {
public:
	DocumentPrimitive(Manager &mgr, Handle<DocumentEntity> parent,
	                  Variant content)
	    : StructuredEntity(mgr, parent, nullptr, std::move(content))
	{
	}

	Variant getContent() const { return getAttributes(); }

	// TODO: Override such methods like "getField" to disable them?

	/**
	 * This builds a DocumentPrimitive as child of the given DocumentEntity. It
	 * automatically appends the newly build entity to its parent.
	 *
	 * @param parent     is the parent DocumentEntity. The newly constructed
	 *                   DocumentPrimitive will automatically be appended to it.
	 * @param content    is the primitive content of the new node in terms of a
	 *                   Struct variant.
	 * @param fieldName  is the name of the field where the newly constructed
	 *                   StructuredEntity shall be appended.
	 *
	 * @return           the newly created StructuredEntity or a nullptr if some
	 *                   input handle was empty or the given domains did not
	 *                   contain a StructuredClass with the given name.
	 */
	static Rooted<DocumentPrimitive> buildEntity(
	    Handle<DocumentEntity> parent, 
	    Variant content, const std::string &fieldName = "");
};

/**
 * An AnnotationEntity is a span-like instance that is not bound by the elements
 * of the Structure Tree. An annotation may very well overlap and cross the
 * limits of StructureEntities. A typical example for AnnotationEntities are
 * the markups "emphasized" and "strong". In HTML like markup languages these
 * concepts are handeled as structure elements, like this:
 *
 * <em>emphasized</em> <em><strong>and</strong></em> <strong>strong</strong>
 *
 * which is neither intuitive nor semantically sound. Therefore we take the
 * approach of anchoring the Annotation entities in the text like this:
 *
 * <Anchor id=1/>emphasized <Anchor id=2/>and<Anchor id=3/> strong<Anchor id=4/>
 * <AnnotationEntity class="emphasized" start=1 end=3/>
 * <AnnotationEntity class="strong" start=2 end=4/>
 *
 * Which signifies that indeed the text "emphasized and" is emphasized, not
 * the two text exerpts "emphasized" and "and" separately.
 *
 */
class AnnotationEntity : public DocumentEntity {
public:
	/**
	 * An Anchor is an elementary StructuredEntity without any children that
	 * marks a point in the text content of the document that can later be
	 * referenced by an AnnotationEntity as it start and end point.
	 * Please refer to the AnnotationEntity documentation for more information.
	 */
	class Anchor : public StructuredEntity {
	public:
		/**
		 * @param mgr    is the Manager instance.
		 * @param name   is the Anchor id.
		 * @param parent is the parent of this Anchor in the Structure Tree (!),
		 *               not the AnnotationEntity that references this Anchor.
		 */
		Anchor(Manager &mgr, Handle<StructuredEntity> parent,
		       std::string name = "")
		    : StructuredEntity(mgr, parent, nullptr, Variant(), std::move(name))
		{
		}
	};

private:
	Owned<Anchor> start;
	Owned<Anchor> end;

public:
	AnnotationEntity(Manager &mgr, Handle<Node> parent,
	                 Handle<StructuredClass> descriptor, Variant attributes,
	                 Handle<Anchor> start, Handle<Anchor> end,
	                 std::string name = "")
	    : DocumentEntity(mgr, parent, descriptor, attributes, std::move(name)),
	      start(acquire(start)),
	      end(acquire(end))
	{
	}

	Rooted<Anchor> getStart() { return start; }

	Rooted<Anchor> getEnd() { return end; }
};

/**
 * A Document is mainly a wrapper for the Root structure node of the Document
 * Graph.
 */
class Document : public Node {
private:
	Owned<StructuredEntity> root;

public:
	Document(Manager &mgr, std::string name)
	    // TODO: Can a document have a parent?
	    : Node(mgr, std::move(name), nullptr)
	{
	}

	void setRoot(Handle<StructuredEntity> root) { root = acquire(root); };

	Rooted<StructuredEntity> getRoot() const { return root; }
};
}
}

#endif /* _OUSIA_MODEL_DOCUMENT_HPP_ */

