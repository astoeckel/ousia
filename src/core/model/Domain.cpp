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
		logger.error("This Descriptor has no Attribute specification!");
		valid = false;
	} else {
		if (attributesDescriptor->hasAttribute("name")) {
			logger.error(
			    "This Descriptor has an attribute \"name\" which is a reserved "
			    "word!");
			valid = false;
		}
		valid = valid & attributesDescriptor->validate(logger);
	}

	// check attributes and the FieldDescriptors
	return valid & continueValidationCheckDuplicates(fieldDescriptors, logger);
}

std::vector<Rooted<Node>> Descriptor::pathTo(
    Handle<StructuredClass> target) const
{
	std::vector<Rooted<Node>> path;
	continuePath(target, path);
	return path;
}

bool Descriptor::continuePath(Handle<StructuredClass> target,
                              std::vector<Rooted<Node>> &currentPath) const
{
	// check if we are at the target already
	if (this == target) {
		return true;
	}
	// a variable to determine if we already found a solution
	bool found = false;
	// the currently optimal path.
	std::vector<Rooted<Node>> optimum;
	// use recursive depth-first search from the top to reach the given child
	// get the list of effective FieldDescriptors.
	NodeVector<FieldDescriptor> fields = getFieldDescriptors();

	for (auto &fd : fields) {
		for (auto &c : fd->getChildren()) {
			// check if a child is the target node.
			if (c == target) {
				// if we have made the connection, stop the search.
				currentPath.push_back(fd);
				return true;
			}
			// look for transparent intermediate nodes.
			if (c->isTransparent()) {
				// copy the path.
				std::vector<Rooted<Node>> cPath = currentPath;
				cPath.push_back(fd);
				cPath.push_back(c);
				// recursion.
				if (c->continuePath(target, cPath) &&
				    (!found || optimum.size() > cPath.size())) {
					// look if this path is better than the current optimum.
					optimum = std::move(cPath);
					found = true;
				}
			}
		}
	}

	if (isa(&RttiTypes::StructuredClass)) {
		const StructuredClass *tis = static_cast<const StructuredClass *>(this);
		// if this is a StructuredClass we also can call the subclasses.
		for (auto &c : tis->getSubclasses()) {
			// copy the path.
			std::vector<Rooted<Node>> cPath = currentPath;
			if (c->continuePath(target, cPath) &&
			    (!found || optimum.size() > cPath.size())) {
				// look if this path is better than the current optimum.
				optimum = std::move(cPath);
				found = true;
			}
		}
	}

	// put the optimum in the given path reference.
	currentPath = std::move(optimum);

	// return if we found something.
	return found;
}

ssize_t Descriptor::getFieldDescriptorIndex(const std::string &name) const
{
	size_t f = 0;
	for (auto &fd : getFieldDescriptors()) {
		if (fd->getName() == name) {
			return f;
		}
		f++;
	}
	return -1;
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
	for (auto &fd : getFieldDescriptors()) {
		if (fd->getName() == name) {
			return fd;
		}
	}
	return nullptr;
}

void Descriptor::addFieldDescriptor(Handle<FieldDescriptor> fd)
{
	// only add it if we need to.
	if (fieldDescriptors.find(fd) == fieldDescriptors.end()) {
		invalidate();
		fieldDescriptors.push_back(fd);
	}
	if (fd->getParent() == nullptr) {
		fd->setParent(this);
	}
}

void Descriptor::moveFieldDescriptor(Handle<FieldDescriptor> fd)
{
	// only add it if we need to.
	if (fieldDescriptors.find(fd) == fieldDescriptors.end()) {
		invalidate();
		fieldDescriptors.push_back(fd);
	}
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

const void StructuredClass::gatherFieldDescriptors(
    NodeVector<FieldDescriptor> &current,
    std::set<std::string> &overriddenFields) const
{
	// append all FieldDescriptors that are not overridden.
	for (auto &f : Descriptor::getFieldDescriptors()) {
		if (overriddenFields.insert(f->getName()).second) {
			current.push_back(f);
		}
	}
	if (superclass != nullptr) {
		superclass->gatherFieldDescriptors(current, overriddenFields);
	}
}

NodeVector<FieldDescriptor> StructuredClass::getFieldDescriptors() const
{
	// in this case we return a NodeVector of Rooted entries without owner.
	NodeVector<FieldDescriptor> vec;
	std::set<std::string> overriddenFields;
	gatherFieldDescriptors(vec, overriddenFields);
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
