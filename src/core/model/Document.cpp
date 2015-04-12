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

/* Class DocumentEntity */

void DocumentEntity::invalidateSubInstance()
{
	if (subInst->isa(&RttiTypes::StructuredEntity)) {
		subInst.cast<StructuredEntity>()->invalidate();
	} else {
		subInst.cast<AnnotationEntity>()->invalidate();
	}
}

DocumentEntity::DocumentEntity(Handle<Node> subInst,
                               Handle<Descriptor> descriptor,
                               Variant attributes)
    : subInst(subInst),
      // initialize descriptor as nullptr first and then set it right
      attributes(std::move(attributes))
{
	// insert empty vectors for each field.
	if (descriptor != nullptr) {
		setDescriptor(descriptor);
	}
}

void DocumentEntity::setDescriptor(Handle<Descriptor> d)
{
	// check if we have to do anything.
	if (descriptor == d) {
		return;
	}
	invalidateSubInstance();
	descriptor = subInst->acquire(d);
	// clear the fields vector.
	fields.clear();
	// fill it again.
	for (size_t f = 0; f < descriptor->getFieldDescriptors().size(); f++) {
		fields.push_back(NodeVector<StructureNode>(subInst));
	}
}

bool DocumentEntity::doValidate(Logger &logger) const
{
	// if we have no descriptor, this is invalid.
	if (descriptor == nullptr) {
		logger.error("This entity has no descriptor!", *subInst);
		// in this case we have to stop the validation process, because without
		// a constructor we can not check anything else.
		return false;
	}
	// if we have an invalid descriptor we can not proceed either.
	if (!descriptor->validate(logger)) {
		return false;
	}
	// check the attribute primitive content.
	bool valid;
	if (descriptor->getAttributesDescriptor() == nullptr) {
		valid = getAttributes() == nullptr;
	} else {
		valid = descriptor->getAttributesDescriptor()->isValid(getAttributes(),
		                                                       logger);
	}
	/*
	 * generate the set of effective fields. This is trivial for
	 * AnnotationEntities, but in the case of StructuredEntities we have to
	 * gather all fields of superclasses as well, that have not been
	 * overridden in the subclasses.
	 */
	ManagedVector<FieldDescriptor> fieldDescs =
	    descriptor->getFieldDescriptors();
	// iterate over every field
	for (unsigned int f = 0; f < fields.size(); f++) {
		// we have a special check for primitive fields.
		if (fieldDescs[f]->isPrimitive()) {
			switch (fields[f].size()) {
				case 0:
					if (!fieldDescs[f]->isOptional()) {
						logger.error(std::string("Primitive Field \"") +
						                 fieldDescs[f]->getNameOrDefaultName() +
						                 "\" had no content!",
						             *subInst);
						valid = false;
					}
					continue;
				case 1:
					break;
				default:
					logger.error(std::string("Primitive Field \"") +
					                 fieldDescs[f]->getNameOrDefaultName() +
					                 "\" had more than one child!",
					             *subInst);
					valid = false;
					continue;
			}
			// if we are here we know that exactly one child exists.
			if (!fields[f][0]->isa(&RttiTypes::DocumentPrimitive)) {
				logger.error(std::string("Primitive Field \"") +
				                 fieldDescs[f]->getNameOrDefaultName() +
				                 "\" has non primitive content!",
				             *subInst);
				valid = false;
			} else {
				Handle<DocumentPrimitive> primitive =
				    fields[f][0].cast<DocumentPrimitive>();
				valid = valid &
				        fieldDescs[f]->getPrimitiveType()->isValid(
				            primitive->getContent(), logger);
			}
			continue;
		}

		std::unordered_set<StructuredClass *> childClasses;
		{
			ManagedVector<StructuredClass> tmp =
			    fieldDescs[f]->getChildrenWithSubclasses();
			for (const auto &s : tmp) {
				childClasses.insert(s.get());
			}
		}

		// we can do a faster check if this field is empty.
		if (fields[f].size() == 0) {
			// if this field is optional, an empty field is valid anyways.
			if (fieldDescs[f]->isOptional()) {
				continue;
			}
			/*
			 * if it is not optional we have to check if zero is a valid
			 * cardinality.
			 */
			for (auto childClass : childClasses) {
				const size_t min =
				    childClass->getCardinality().asCardinality().min();
				if (min > 0) {
					logger.error(std::string("Field \"") +
					                 fieldDescs[f]->getNameOrDefaultName() +
					                 "\" was empty but needs at least " +
					                 std::to_string(min) +
					                 " elements of class \"" +
					                 childClass->getName() +
					                 "\" according to the definition of \"" +
					                 descriptor->getName() + "\"",
					             *subInst);
					valid = false;
				}
			}
			continue;
		}

		// store the actual numbers of children for each child class in a map
		std::unordered_map<StructuredClass *, unsigned int> nums;

		// iterate over every actual child of this field
		for (auto child : fields[f]) {
			// check if the parent reference is correct.
			if (child->getParent() != subInst) {
				logger.error(std::string("A child of field \"") +
				                 fieldDescs[f]->getNameOrDefaultName() +
				                 "\" has the wrong parent reference!",
				             *child);
				valid = false;
			}
			if (child->isa(&RttiTypes::Anchor)) {
				// Anchors are uninteresting and can be ignored.
				continue;
			}
			if (child->isa(&RttiTypes::DocumentPrimitive)) {
				logger.error(std::string("Non-primitive Field \"") +
				                 fieldDescs[f]->getNameOrDefaultName() +
				                 "\" had primitive content!",
				             *child);
				valid = false;
				continue;
			}
			// otherwise this is a StructuredEntity
			Handle<StructuredEntity> c = child.cast<StructuredEntity>();
			StructuredClass *classPtr =
			    c->getDescriptor().cast<StructuredClass>().get();

			// check if its class is allowed.
			if (childClasses.find(classPtr) == childClasses.end()) {
				logger.error(
				    std::string("An instance of \"") +
				        c->getDescriptor()->getName() +
				        "\" is not allowed as child of an instance of \"" +
				        descriptor->getName() + "\" in field \"" +
				        fieldDescs[f]->getNameOrDefaultName() + "\"",
				    *child);
				valid = false;
				continue;
			}
			// note the number of occurences for this class and all
			// superclasses, because a subclass instance should count for
			// superclasses as well.
			while (classPtr != nullptr &&
			       childClasses.find(classPtr) != childClasses.end()) {
				const auto &n = nums.find(classPtr);
				if (n != nums.end()) {
					n->second++;
				} else {
					nums.emplace(classPtr, 1);
				}
				classPtr = classPtr->getSuperclass().get();
			}
		}

		// now check if the cardinalities are right.
		for (auto childClass : childClasses) {
			const auto &n = nums.find(childClass);
			unsigned int num = 0;
			if (n != nums.end()) {
				num = n->second;
			}
			if (!childClass->getCardinality().asCardinality().contains(num)) {
				logger.error(std::string("Field \"") +
				                 fieldDescs[f]->getNameOrDefaultName() +
				                 "\" had " + std::to_string(num) +
				                 " elements of class \"" +
				                 childClass->getName() +
				                 "\", which is invalid according to the "
				                 "definition of \"" +
				                 descriptor->getName() + "\"",
				             *subInst);
				valid = false;
				continue;
			}
		}
	}

	// go into recursion.
	for (auto f : fields) {
		for (auto n : f) {
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

static int enforceGetFieldDescriptorIndex(Handle<Descriptor> desc,
                                          const std::string &fieldName)
{
	ssize_t idx = desc->getFieldDescriptorIndex(fieldName);
	if (idx == -1) {
		throw OusiaException(std::string("Descriptor \"") + desc->getName() +
		                     "\" has no field with the name \"" + fieldName +
		                     "\"");
	}
	return idx;
}

static int enforceGetFieldDescriptorIndex(
    Handle<Descriptor> desc, Handle<FieldDescriptor> fieldDescriptor)
{
	ssize_t idx = desc->getFieldDescriptorIndex(fieldDescriptor);
	if (idx == -1) {
		throw OusiaException(std::string("Descriptor \"") + desc->getName() +
		                     "\" does not reference the given field \"" +
		                     fieldDescriptor->getNameOrDefaultName() + "\"");
	}
	return idx;
}

const NodeVector<StructureNode> &DocumentEntity::getField(
    const std::string &fieldName) const
{
	return fields[enforceGetFieldDescriptorIndex(descriptor, fieldName)];
}

const NodeVector<StructureNode> &DocumentEntity::getField(
    Handle<FieldDescriptor> fieldDescriptor) const
{
	return fields[enforceGetFieldDescriptorIndex(descriptor, fieldDescriptor)];
}

const NodeVector<StructureNode> &DocumentEntity::getField(size_t idx) const
{
	if (idx >= fields.size()) {
		throw OusiaException(std::string("Descriptor \"") +
		                     descriptor->getName() +
		                     "\" does not have enough fields for index \"" +
		                     std::to_string(idx) + "\".");
	}
	return fields[idx];
}

void DocumentEntity::addStructureNode(Handle<StructureNode> s, size_t i)
{
	// only add the new node if we don't have it already.
	auto it = fields[i].find(s);
	if (it == fields[i].end()) {
		invalidateSubInstance();
		fields[i].push_back(s);
	}
	Handle<Managed> par = s->getParent();
	if (par != subInst) {
		// if a previous parent existed, remove the StructureNode from it
		if (par != nullptr) {
			if (par->isa(&RttiTypes::StructuredEntity)) {
				par.cast<StructuredEntity>()->removeStructureNode(s);
			} else if (par->isa(&RttiTypes::AnnotationEntity)) {
				par.cast<AnnotationEntity>()->removeStructureNode(s);
			} else if (par->isa(&RttiTypes::Document)) {
				par.cast<Document>()->setRoot(nullptr);
			}
		}
		s->setParent(subInst);
	}
}

void DocumentEntity::addStructureNode(Handle<StructureNode> s,
                                      Handle<FieldDescriptor> fieldDescriptor)
{
	addStructureNode(
	    s, enforceGetFieldDescriptorIndex(descriptor, fieldDescriptor));
}

void DocumentEntity::addStructureNodes(
    const std::vector<Handle<StructureNode>> &ss,
    Handle<FieldDescriptor> fieldDescriptor)
{
	const int i = enforceGetFieldDescriptorIndex(descriptor, fieldDescriptor);
	for (Handle<StructureNode> s : ss) {
		addStructureNode(s, i);
	}
}

void DocumentEntity::addStructureNode(Handle<StructureNode> s,
                                      const std::string &fieldName)
{
	addStructureNode(s, enforceGetFieldDescriptorIndex(descriptor, fieldName));
}

void DocumentEntity::addStructureNodes(
    const std::vector<Handle<StructureNode>> &ss, const std::string &fieldName)
{
	const int idx = enforceGetFieldDescriptorIndex(descriptor, fieldName);
	for (Handle<StructureNode> s : ss) {
		addStructureNode(s, idx);
	}
}

bool DocumentEntity::removeStructureNodeFromField(Handle<StructureNode> s,
                                                  size_t i)
{
	auto it = fields[i].find(s);
	if (it != fields[i].end()) {
		invalidateSubInstance();
		fields[i].erase(it);
		s->setParent(nullptr);
		return true;
	}
	return false;
}

bool DocumentEntity::removeStructureNodeFromField(Handle<StructureNode> s,
                                                  const std::string &fieldName)
{
	return removeStructureNodeFromField(
	    s, enforceGetFieldDescriptorIndex(descriptor, fieldName));
}

bool DocumentEntity::removeStructureNodeFromField(
    Handle<StructureNode> s, Handle<FieldDescriptor> fieldDescriptor)
{
	return removeStructureNodeFromField(
	    s, enforceGetFieldDescriptorIndex(descriptor, fieldDescriptor));
}

bool DocumentEntity::removeStructureNode(Handle<StructureNode> s)
{
	for (auto field : fields) {
		auto it = field.find(s);
		if (it != field.end()) {
			invalidateSubInstance();
			field.erase(it);
			s->setParent(nullptr);
			return true;
		}
	}

	return false;
}

Rooted<StructuredEntity> DocumentEntity::createChildStructuredEntity(
    Handle<StructuredClass> descriptor, Variant attributes,
    const std::string &fieldName, std::string name)
{
	return Rooted<StructuredEntity>{new StructuredEntity(
	    subInst->getManager(), subInst, descriptor, std::move(attributes),
	    fieldName, std::move(name))};
}

Rooted<StructuredEntity> DocumentEntity::createChildStructuredEntity(
    Handle<StructuredClass> descriptor, size_t fieldIdx, Variant attributes,
    std::string name)
{
	return Rooted<StructuredEntity>{
	    new StructuredEntity(subInst->getManager(), subInst, descriptor,
	                         fieldIdx, std::move(attributes), std::move(name))};
}

Rooted<DocumentPrimitive> DocumentEntity::createChildDocumentPrimitive(
    Variant content, const std::string &fieldName)
{
	return Rooted<DocumentPrimitive>{new DocumentPrimitive(
	    subInst->getManager(), subInst, std::move(content), fieldName)};
}

Rooted<DocumentPrimitive> DocumentEntity::createChildDocumentPrimitive(
    Variant content, size_t fieldIdx)
{
	return Rooted<DocumentPrimitive>{new DocumentPrimitive(
	    subInst->getManager(), subInst, std::move(content), fieldIdx)};
}

Rooted<Anchor> DocumentEntity::createChildAnchor(const std::string &fieldName)
{
	return Rooted<Anchor>{
	    new Anchor(subInst->getManager(), subInst, fieldName)};
}
Rooted<Anchor> DocumentEntity::createChildAnchor(size_t fieldIdx)
{
	return Rooted<Anchor>{new Anchor(subInst->getManager(), subInst, fieldIdx)};
}

static bool matchStartAnchor(Handle<AnnotationClass> desc,
                             const std::string &name, Handle<Anchor> a)
{
	return (a->getAnnotation() != nullptr) &&
	       (a->getAnnotation()->getEnd() == nullptr) &&
	       (desc == nullptr || a->getAnnotation()->getDescriptor() == desc) &&
	       (name.empty() || a->getAnnotation()->getName() == name);
}

template <typename Iterator>
Rooted<Anchor> DocumentEntity::searchStartAnchorInField(
    Handle<AnnotationClass> desc, const std::string &name, Iterator begin,
    Iterator end, std::unordered_set<const DocumentEntity *> &visited)
{
	for (Iterator it = begin; it != end; it++) {
		Handle<StructureNode> strct = *it;
		if (strct->isa(&RttiTypes::Anchor)) {
			// check if this Anchor is the right one.
			Handle<Anchor> a = strct.cast<Anchor>();
			if (matchStartAnchor(desc, name, a)) {
				return a;
			}
			continue;
		} else if (strct->isa(&RttiTypes::StructuredEntity)) {
			// search downwards.
			Rooted<Anchor> a =
			    strct.cast<StructuredEntity>()->searchStartAnchorDownwards(
			        desc, name, visited);
			if (a != nullptr) {
				return a;
			}
		}
	}
	return nullptr;
}

Rooted<Anchor> DocumentEntity::searchStartAnchorDownwards(
    Handle<AnnotationClass> desc, const std::string &name,
    std::unordered_set<const DocumentEntity *> &visited)
{
	if (!visited.insert(this).second) {
		return nullptr;
	}
	if (fields.empty()) {
		return nullptr;
	}
	// get the default field.
	NodeVector<StructureNode> children = fields[fields.size() - 1];
	// search it from back to front.
	return searchStartAnchorInField(desc, name, children.rbegin(),
	                                children.rend(), visited);
}

Rooted<Anchor> DocumentEntity::searchStartAnchorUpwards(
    Handle<AnnotationClass> desc, const std::string &name,
    const DocumentEntity *child,
    std::unordered_set<const DocumentEntity *> &visited)
{
	if (!visited.insert(this).second) {
		return nullptr;
	}
	if (fields.empty()) {
		return nullptr;
	}
	// get the default field.
	NodeVector<StructureNode> children = fields[fields.size() - 1];
	// search for the child from back to front.
	auto it = children.rbegin();
	while (static_cast<void *>(it->get()) != child->subInst.get() &&
	       it != children.rend()) {
		it++;
	}
	// increment the reverse iterator once more to prevent downwards search
	// to the child.
	if (it != children.rend()) {
		it++;
		return searchStartAnchorInField(desc, name, it, children.rend(),
		                                visited);
	}
	throw OusiaException("Internal error: Child node not found in parent!");
}

Rooted<Anchor> DocumentEntity::searchStartAnchor(size_t fieldIdx,
                                                 Handle<AnnotationClass> desc,
                                                 const std::string &name)
{
	std::unordered_set<const DocumentEntity *> visited;
	visited.insert(this);
	// get the correct field.
	NodeVector<StructureNode> children = fields[fieldIdx];
	// search it from back to front.
	Rooted<Anchor> a = searchStartAnchorInField(desc, name, children.rbegin(),
	                                            children.rend(), visited);
	// if we found the Anchor, return it.
	if (a != nullptr) {
		return a;
	}

	// If this is either an AnnotationEntity or a SUBTREE field we can not
	// search upwards.
	if (subInst->isa(&RttiTypes::AnnotationEntity) ||
	    fieldIdx + 1 < fields.size()) {
		return nullptr;
	}
	// if the children here did not contain the right start Anchor go upwards.
	if (subInst->getParent()->isa(&RttiTypes::StructuredEntity)) {
		return subInst->getParent()
		    .cast<StructuredEntity>()
		    ->searchStartAnchorUpwards(desc, name, this, visited);
	}
	if (subInst->getParent()->isa(&RttiTypes::AnnotationEntity)) {
		subInst->getParent().cast<AnnotationEntity>()->searchStartAnchorUpwards(
		    desc, name, this, visited);
	}
	return nullptr;
}

/* Class StructureNode */

bool StructureNode::doValidate(Logger &logger) const
{
	bool valid = true;
	// check name
	if (!getName().empty()) {
		valid = validateName(logger);
	}
	// check the parent.
	if (getParent() == nullptr) {
		logger.error("The parent is not set!", *this);
		valid = false;
	}
	if (!getParent()->isa(&RttiTypes::StructuredEntity) &&
	    !getParent()->isa(&RttiTypes::AnnotationEntity) &&
	    !getParent()->isa(&RttiTypes::Document)) {
		logger.error("The parent does not have a valid type!", *this);
		valid = false;
	}
	return valid;
}

StructureNode::StructureNode(Manager &mgr, std::string name,
                             Handle<Node> parent, const std::string &fieldName)
    : Node(mgr, std::move(name), parent)
{
	if (parent->isa(&RttiTypes::StructuredEntity)) {
		parent.cast<StructuredEntity>()->addStructureNode(this, fieldName);
	} else if (parent->isa(&RttiTypes::AnnotationEntity)) {
		parent.cast<AnnotationEntity>()->addStructureNode(this, fieldName);
	} else {
		throw OusiaException("The proposed parent was no DocumentEntity!");
	}
}

StructureNode::StructureNode(Manager &mgr, std::string name,
                             Handle<Node> parent, size_t fieldIdx)
    : Node(mgr, std::move(name), parent)
{
	if (parent->isa(&RttiTypes::StructuredEntity)) {
		parent.cast<StructuredEntity>()->addStructureNode(this, fieldIdx);
	} else if (parent->isa(&RttiTypes::AnnotationEntity)) {
		parent.cast<AnnotationEntity>()->addStructureNode(this, fieldIdx);
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

StructuredEntity::StructuredEntity(Manager &mgr, Handle<Node> parent,
                                   Handle<StructuredClass> descriptor,
                                   Variant attributes, std::string name)
    : StructureNode(mgr, std::move(name), parent),
      DocumentEntity(this, descriptor, std::move(attributes))
{
}

bool StructuredEntity::doValidate(Logger &logger) const
{
	bool valid = true;
	// check the parent.
	if (getDescriptor() == nullptr) {
		logger.error("The descriptor is not set!", *this);
		valid = false;
	} else if (!getDescriptor()->isa(&RttiTypes::StructuredClass)) {
		logger.error("The descriptor is not a structure descriptor!", *this);
		valid = false;
	} else if (transparent &&
	           !getDescriptor().cast<StructuredClass>()->isTransparent()) {
		logger.error(
		    "The entity is marked as transparent but the descriptor "
		    "does not allow transparency!",
		    *this);
		valid = false;
	}

	// check the validity as a StructureNode and as a DocumentEntity.
	return valid & StructureNode::doValidate(logger) &
	       DocumentEntity::doValidate(logger);
}

/* Class Anchor */

bool Anchor::doValidate(Logger &logger) const
{
	bool valid = true;
	// check name
	if (!getName().empty()) {
		logger.error(
		    "This anchor has a name! Anchors should only be referred to by "
		    "reference, not by name!",
		    *this);
		valid = false;
	}
	if (annotation == nullptr) {
		logger.error("This anchor is disconnected.", *this);
		valid = false;
	}
	return valid & StructureNode::doValidate(logger);
}

void Anchor::setAnnotation(Handle<AnnotationEntity> anno, bool start)
{
	if (annotation == anno) {
		return;
	}
	invalidate();
	// unset the old reference.
	if (annotation != nullptr) {
		if (isStart()) {
			annotation->setStart(nullptr);
		} else {
			annotation->setEnd(nullptr);
		}
	}
	annotation = acquire(anno);
	// set the new reference.
	if (anno != nullptr) {
		if (start) {
			anno->setStart(this);
		} else {
			anno->setEnd(this);
		}
	}
}

bool Anchor::isStart() const
{
	if (annotation == nullptr) {
		return false;
	}
	return annotation->getStart() == this;
}

bool Anchor::isEnd() const
{
	if (annotation == nullptr) {
		return false;
	}
	return annotation->getEnd() == this;
}

/* Class AnnotationEntity */

AnnotationEntity::AnnotationEntity(Manager &mgr, Handle<Document> parent,
                                   Handle<AnnotationClass> descriptor,
                                   Handle<Anchor> start, Handle<Anchor> end,
                                   Variant attributes, std::string name)
    : Node(mgr, std::move(name), parent),
      DocumentEntity(this, descriptor, attributes)
{
	if (parent != nullptr) {
		parent->addAnnotation(this);
	}
	setStart(start);
	setEnd(end);
}

bool AnnotationEntity::doValidate(Logger &logger) const
{
	bool valid = true;
	// check name
	if (!getName().empty()) {
		valid = valid & validateName(logger);
	}
	// check if this AnnotationEntity is correctly registered at its document.
	if (getParent() == nullptr) {
		logger.error("The parent is not set!", *this);
		valid = false;
	} else if (!getParent()->isa(&RttiTypes::Document)) {
		logger.error("The parent is not a document!", *this);
		valid = false;
	} else {
		Handle<Document> doc = getParent().cast<Document>();
		bool found = false;
		for (auto &a : doc->getAnnotations()) {
			if (a == this) {
				found = true;
				break;
			}
		}
		if (!found) {
			logger.error("This annotation was not registered at the document.",
			             *this);
			valid = false;
		}
		// check if the Anchors are part of the right document.
		if (start == nullptr) {
			logger.error("This annotation has no start Anchor!", *this);
			valid = false;
		} else if (!doc->hasChild(start)) {
			logger.error(
			    "This annotations start anchor was not part of the same "
			    "document!",
			    *this);
			valid = false;
		}
		if (end == nullptr) {
			logger.error("This annotation has no end Anchor!", *this);
			valid = false;
		} else if (!doc->hasChild(end)) {
			logger.error(
			    "This annotations end anchor was not part of the same "
			    "document!",
			    *this);
			valid = false;
		}
	}
	// check if the Anchors reference this AnnotationEntity correctly.
	if (start != nullptr) {
		if (start->getAnnotation() != this) {
			logger.error(
			    "This annotations start anchor does not have the correct "
			    "annotation as parent!",
			    *this);
			valid = false;
		}
	}
	if (end != nullptr) {
		if (end->getAnnotation() != this) {
			logger.error(
			    "This annotations end anchor does not have the correct "
			    "annotation as parent!",
			    *this);
			valid = false;
		}
	}

	// check the validity as a DocumentEntity.
	return valid & DocumentEntity::doValidate(logger);
}

void AnnotationEntity::setStart(Handle<Anchor> s)
{
	if (start == s) {
		return;
	}
	invalidate();
	start = acquire(s);
	if (s != nullptr) {
		s->setAnnotation(this, true);
	}
}

void AnnotationEntity::setEnd(Handle<Anchor> e)
{
	if (end == e) {
		return;
	}
	invalidate();
	end = acquire(e);
	if (e != nullptr) {
		e->setAnnotation(this, false);
	}
}

/* Class Document */

void Document::doResolve(ResolutionState &state)
{
	continueResolveComposita(annotations, annotations.getIndex(), state);
	if (root != nullptr) {
		continueResolveCompositum(root, state);
	}
	continueResolveReferences(ontologies, state);
	continueResolveReferences(typesystems, state);
}

bool Document::doValidate(Logger &logger) const
{
	// An empty document is always invalid. TODO: Is this a smart choice?
	bool valid = true;
	if (root == nullptr) {
		logger.error("This document is empty (it has no root)!", *this);
		valid = false;
	} else {
		// check if the root is allowed to be a root.
		if (!root->getDescriptor()
		         .cast<StructuredClass>()
		         ->hasRootPermission()) {
			logger.error(std::string("A node of type \"") +
			                 root->getDescriptor()->getName() +
			                 "\" is not allowed to be the Document root!",
			             *root);
			valid = false;
		}
		// check if it has this document as parent.
		if (root->getParent() != this) {
			logger.error(
			    "The document root does not have the document as parent!",
			    *root);
			valid = false;
		}
		// then call validate on the root
		valid = valid & root->validate(logger);
	}
	// call validate on the AnnotationEntities
	return valid & continueValidation(annotations, logger);
}

void Document::doReference(Handle<Node> node)
{
	if (node->isa(&RttiTypes::Ontology)) {
		referenceOntology(node.cast<Ontology>());
	}
	if (node->isa(&RttiTypes::Typesystem)) {
		referenceTypesystem(node.cast<Typesystem>());
	}
}

RttiSet Document::doGetReferenceTypes() const
{
	return RttiSet{&RttiTypes::Ontology, &RttiTypes::Typesystem};
}

Rooted<StructuredEntity> Document::createRootStructuredEntity(
    Handle<StructuredClass> descriptor, Variant attributes, std::string name)
{
	return Rooted<StructuredEntity>{
	    new StructuredEntity(getManager(), Handle<Document>{this}, descriptor,
	                         attributes, std::move(name))};
}

void Document::addAnnotation(Handle<AnnotationEntity> a)
{
	// only add it if we need to.
	if (annotations.find(a) == annotations.end()) {
		invalidate();
		annotations.push_back(a);
	}
	Handle<Managed> par = a->getParent();
	if (par != this) {
		if (par != nullptr) {
			// remove the StructuredClass from the old parent.
			par.cast<Document>()->removeAnnotation(a);
		}
		a->setParent(this);
	}
}

void Document::addAnnotations(const std::vector<Handle<AnnotationEntity>> &as)
{
	for (Handle<AnnotationEntity> a : as) {
		addAnnotation(a);
	}
}

bool Document::removeAnnotation(Handle<AnnotationEntity> a)
{
	auto it = annotations.find(a);
	if (it != annotations.end()) {
		invalidate();
		annotations.erase(it);
		a->setParent(nullptr);
		return true;
	}
	return false;
}

Rooted<AnnotationEntity> Document::createChildAnnotation(
    Handle<AnnotationClass> descriptor, Handle<Anchor> start,
    Handle<Anchor> end, Variant attributes, std::string name)
{
	return Rooted<AnnotationEntity>{
	    new AnnotationEntity(getManager(), this, descriptor, start, end,
	                         attributes, std::move(name))};
}

bool Document::hasChild(Handle<StructureNode> s) const
{
	Rooted<Managed> parent = s->getParent();
	if (parent->isa(&RttiTypes::StructureNode)) {
		return hasChild(parent.cast<StructureNode>());
	} else if (parent->isa(&RttiTypes::AnnotationEntity)) {
		Handle<AnnotationEntity> a = parent.cast<AnnotationEntity>();
		return this == a->getParent();
	} else if (parent->isa(&RttiTypes::Document)) {
		return this == parent;
	}
	return false;
}

void Document::setRoot(Handle<StructuredEntity> rt)
{
	if (rt == nullptr || root == rt) {
		return;
	}
	if (root != nullptr) {
		root->setParent(nullptr);
	}
	root = acquire(rt);
	if (rt->getParent() != this) {
		rt->setParent(this);
	}
	invalidate();
}

/* Type registrations */
namespace RttiTypes {
const Rtti Document = RttiBuilder<ousia::Document>("Document")
                          .parent(&RootNode)
                          .composedOf({&AnnotationEntity, &StructuredEntity});
const Rtti StructureNode =
    RttiBuilder<ousia::StructureNode>("StructureNode").parent(&Node);
const Rtti StructuredEntity =
    RttiBuilder<ousia::StructuredEntity>("StructuredEntity")
        .parent(&StructureNode)
        .composedOf({&StructuredEntity, &DocumentPrimitive, &Anchor});
const Rtti DocumentPrimitive = RttiBuilder<ousia::DocumentPrimitive>(
                                   "DocumentPrimitive").parent(&StructureNode);
const Rtti Anchor = RttiBuilder<ousia::Anchor>("Anchor").parent(&StructureNode);
const Rtti AnnotationEntity =
    RttiBuilder<ousia::AnnotationEntity>("AnnotationEntity")
        .parent(&Node)
        .composedOf({&StructuredEntity, &DocumentPrimitive, &Anchor});
}
}
