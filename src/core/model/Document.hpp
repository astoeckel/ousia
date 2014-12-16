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
 * for example be a "book" node, if the respective document implements the
 * "book" domain. That root node in turn has structure nodes as children as well
 * as annotations that refer to the content of that structure node.
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
	std::vector<ManagedVector<StructuredEntity>> fields;

	Rooted<FieldDescriptor> getFieldDescriptor(const std::string &fieldName);

public:
	DocumentEntity(Manager &mgr, std::string name = "", Handle<Node> parent,
	               Handle<Descriptor> descriptor, Variant attributes)
	    : Node(mgr, std::move(name), parent),
	      descriptor(acquire(descriptor)),
	      attributes(std::move(attributes))
	{
		// TODO: Validation at construction time?
		// insert empty vectors for each field.
		for (int f = 0; f < descriptor->getFieldDescriptors.size(); f++) {
			fields.push_back(ManagedVector(this));
		}
	}

	Rooted<Descriptor> getDescriptor const() { return descriptor; }

	const Variant &getAttributes() const { return attributes; }

	Variant getAttributes const { return attributes; }

	/**
	 * This allows a direct manipulation of the internal data structure of a
	 * DocumentEntity and is not recommended. TODO: Delete this?
	 */
	std::vector<ManagedVector<StructuredEntity>> &getFields() { return fields; }

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
		return getFieldDescriptor(fieldName) != nullptr;
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
	 * FieldDescriptor matches the given name an empty ManagedVector is
	 * returned. This is also the case, however, if there are no members for an
	 * existing field. Therefore it is recommended to additionally check the
	 * output of "hasField" or use the overloaded version of this method with
	 * a FieldDescriptor as input.
	 *
	 * @param fieldName is the name of the field as specified in the
	 *                  FieldDescriptor in the Domain description.
	 * @return a ManagedVector of all StructuredEntities in that field. If the
	 *         field is unknown or if no members exist in that field yet, the
	 *         ManagedVector will be empty. Note that the ManagedVector is
	 *         returned as a reference, so it is possible to manipulate this
	 *         DocumentEntities content using this function.
	 */
	ManagedVector<StructuredEntity> &getField(const std::string &fieldName = "")
	{
		Rooted<FieldDescriptor> fd = getFieldDescriptor(fieldName);
		if (fd == nullptr) {
			return ManagedVector<StructuredEntity>(this);
		}
		return getField(fd);
	}

	/**
	 * This returns the vector of entities containing all members of the field
	 * with the given FieldDescriptor.
	 *
	 * If the FieldDescriptor does not belong to the Descriptor of this node
	 * an exception is thrown.
	 *
	 * @param fieldDescriptor is a FieldDescriptor defined in the Descriptor for
	 *                        this DocumentEntity.
	 * @return a ManagedVector of all StructuredEntities in that field.
	 */
	ManagedVector<StructuredEntity> &getField(
	    Rooted<FieldDescriptor> fieldDescriptor);
};
}
}

#endif /* _OUSIA_MODEL_DOCUMENT_HPP_ */

