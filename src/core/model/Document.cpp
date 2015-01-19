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

#include "Document.hpp"

#include <map>
#include <set>

#include <core/common/Exceptions.hpp>
#include <core/common/RttiBuilder.hpp>

namespace ousia {
namespace model {

/* Class DocumentEntity */

int DocumentEntity::getFieldDescriptorIndex(const std::string &fieldName,
                                            bool enforce) const
{
	const NodeVector<FieldDescriptor> &fds = descriptor->getFieldDescriptors();
	unsigned int f = 0;

	// look if we have an empty name.
	if (fieldName == "") {
		// in that case we look for a default field.
		// First: Do we only have one field?
		if (fds.size() == 1) {
			// if so we return that one.
			return f;
		}
		// Second: Do we have a TREE field?
		for (auto &fd : fds) {
			if (fd->getFieldType() == FieldDescriptor::FieldType::TREE) {
				return f;
			}
			f++;
		}
	} else {
		// otherwise we return the FieldDescriptor with the correct name (if
		// such a descriptor exists).
		for (auto &fd : fds) {
			if (fd->getName() == fieldName) {
				return f;
			}
			f++;
		}
	}
	if (enforce) {
		throw OusiaException(std::string("\"") + descriptor->getName() +
		                     "\" has no field with name \"" + fieldName + "\"");
	} else {
		return -1;
	}
}

int DocumentEntity::getFieldDescriptorIndex(
    Handle<FieldDescriptor> fieldDescriptor, bool enforce) const
{
	if (fieldDescriptor.isNull()) {
		throw OusiaException("The given FieldDescriptor handle is null!");
	}
	const NodeVector<FieldDescriptor> &fds = descriptor->getFieldDescriptors();
	int f = 0;
	for (auto &fd : fds) {
		if (fd->getName() == fieldDescriptor->getName() &&
		    fd->getFieldType() == fieldDescriptor->getFieldType()) {
			return f;
		}
		f++;
	}
	if (enforce) {
		throw OusiaException(std::string("\"") + descriptor->getName() +
		                     "\" has no field with name \"" +
		                     fieldDescriptor->getName() + "\"");
	} else {
		return -1;
	}
}

void DocumentEntity::invalidateSubInstance()
{
	if (subInst->isa(RttiTypes::StructuredEntity)) {
		subInst.cast<StructuredEntity>()->invalidate();
	} else {
		subInst.cast<AnnotationEntity>()->invalidate();
	}
}

DocumentEntity::DocumentEntity(Handle<Node> subInst,
                               Handle<Descriptor> descriptor,
                               Variant attributes)
    : subInst(subInst),
      descriptor(subInst->acquire(descriptor)),
      attributes(std::move(attributes))
{
	// insert empty vectors for each field.
	if (!descriptor.isNull()) {
		NodeVector<FieldDescriptor> fieldDescs;
		if (descriptor->isa(RttiTypes::StructuredClass)) {
			fieldDescs = descriptor.cast<StructuredClass>()
			                 ->getEffectiveFieldDescriptors();
		} else {
			fieldDescs = descriptor->getFieldDescriptors();
		}
		for (size_t f = 0; f < fieldDescs.size(); f++) {
			fields.push_back(NodeVector<StructureNode>(subInst));
		}
	}
}

bool DocumentEntity::doValidate(Logger &logger) const
{
	// if we have no descriptor, this is invalid.
	if (descriptor == nullptr) {
		logger.error("This entity has no descriptor!");
		// in this case we have to stop the validation process, because without
		// a constructor we can not check anything else.
		return false;
	}
	// TODO: check the validated form of Attributes
	// TODO: Check if descriptor is registered at the Document?

	bool valid = true;
	/*
	 * generate the set of effective fields. This is trivial for
	 * AnnotationEntities, but in the case of StructuredEntities we have to
	 * gather all fields of superclasses as well, that have not been
	 * overridden in the subclasses.
	 */
	NodeVector<FieldDescriptor> fieldDescs;
	if (descriptor->isa(RttiTypes::StructuredClass)) {
		fieldDescs =
		    descriptor.cast<StructuredClass>()->getEffectiveFieldDescriptors();
	} else {
		fieldDescs = descriptor->getFieldDescriptors();
	}
	// iterate over every field
	for (unsigned int f = 0; f < fields.size(); f++) {
		// we have a special check for primitive fields.
		if (fieldDescs[f]->getFieldType() ==
		    FieldDescriptor::FieldType::PRIMITIVE) {
			switch (fields[f].size()) {
				case 0:
					if (!fieldDescs[f]->optional) {
						logger.error(std::string("Primitive Field \"") +
						             fieldDescs[f]->getName() +
						             "\" had no content!");
						valid = false;
					}
					continue;
				case 1:
					break;
				default:
					logger.error(std::string("Primitive Field \"") +
					             fieldDescs[f]->getName() +
					             "\" had more than one child!");
					valid = false;
					continue;
			}
			// if we are here we know that exactly one child exists.
			// TODO: Check the primitive type of the child.
			continue;
		}

		// we can do a faster check if this field is empty.
		if (fields[f].size() == 0) {
			// if this field is optional, an empty field is valid anyways.
			if (fieldDescs[f]->optional) {
				continue;
			}
			/*
			 * if it is not optional we have to check if zero is a valid
			 * cardinality.
			 */
			for (auto &ac : fieldDescs[f]->getChildren()) {
				const size_t min = ac->getCardinality().min();
				if (min > 0) {
					logger.error(
					    std::string("Field \"") + fieldDescs[f]->getName() +
					    "\" was empty but needs at least " +
					    std::to_string(min) + " elements of class \"" +
					    ac->getName() + "\" according to the definition of \"" +
					    descriptor->getName() + "\"");
					valid = false;
				}
			}
			continue;
		}

		// create a set of allowed classes identified by their unique id.
		std::set<ManagedUid> accs;
		for (auto &ac : fieldDescs[f]->getChildren()) {
			accs.insert(ac->getUid());
		}
		// store the actual numbers of children for each child class in a map
		std::map<ManagedUid, unsigned int> nums;

		// iterate over every actual child of this DocumentEntity
		for (auto &rc : fields[f]) {
			if (rc->isa(RttiTypes::Anchor)) {
				// Anchors are uninteresting and can be ignored.
				continue;
			}
			if (rc->isa(RttiTypes::DocumentPrimitive)) {
				logger.error(std::string("Non-primitive Field \"") +
				             fieldDescs[f]->getName() +
				             "\" had primitive content!");
				valid = false;
				continue;
			}
			// otherwise this is a StructuredEntity
			Handle<StructuredEntity> c = rc.cast<StructuredEntity>();

			ManagedUid id = c->getDescriptor()->getUid();
			// check if its class is allowed.
			bool allowed = accs.find(id) != accs.end();
			/*
			 * if it is not allowed directly, we have to check if the class is a
			 * child of a permitted class.
			 */
			if (!allowed) {
				for (auto &ac : fieldDescs[f]->getChildren()) {
					if (c->getDescriptor()
					        .cast<StructuredClass>()
					        ->isSubclassOf(ac)) {
						allowed = true;
						id = ac->getUid();
					}
				}
			}
			if (!allowed) {
				logger.error(std::string("An instance of \"") +
				             c->getDescriptor()->getName() +
				             "\" is not allowed as child of an instance of \"" +
				             descriptor->getName() + "\" in field \"" +
				             fieldDescs[f]->getName() + "\"");
				valid = false;
				continue;
			}
			// note the number of occurences.
			const auto &n = nums.find(id);
			if (n != nums.end()) {
				n->second++;
			} else {
				nums.emplace(id, 1);
			}
		}

		// now check if the cardinalities are right.
		for (auto &ac : fieldDescs[f]->getChildren()) {
			const auto &n = nums.find(ac->getUid());
			unsigned int num = 0;
			if (n != nums.end()) {
				num = n->second;
			}
			if (!ac->getCardinality().contains(num)) {
				logger.error(
				    std::string("Field \"") + fieldDescs[f]->getName() +
				    "\" had " + std::to_string(num) + " elements of class \"" +
				    ac->getName() +
				    "\", which is invalid according to the definition of \"" +
				    descriptor->getName() + "\"");
				valid = false;
				continue;
			}
		}
	}

	// go into recursion.
	for (auto &f : fields) {
		for (auto &n : f) {
			valid = valid & n->validate(logger);
		}
	}
	return valid;
}

void DocumentEntity::setAttributes(const Variant &a)
{
	invalidateSubInstance();
	attributes = a;
}

void DocumentEntity::addStructureNode(Handle<StructureNode> s,
                                      const std::string &fieldName)
{
	invalidateSubInstance();
	fields[getFieldDescriptorIndex(fieldName, true)].push_back(s);
}

void DocumentEntity::addStructureNodes(
    const std::vector<Handle<StructureNode>> &ss, const std::string &fieldName)
{
	invalidateSubInstance();
	NodeVector<StructureNode> &field =
	    fields[getFieldDescriptorIndex(fieldName, true)];
	field.insert(field.end(), ss.begin(), ss.end());
}

void DocumentEntity::addStructureNode(Handle<StructureNode> s,
                                      Handle<FieldDescriptor> fieldDescriptor)
{
	invalidateSubInstance();
	fields[getFieldDescriptorIndex(fieldDescriptor, true)].push_back(s);
}

void DocumentEntity::addStructureNodes(
    const std::vector<Handle<StructureNode>> &ss,
    Handle<FieldDescriptor> fieldDescriptor)
{
	invalidateSubInstance();
	NodeVector<StructureNode> &field =
	    fields[getFieldDescriptorIndex(fieldDescriptor, true)];
	field.insert(field.end(), ss.begin(), ss.end());
}

/* Class StructureNode */

StructureNode::StructureNode(Manager &mgr, std::string name,
                             Handle<Node> parent, const std::string &fieldName)
    : Node(mgr, std::move(name), parent)
{
	if (parent->isa(RttiTypes::StructuredEntity)) {
		parent.cast<StructuredEntity>()->addStructureNode(this, fieldName);
	} else if (parent->isa(RttiTypes::AnnotationEntity)) {
		parent.cast<AnnotationEntity>()->addStructureNode(this, fieldName);
	} else {
		throw OusiaException("The proposed parent was no DocumentEntity!");
	}
}

/* Class StructuredEntity */

StructuredEntity::StructuredEntity(Manager &mgr, Handle<Document> doc,
                                   Handle<StructuredClass> descriptor,
                                   Variant attributes, std::string name)
    : StructureNode(mgr, std::move(name), doc),
      DocumentEntity(this, descriptor, std::move(attributes))
{
	doc->setRoot(this);
}

bool StructuredEntity::doValidate(Logger &logger) const
{
	bool valid = true;
	// check if the parent is set.
	if (getParent() == nullptr) {
		logger.error("The parent is not set!");
		valid = false;
	}
	// check name
	if (!getName().empty()) {
		valid = valid & validateName(logger);
	}
	// check the validity as a DocumentEntity.
	return valid & DocumentEntity::doValidate(logger);
}

/* Class AnnotationEntity */

AnnotationEntity::AnnotationEntity(Manager &mgr, Handle<Document> parent,
                                   Handle<AnnotationClass> descriptor,
                                   Handle<Anchor> start, Handle<Anchor> end,
                                   Variant attributes, std::string name)
    : Node(mgr, std::move(name), parent),
      DocumentEntity(this, descriptor, attributes),
      start(acquire(start)),
      end(acquire(end))
{
	parent->annotations.push_back(this);
	parent->invalidate();
}

bool AnnotationEntity::doValidate(Logger &logger) const
{
	bool valid = true;
	// check if this AnnotationEntity is correctly registered at its document.
	if (getParent() == nullptr) {
		logger.error("The parent is not set!");
		valid = false;
	} else if (!getParent()->isa(RttiTypes::Document)) {
		logger.error("The parent is not a document!");
		valid = false;
	}
	Handle<Document> doc = getParent().cast<Document>();
	bool found = false;
	for (auto &a : doc->getAnnotations()) {
		if (a == this) {
			found = true;
			break;
		}
	}
	if (!found) {
		logger.error("This annotation was not registered at the document.");
		valid = false;
	}
	// check name
	if (!getName().empty()) {
		valid = valid & validateName(logger);
	}
	// check if the Anchors are part of the right document.
	if (start == nullptr) {
		logger.error("This annotation has no start Anchor!");
		valid = false;
	} else if (!doc->hasChild(start)) {
		logger.error(
		    "This annotations start anchor was not part of the same document!");
		valid = false;
	}
	if (end == nullptr) {
		logger.error("This annotation has no end Anchor!");
		valid = false;
	} else if (!doc->hasChild(end)) {
		logger.error(
		    "This annotations end anchor was not part of the same document!");
		valid = false;
	}
	// check the validity as a DocumentEntity.
	return valid & DocumentEntity::doValidate(logger);
}

/* Class Document */

void Document::doResolve(ResolutionState &state)
{
	continueResolveComposita(annotations, annotations.getIndex(), state);
	if (root != nullptr) {
		continueResolveCompositum(root, state);
	}
	continueResolveReferences(domains, state);
}

bool Document::doValidate(Logger &logger) const
{
	// An empty document is always invalid. TODO: Is this a smart choice?
	bool valid = true;
	if (root == nullptr) {
		valid = false;
	} else {
		// check if the root is allowed to be a root.
		if (!root->getDescriptor().cast<StructuredClass>()->root) {
			logger.error(std::string("A node of type \"") +
			             root->getDescriptor()->getName() +
			             "\" is not allowed to be the Document root!");
			valid = false;
		}
		// then call validate on the root
		valid = valid & root->validate(logger);
	}
	// call validate on the AnnotationEntities
	return valid & continueValidation(annotations, logger);
}

bool Document::hasChild(Handle<StructureNode> s) const
{
	Rooted<Managed> parent = s->getParent();
	if (parent->isa(RttiTypes::StructureNode)) {
		return hasChild(parent.cast<StructureNode>());
	} else if (parent->isa(RttiTypes::AnnotationEntity)) {
		Handle<AnnotationEntity> a = parent.cast<AnnotationEntity>();
		return this == a->getParent();
	} else if (parent->isa(RttiTypes::Document)) {
		return this == parent;
	}
	return false;
}
}

/* Type registrations */
namespace RttiTypes {
const Rtti Document = RttiBuilder<model::Document>("Document")
                          .parent(&Node)
                          .composedOf({&AnnotationEntity, &StructuredEntity});
const Rtti StructureNode =
    RttiBuilder<model::StructureNode>("StructureNode").parent(&Node);
const Rtti StructuredEntity =
    RttiBuilder<model::StructuredEntity>("StructuredEntity")
        .parent(&StructureNode)
        .composedOf({&StructuredEntity, &DocumentPrimitive, &Anchor});
const Rtti DocumentPrimitive = RttiBuilder<model::DocumentPrimitive>(
                                   "DocumentPrimitive").parent(&StructureNode);
const Rtti Anchor = RttiBuilder<model::Anchor>("Anchor").parent(&StructureNode);
const Rtti AnnotationEntity =
    RttiBuilder<model::AnnotationEntity>("AnnotationEntity")
        .parent(&Node)
        .composedOf({&StructuredEntity, &DocumentPrimitive, &Anchor});
}
}

