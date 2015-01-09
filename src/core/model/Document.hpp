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
 * Consider this XML representation of a document using the "book" domain:
 *
 * <doc>
 * 	<head>
 * 		<import rel="domain" src="book_domain.oxm"/>
 * 		<import rel="domain" src="emphasized_domain.oxm"/>
 * 		<alias tag="paragraph" aka="p"/>
 * 	</head>
 * 	<book>
 * 		This might be some introductory text or a dedication. Ideally, of
 * 		course, such elements would be semantically specified as such in
 * 		additional domains (or in this one).
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
 *
 * As can be seen the StructureEntities inherently follow a tree structure that
 * is restricted by the implicit context free grammar of the "book" Domain
 * definition (e.g. it is not allowed to have a "book" node inside a "section";
 * refer to te Domain.hpp for more information).
 *
 * Another interesting fact is the special place of AnnotationEntities: They are
 * Defined by start and end Anchors in the text. Note that this allows for
 * overlapping annotations and provides a more intuitive (and semantically
 * sound) handling of such span-like concepts. So the
 *
 * <em>content</em>
 *
 * is implicitly expanded to:
 *
 * <a id="1"/>content<a id="2"/>
 * <emphasized start="1" end="2"/>
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
 * <paragraph>
 * 	<text>
 * 		Here we might find the actual section content.
 * 	</text>
 * </paragraph>
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_DOCUMENT_HPP_
#define _OUSIA_MODEL_DOCUMENT_HPP_

#include <core/managed/ManagedContainer.hpp>
#include <core/common/Variant.hpp>

#include "Node.hpp"
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
	 * with the given name.  If an empty name is given it is assumed that the
	 * 'default' FieldDescriptor is referenced, where 'default' means either:
	 * 1.) The only TREE typed FieldDescriptor (if present) or
	 * 2.) the only FieldDescriptor (if only one is specified).
	 *
	 * If the name is unknown an exception is thrown.
	 *
	 * @param fieldName is the name of a field as specified in the
	 *                  FieldDescriptor in the Domain description.
	 * @return a NodeVector of all StructuredEntities in that field.
	 */
	NodeVector<StructuredEntity> &getField(const std::string &fieldName = "");

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
	    Handle<FieldDescriptor> fieldDescriptor);
};

/**
 * A StructuredEntity is a node in the Structure Tree of a document. For more
 * information please refer to the header documentation above.
 */
class StructuredEntity : public DocumentEntity {
public:
	StructuredEntity(Manager &mgr, Handle<Node> parent,
	                 Handle<StructuredClass> descriptor, Variant attributes,
	                 std::string name = "")
	    : DocumentEntity(mgr, parent, descriptor, std::move(attributes),
	                     std::move(name))
	{
	}

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
	    Handle<DocumentEntity> parent, Variant content,
	    const std::string &fieldName = "");
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
		 * @param parent is the parent of this Anchor in the Structure Tree (!),
		 *               not the AnnotationEntity that references this Anchor.
		 * @param name   is the Anchor id.
		 */
		Anchor(Manager &mgr, Handle<DocumentEntity> parent, std::string name)
		    : StructuredEntity(mgr, parent, nullptr, Variant(), std::move(name))
		{
		}
	};

private:
	Owned<Anchor> start;
	Owned<Anchor> end;

public:
	AnnotationEntity(Manager &mgr, Handle<Node> parent,
	                 Handle<AnnotationClass> descriptor, Variant attributes,
	                 Handle<Anchor> start, Handle<Anchor> end,
	                 std::string name = "")
	    : DocumentEntity(mgr, parent, descriptor, attributes, std::move(name)),
	      start(acquire(start)),
	      end(acquire(end))
	{
	}

	Rooted<Anchor> getStart() { return start; }

	Rooted<Anchor> getEnd() { return end; }

	/**
	 * This builds an Anchor as child of the given DocumentEntity. It
	 * automatically appends the newly build Anchor to its parent.
	 *
	 * @param parent     is the parent DocumentEntity. The newly constructed
	 *                   Anchor will automatically be appended to it.
	 * @param id         is the id of this Anchor.
	 * @param fieldName  is the name of the field where the newly constructed
	 *                   Anchor shall be appended.
	 *
	 * @return           the newly created Anchor or a nullptr if some
	 *                   input handle was empty.
	 */
	static Rooted<Anchor> buildAnchor(Handle<DocumentEntity> parent,
	                                  std::string id,
	                                  const std::string &fieldName = "");
	/**
	 * This builds an AnnotationEntity as child of the given DocumentEntity. It
	 * automatically appends the newly build entity to its parent.
	 *
	 * @param parent     is the document the newly constructed AnnotationEntity
	 *                   will be appended to.
	 * @param domains    are the domains that are used to find the
	 *                   AnnotationClass for the new node. The domains will be
	 *                   searched in the given order.
	 * @param className  is the name of the AnnotationClass.
	 * @param attributes are the attributes of the new node in terms of a Struct
	 *                   variant (empty per default).
	 * @param name       is the name of this AnnotationEntity (empty per
	 *                   default).
	 * @return           the newly created AnnotationEntity or a nullptr if some
	 *                   input handle was empty or the given domains did not
	 *                   contain a AnnotationClass with the given name.
	 */
	static Rooted<AnnotationEntity> buildEntity(Handle<Document> parent, std::vector<Handle<Domain>> domains,
	    const std::string &className,
	    Handle<Anchor> start, Handle<Anchor> end,
	    Variant attributes = Variant(), std::string name = "");
};

/**
 * A Document is mainly a wrapper for the Root structure node of the Document
 * Graph.
 */
class Document : public Node {
private:
	// TODO: Might there be several roots? E.g. metadata?
	Owned<StructuredEntity> root;
	NodeVector<AnnotationEntity> annotations;
	NodeVector<Domain> domains;

	void continueResolve(ResolutionState &state) override;

public:
	Document(Manager &mgr, std::string name)
	    // TODO: Can a document have a parent?
	    : Node(mgr, std::move(name), nullptr),
	      annotations(this)
	{
	}

	void setRoot(Handle<StructuredEntity> root) { this->root = acquire(root); };

	Rooted<StructuredEntity> getRoot() const { return root; }

	NodeVector<AnnotationEntity> &getAnnotations() { return annotations; }

	const NodeVector<Domain> &getDomains() const { return domains; }

	void addDomain(Handle<Domain> d) { domains.push_back(d); }

	void addDomains(const std::vector<Handle<Domain>> d)
	{
		domains.insert(domains.end(), d.begin(), d.end());
	}
};
}

namespace RttiTypes {
extern const Rtti<model::Document> Document;
extern const Rtti<model::DocumentEntity> DocumentEntity;
extern const Rtti<model::AnnotationEntity> AnnotationEntity;
extern const Rtti<model::StructuredEntity> StructuredEntity;
extern const Rtti<model::DocumentPrimitive> DocumentPrimitive;
extern const Rtti<model::AnnotationEntity::Anchor> Anchor;
}
}

#endif /* _OUSIA_MODEL_DOCUMENT_HPP_ */

