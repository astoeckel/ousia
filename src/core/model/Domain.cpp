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

#include <memory>
#include <queue>
#include <set>

#include <core/common/RttiBuilder.hpp>
#include <core/common/Exceptions.hpp>

#include "Domain.hpp"

namespace ousia {

/* Class FieldDescriptor */

FieldDescriptor::FieldDescriptor(Manager &mgr, Handle<Type> primitiveType,
                                 Handle<Descriptor> parent, FieldType fieldType,
                                 std::string name, bool optional)
    : Node(mgr, std::move(name), parent),
      children(this),
      fieldType(fieldType),
      primitiveType(acquire(primitiveType)),
      optional(optional),
      primitive(true)
{
}

FieldDescriptor::FieldDescriptor(Manager &mgr, Handle<Descriptor> parent,
                                 FieldType fieldType, std::string name,
                                 bool optional)
    : Node(mgr, std::move(name), parent),
      children(this),
      fieldType(fieldType),
      optional(optional),
      primitive(false)
{
}

bool FieldDescriptor::doValidate(Logger &logger) const
{
	bool valid = true;
	// check parent type
	if (getParent() == nullptr) {
		logger.error(std::string("Field \"") + getName() + "\" has no parent!",
		             *this);
		valid = false;
	} else if (!getParent()->isa(&RttiTypes::Descriptor)) {
		logger.error(std::string("The parent of Field \"") + getName() +
		                 "\" is not a descriptor!",
		             *this);
		valid = false;
	}
	// check name
	if (getName().empty()) {
		if (fieldType != FieldType::TREE) {
			logger.error(std::string("Field \"") + getName() +
			                 "\" is not the main field but has an empty name!",
			             *this);
			valid = false;
		}
	} else {
		valid = valid & validateName(logger);
	}

	// check consistency of FieldType with the rest of the FieldDescriptor.
	if (primitive) {
		if (children.size() > 0) {
			logger.error(std::string("Field \"") + getName() +
			                 "\" is supposed to be primitive but has "
			                 "registered child classes!",
			             *this);
			valid = false;
		}
		if (primitiveType == nullptr) {
			logger.error(std::string("Field \"") + getName() +
			                 "\" is supposed to be primitive but has "
			                 "no primitive type!",
			             *this);
			valid = false;
		}
	} else {
		if (primitiveType != nullptr) {
			logger.error(std::string("Field \"") + getName() +
			                 "\" is supposed to be non-primitive but has "
			                 "a primitive type!",
			             *this);
			valid = false;
		}
		// if this is not a primitive field we require at least one child.
		if (children.empty()) {
			logger.error(std::string("Field \"") + getName() +
			                 "\" is non primitive but does not allow children!",
			             *this);
			valid = false;
		}
	}
	/*
	 * we are not allowed to call the validation functions of each child because
	 * this might lead to cycles. What we should do, however, is to check if
	 * there are duplicates.
	 */
	std::set<std::string> names;
	for (Handle<StructuredClass> c : children) {
		if (!names.insert(c->getName()).second) {
			logger.error(std::string("Field \"") + getName() +
			                 "\" had multiple children with the name \"" +
			                 c->getName() + "\"",
			             *this);
			valid = false;
		}
	}

	return valid;
}

bool FieldDescriptor::removeChild(Handle<StructuredClass> c)
{
	auto it = children.find(c);
	if (it != children.end()) {
		invalidate();
		children.erase(it);
		return true;
	}
	return false;
}

/* Class Descriptor */

void Descriptor::doResolve(ResolutionState &state)
{
	const NodeVector<Attribute> &attributes =
	    attributesDescriptor->getAttributes();
	continueResolveComposita(attributes, attributes.getIndex(), state);
	continueResolveComposita(fieldDescriptors, fieldDescriptors.getIndex(),
	                         state);
}

bool Descriptor::doValidate(Logger &logger) const
{
	bool valid = true;
	// check parent type
	if (getParent() == nullptr) {
		logger.error(
		    std::string("Descriptor \"") + getName() + "\" has no parent!",
		    *this);
		valid = false;
	} else if (!getParent()->isa(&RttiTypes::Domain)) {
		logger.error(std::string("The parent of Descriptor \"") + getName() +
		                 "\" is not a Domain!",
		             *this);
		valid = false;
	}
	// check name
	if (getName().empty()) {
		logger.error("The name of this Descriptor is empty!", *this);
		valid = false;
	} else {
		valid = valid & validateName(logger);
	}
	// ensure that no attribute with the key "name" exists.
	if (attributesDescriptor == nullptr) {
		logger.error(std::string("Descriptor \"") + getName() +
		             "\" has no Attribute specification!");
		valid = false;
	} else {
		if (attributesDescriptor->hasAttribute("name")) {
			logger.error(
			    std::string("Descriptor \"") + getName() +
			    "\" has an attribute \"name\" which is a reserved word!");
			valid = false;
		}
		valid = valid & attributesDescriptor->validate(logger);
	}
	// check that only one FieldDescriptor is of type TREE.
	auto fds = Descriptor::getFieldDescriptors();
	bool hasTREE = false;
	for (auto fd : fds) {
		if (fd->getFieldType() == FieldDescriptor::FieldType::TREE) {
			if (!hasTREE) {
				hasTREE = true;
			} else {
				logger.error(
				    std::string("Descriptor \"") + getName() +
				        "\" has multiple TREE fields, which is not permitted",
				    *fd);
				valid = false;
				break;
			}
		}
	}

	// check attributes and the FieldDescriptors
	return valid & continueValidationCheckDuplicates(fds, logger);
}

struct PathState {
	std::shared_ptr<PathState> pred;
	Node *node;
	size_t length;

	PathState(std::shared_ptr<PathState> pred, Node *node)
	    : pred(pred), node(node)
	{
		if (pred == nullptr) {
			length = 1;
		} else {
			length = pred->length + 1;
		}
	}
};

static void constructPath(std::shared_ptr<PathState> state,
                          NodeVector<Node> &vec)
{
	if (state->pred != nullptr) {
		constructPath(state->pred, vec);
	}
	vec.push_back(state->node);
}

static NodeVector<Node> pathTo(const Descriptor *start, Logger &logger,
                               Handle<Node> target, bool &success)
{
	success = false;
	// shortest path.
	NodeVector<Node> shortest;
	// state queue for breadth-first search.
	std::queue<std::shared_ptr<PathState>> states;
	{
		// initially put every field descriptor on the queue.
		NodeVector<FieldDescriptor> fields = start->getFieldDescriptors();

		for (auto fd : fields) {
			if (fd == target) {
				// if we have found the target directly, return without search.
				success = true;
				return shortest;
			}
			if (fd->getFieldType() == FieldDescriptor::FieldType::TREE) {
				states.push(std::make_shared<PathState>(nullptr, fd.get()));
			}
		}
	}
	// set of visited nodes.
	std::unordered_set<const Node *> visited;
	while (!states.empty()) {
		std::shared_ptr<PathState> current = states.front();
		states.pop();
		// do not proceed if this node was already visited.
		if (!visited.insert(current->node).second) {
			continue;
		}
		// also do not proceed if we can't get better than the current shortest
		// path anymore.
		if (!shortest.empty() && current->length > shortest.size()) {
			continue;
		}

		bool fin = false;
		if (current->node->isa(&RttiTypes::StructuredClass)) {
			const StructuredClass *strct =
			    static_cast<const StructuredClass *>(current->node);

			// look through all fields.
			NodeVector<FieldDescriptor> fields = strct->getFieldDescriptors();
			for (auto fd : fields) {
				// if we found our target, break off the search in this branch.
				if (fd == target) {
					fin = true;
					continue;
				}
				// only continue in the TREE field.
				if (fd->getFieldType() == FieldDescriptor::FieldType::TREE) {
					states.push(std::make_shared<PathState>(current, fd.get()));
				}
			}

			/*
			 * Furthermore we have to consider that all subclasses of this
			 * StructuredClass are allowed in place of this StructuredClass as
			 * well, so we continue the search for them as well.
			 */

			NodeVector<StructuredClass> subs = strct->getSubclasses();
			for (auto sub : subs) {
				// if we found our target, break off the search in this branch.
				if (sub == target) {
					fin = true;
					current = current->pred;
					continue;
				}
				// We only continue our path via transparent classes.
				if (sub->isTransparent()) {
					states.push(
					    std::make_shared<PathState>(current->pred, sub.get()));
				}
			}
		} else {
			// otherwise this is a FieldDescriptor.
			const FieldDescriptor *field =
			    static_cast<const FieldDescriptor *>(current->node);
			// and we proceed by visiting all permitted children.
			for (auto c : field->getChildren()) {
				// if we found our target, break off the search in this branch.
				if (c == target) {
					fin = true;
					continue;
				}
				// We only allow to continue our path via transparent children.
				if (c->isTransparent()) {
					states.push(std::make_shared<PathState>(current, c.get()));
				}
			}
		}
		// check if we are finished.
		if (fin) {
			success = true;
			// if so we look if we found a shorter path than the current minimum
			if (shortest.empty() || current->length < shortest.size()) {
				NodeVector<Node> newPath;
				constructPath(current, newPath);
				shortest = newPath;
			} else if (current->length == shortest.size()) {
				// if the length is the same the result is ambigous and we log
				// an error.
				NodeVector<Node> newPath;
				constructPath(current, newPath);
				logger.error(
				    std::string("Can not unambigously create a path from \"") +
				    start->getName() + "\" to \"" + target->getName() + "\".");
				logger.note("Dismissed the path:", SourceLocation{},
				            MessageMode::NO_CONTEXT);
				for (auto n : newPath) {
					logger.note(n->getName());
				}
			}
		}
	}
	return shortest;
}

NodeVector<Node> Descriptor::pathTo(Handle<StructuredClass> target,
                                    Logger &logger) const
{
	bool success = false;
	return ousia::pathTo(this, logger, target, success);
}

std::pair<NodeVector<Node>, bool> Descriptor::pathTo(
    Handle<FieldDescriptor> field, Logger &logger) const
{
	bool success = false;
	NodeVector<Node> path = ousia::pathTo(this, logger, field, success);
	return std::make_pair(path, success);
}
{
}

static ssize_t getFieldDescriptorIndex(const NodeVector<FieldDescriptor> &fds,
                                       const std::string &name)
{
	if (fds.empty()) {
		return -1;
	}

	if (name == DEFAULT_FIELD_NAME) {
		if (fds.back()->getFieldType() == FieldDescriptor::FieldType::TREE) {
			return fds.size() - 1;
		} else {
			/* The last field has to be the TREE field. If the last field does
			 * not have the FieldType TREE no TREE-field exists at all. So we
			 * return -1.
			 */
			return -1;
		}
	}

	for (size_t f = 0; f < fds.size(); f++) {
		if (fds[f]->getName() == name) {
			return f;
		}
	}
	return -1;
}

ssize_t Descriptor::getFieldDescriptorIndex(const std::string &name) const
{
	NodeVector<FieldDescriptor> fds = getFieldDescriptors();
	return ousia::getFieldDescriptorIndex(fds, name);
}

ssize_t Descriptor::getFieldDescriptorIndex(Handle<FieldDescriptor> fd) const
{
	size_t f = 0;
	for (auto &fd2 : getFieldDescriptors()) {
		if (fd == fd2) {
			return f;
		}
		f++;
	}
	return -1;
}

Rooted<FieldDescriptor> Descriptor::getFieldDescriptor(
    const std::string &name) const
{
	NodeVector<FieldDescriptor> fds = getFieldDescriptors();
	ssize_t idx = ousia::getFieldDescriptorIndex(fds, name);
	if (idx != -1) {
		return fds[idx];
	} else {
		return nullptr;
	}
}

void Descriptor::addAndSortFieldDescriptor(Handle<FieldDescriptor> fd,
                                           Logger &logger)
{
	// only add it if we need to.
	auto fds = getFieldDescriptors();
	if (fds.find(fd) == fds.end()) {
		invalidate();
		// check if the previous field is a tree field already.
		if (!fds.empty() &&
		    fds.back()->getFieldType() == FieldDescriptor::FieldType::TREE &&
		    fd->getFieldType() != FieldDescriptor::FieldType::TREE) {
			// if so we add the new field before the TREE field and log a
			// warning.

			logger.warning(
			    std::string("Field \"") + fd->getName() +
			        "\" was declared after main field \"" +
			        fds.back()->getName() +
			        "\". The order of fields was changed to make the "
			        "main field the last field.",
			    *fd);
			fieldDescriptors.insert(fieldDescriptors.end() - 1, fd);
		} else {
			fieldDescriptors.push_back(fd);
		}
	}
}

void Descriptor::addFieldDescriptor(Handle<FieldDescriptor> fd, Logger &logger)
{
	addAndSortFieldDescriptor(fd, logger);
	if (fd->getParent() == nullptr) {
		fd->setParent(this);
	}
}

void Descriptor::moveFieldDescriptor(Handle<FieldDescriptor> fd, Logger &logger)
{
	addAndSortFieldDescriptor(fd, logger);
	Handle<Managed> par = fd->getParent();
	if (par != this) {
		if (par != nullptr) {
			// remove the FieldDescriptor from the old parent.
			par.cast<Descriptor>()->removeFieldDescriptor(fd);
		}
		fd->setParent(this);
	}
}

void Descriptor::copyFieldDescriptor(Handle<FieldDescriptor> fd, Logger &logger)
{
	Rooted<FieldDescriptor> copy;
	if (fd->isPrimitive()) {
		copy = Rooted<FieldDescriptor>{new FieldDescriptor(
		    getManager(), fd->getPrimitiveType(), this, fd->getFieldType(),
		    fd->getName(), fd->isOptional())};
	} else {
		/*
		 * In case of non-primitive FieldDescriptors we also want to copy the
		 * child references.
		 */
		copy = Rooted<FieldDescriptor>{
		    new FieldDescriptor(getManager(), this, fd->getFieldType(),
		                        fd->getName(), fd->isOptional())};
		for (auto &c : fd->getChildren()) {
			copy->addChild(c);
		}
	}
	addFieldDescriptor(copy, logger);
}

bool Descriptor::removeFieldDescriptor(Handle<FieldDescriptor> fd)
{
	auto it = fieldDescriptors.find(fd);
	if (it != fieldDescriptors.end()) {
		invalidate();
		fieldDescriptors.erase(it);
		fd->setParent(nullptr);
		return true;
	}
	return false;
}

Rooted<FieldDescriptor> Descriptor::createPrimitiveFieldDescriptor(
    Handle<Type> primitiveType, Logger &logger,
    FieldDescriptor::FieldType fieldType, std::string name, bool optional)
{
	Rooted<FieldDescriptor> fd{new FieldDescriptor(getManager(), primitiveType,
	                                               this, fieldType,
	                                               std::move(name), optional)};
	addFieldDescriptor(fd, logger);
	return fd;
}

Rooted<FieldDescriptor> Descriptor::createFieldDescriptor(
    Logger &logger, FieldDescriptor::FieldType fieldType, std::string name,
    bool optional)
{
	Rooted<FieldDescriptor> fd{new FieldDescriptor(
	    getManager(), this, fieldType, std::move(name), optional)};
	addFieldDescriptor(fd, logger);
	return fd;
}

/* Class StructuredClass */

StructuredClass::StructuredClass(Manager &mgr, std::string name,
                                 Handle<Domain> domain, Variant cardinality,
                                 Handle<StructuredClass> superclass,
                                 bool transparent, bool root)
    : Descriptor(mgr, std::move(name), domain),
      cardinality(cardinality),
      superclass(acquire(superclass)),
      subclasses(this),
      transparent(transparent),
      root(root)
{
	ExceptionLogger logger;
	if (superclass != nullptr) {
		superclass->addSubclass(this, logger);
	}
	if (domain != nullptr) {
		domain->addStructuredClass(this);
	}
}

bool StructuredClass::doValidate(Logger &logger) const
{
	bool valid = true;
	// check if all registered subclasses have this StructuredClass as parent.
	for (Handle<StructuredClass> sub : subclasses) {
		if (sub->getSuperclass() != this) {
			logger.error(std::string("Struct \"") + sub->getName() +
			                 "\" is registered as subclass of \"" + getName() +
			                 "\" but does not have it as superclass!",
			             *this);
			valid = false;
		}
	}
	// check the cardinality.
	if (!cardinality.isCardinality()) {
		logger.error(cardinality.toString() + " is not a cardinality!", *this);
		valid = false;
	}
	// check the validity of this superclass.
	if (superclass != nullptr) {
		valid = valid & superclass->validate(logger);
	}
	// check the validity as a Descriptor.
	/*
	 * Note that we do not check the validity of all subclasses. This is because
	 * it will lead to cycles as the subclasses would call validate on their
	 * superclass, which is this one.
	 */
	return valid & Descriptor::doValidate(logger);
}

void StructuredClass::setSuperclass(Handle<StructuredClass> sup, Logger &logger)
{
	if (superclass == sup) {
		return;
	}
	// remove this subclass from the old superclass.
	if (superclass != nullptr) {
		superclass->removeSubclass(this, logger);
	}
	// set the new superclass
	superclass = acquire(sup);
	invalidate();
	// add this class as new subclass of the new superclass.
	if (sup != nullptr) {
		sup->addSubclass(this, logger);
		// set the attribute descriptor supertype
		getAttributesDescriptor()->setParentStructure(
		    sup->getAttributesDescriptor(), logger);
	} else {
		getAttributesDescriptor()->setParentStructure(nullptr, logger);
	}
}

bool StructuredClass::isSubclassOf(Handle<StructuredClass> c) const
{
	if (c == nullptr || superclass == nullptr) {
		return false;
	}
	if (c == superclass) {
		return true;
	}
	return superclass->isSubclassOf(c);
}

void StructuredClass::addSubclass(Handle<StructuredClass> sc, Logger &logger)
{
	if (sc == nullptr) {
		return;
	}
	// check if we already have that class.
	if (subclasses.find(sc) == subclasses.end()) {
		invalidate();
		subclasses.push_back(sc);
	}
	sc->setSuperclass(this, logger);
}

void StructuredClass::removeSubclass(Handle<StructuredClass> sc, Logger &logger)
{
	// if we don't have this subclass we can return directly.
	if (sc == nullptr) {
		return;
	}
	auto it = subclasses.find(sc);
	if (it == subclasses.end()) {
		return;
	}
	// otherwise we have to erase it.
	invalidate();
	subclasses.erase(it);
	sc->setSuperclass(nullptr, logger);
}

void StructuredClass::gatherFieldDescriptors(
    NodeVector<FieldDescriptor> &current,
    std::set<std::string> &overriddenFields, bool hasTREE) const
{
	// append all FieldDescriptors that are not overridden.
	for (auto &f : Descriptor::getFieldDescriptors()) {
		if (overriddenFields.insert(f->getName()).second) {
			bool isTREE = f->getFieldType() == FieldDescriptor::FieldType::TREE;
			if (hasTREE) {
				if (!isTREE) {
					/*
					 * If we already have a tree field it has to be at the end
					 * of the current vector. So ensure that all new non-TREE
					 * fields are inserted before the TREE field such that after
					 * this method the TREE field is still at the end.
					 */
					current.insert(current.end() - 1, f);
				}
			} else {
				if (isTREE) {
					hasTREE = true;
				}
				current.push_back(f);
			}
		}
	}
	// if we have a superclass, go there.
	if (superclass != nullptr) {
		superclass->gatherFieldDescriptors(current, overriddenFields, hasTREE);
	}
}

NodeVector<FieldDescriptor> StructuredClass::getFieldDescriptors() const
{
	// in this case we return a NodeVector of Rooted entries without owner.
	NodeVector<FieldDescriptor> vec;
	std::set<std::string> overriddenFields;
	gatherFieldDescriptors(vec, overriddenFields, false);
	return vec;
}

/* Class AnnotationClass */

AnnotationClass::AnnotationClass(Manager &mgr, std::string name,
                                 Handle<Domain> domain)
    : Descriptor(mgr, std::move(name), domain)
{
	if (domain != nullptr) {
		domain->addAnnotationClass(this);
	}
}

/* Class Domain */

void Domain::doResolve(ResolutionState &state)
{
	if (!continueResolveComposita(structuredClasses,
	                              structuredClasses.getIndex(), state) |
	    continueResolveComposita(annotationClasses,
	                             annotationClasses.getIndex(), state)) {
		continueResolveReferences(typesystems, state);
	}
}

bool Domain::doValidate(Logger &logger) const
{
	// check validity of name, of StructuredClasses, of AnnotationClasses and
	// TypeSystems.
	return validateName(logger) &
	       continueValidationCheckDuplicates(structuredClasses, logger) &
	       continueValidationCheckDuplicates(annotationClasses, logger) &
	       continueValidationCheckDuplicates(typesystems, logger);
}

void Domain::doReference(Handle<Node> node)
{
	if (node->isa(&RttiTypes::Domain)) {
		referenceTypesystem(node.cast<Typesystem>());
	}
}

RttiSet Domain::doGetReferenceTypes() const
{
	return RttiSet{&RttiTypes::Domain};
}

void Domain::addStructuredClass(Handle<StructuredClass> s)
{
	// only add it if we need to.
	if (structuredClasses.find(s) == structuredClasses.end()) {
		invalidate();
		structuredClasses.push_back(s);
	}
	Handle<Managed> par = s->getParent();
	if (par != this) {
		if (par != nullptr) {
			// remove the StructuredClass from the old parent.
			par.cast<Domain>()->removeStructuredClass(s);
		}
		s->setParent(this);
	}
}

bool Domain::removeStructuredClass(Handle<StructuredClass> s)
{
	auto it = structuredClasses.find(s);
	if (it != structuredClasses.end()) {
		invalidate();
		structuredClasses.erase(it);
		s->setParent(nullptr);
		return true;
	}
	return false;
}

Rooted<StructuredClass> Domain::createStructuredClass(
    std::string name, Variant cardinality, Handle<StructuredClass> superclass,
    bool transparent, bool root)
{
	return Rooted<StructuredClass>{new StructuredClass(
	    getManager(), std::move(name), this, cardinality, superclass,
	    std::move(transparent), std::move(root))};
}

void Domain::addAnnotationClass(Handle<AnnotationClass> a)
{
	// only add it if we need to.
	if (annotationClasses.find(a) == annotationClasses.end()) {
		invalidate();
		annotationClasses.push_back(a);
	}
	Handle<Managed> par = a->getParent();
	if (par != this) {
		if (par != nullptr) {
			// remove the StructuredClass from the old parent.
			par.cast<Domain>()->removeAnnotationClass(a);
		}
		a->setParent(this);
	}
}

bool Domain::removeAnnotationClass(Handle<AnnotationClass> a)
{
	auto it = annotationClasses.find(a);
	if (it != annotationClasses.end()) {
		invalidate();
		annotationClasses.erase(it);
		a->setParent(nullptr);
		return true;
	}
	return false;
}

Rooted<AnnotationClass> Domain::createAnnotationClass(std::string name)
{
	return Rooted<AnnotationClass>{
	    new AnnotationClass(getManager(), std::move(name), this)};
}

/* Type registrations */

namespace RttiTypes {
const Rtti FieldDescriptor =
    RttiBuilder<ousia::FieldDescriptor>("FieldDescriptor").parent(&Node);
const Rtti Descriptor =
    RttiBuilder<ousia::Descriptor>("Descriptor").parent(&Node);
const Rtti StructuredClass =
    RttiBuilder<ousia::StructuredClass>("StructuredClass")
        .parent(&Descriptor)
        .composedOf(&FieldDescriptor);
const Rtti AnnotationClass =
    RttiBuilder<ousia::AnnotationClass>("AnnotationClass").parent(&Descriptor);
const Rtti Domain = RttiBuilder<ousia::Domain>("Domain")
                        .parent(&RootNode)
                        .composedOf({&StructuredClass, &AnnotationClass});
}
}
