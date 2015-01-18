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
namespace model {

template <class T>
static void checkUniqueName(Handle<Node> parent, NodeVector<T> vec,
                            Handle<T> child, const std::string &parentClassName,
                            const std::string &childClassName)
{
	std::set<std::string> childNames;
	for (auto &c : vec) {
		childNames.insert(c->getName());
	}
	if (childNames.find(child->getName()) != childNames.end()) {
		// TODO: Do we really want to have an exception here?
		throw OusiaException(std::string("The ") + parentClassName + " " +
		                     parent->getName() + " already has a " +
		                     childClassName + " with name " + child->getName());
	}
}

/* Class FieldDescriptor */

FieldDescriptor::FieldDescriptor(Manager &mgr, Handle<Descriptor> parent,
                                 Handle<Type> primitiveType, std::string name,
                                 bool optional)
    : Node(mgr, std::move(name), parent),
      children(this),
      fieldType(FieldType::PRIMITIVE),
      primitiveType(acquire(primitiveType)),
      optional(optional)
{
	parent->addFieldDescriptor(this);
}

FieldDescriptor::FieldDescriptor(Manager &mgr, Handle<Descriptor> parent,
                                 FieldType fieldType, std::string name,
                                 bool optional)
    : Node(mgr, std::move(name), parent),
      children(this),
      fieldType(fieldType),
      optional(optional)
{
	parent->addFieldDescriptor(this);
}

/* Class Descriptor */

void Descriptor::doResolve(ResolutionState &state)
{
	if (attributesDescriptor != nullptr) {
		const NodeVector<Attribute> &attributes =
		    attributesDescriptor->getAttributes();
		continueResolveComposita(attributes, attributes.getIndex(), state);
	}
	continueResolveComposita(fieldDescriptors, fieldDescriptors.getIndex(),
	                         state);
}

void Descriptor::addFieldDescriptor(Handle<FieldDescriptor> fd)
{
	checkUniqueName(this, fieldDescriptors, fd, "Descriptor",
	                "FieldDescriptor");
	fieldDescriptors.push_back(fd);
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
	NodeVector<FieldDescriptor> fields;
	if (isa(RttiTypes::StructuredClass)) {
		const StructuredClass *tis = static_cast<const StructuredClass *>(this);
		fields = tis->getEffectiveFieldDescriptors();
	} else {
		fields = getFieldDescriptors();
	}

	for (auto &fd : fields) {
		for (auto &c : fd->getChildren()) {
			// check if a child is the target node.
			if (c == target) {
				// if we have made the connection, stop the search.
				currentPath.push_back(fd);
				return true;
			}
			// look for transparent intermediate nodes.
			if (c->transparent) {
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

	if (isa(RttiTypes::StructuredClass)) {
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

void Descriptor::copyFieldDescriptor(Handle<FieldDescriptor> fd)
{
	if (fd->getFieldType() == FieldDescriptor::FieldType::PRIMITIVE) {
		/*
		 *To call the "new" operation is enough here, because the
		 * constructor will add the newly constructed FieldDescriptor to this
		 * Descriptor automatically.
		 */
		new FieldDescriptor(getManager(), this, fd->getPrimitiveType(),
		                    fd->getName(), fd->optional);
	} else {
		new FieldDescriptor(getManager(), this, fd->getFieldType(),
		                    fd->getName(), fd->optional);
	}
}

/* Class StructuredClass */

StructuredClass::StructuredClass(Manager &mgr, std::string name,
                                 Handle<Domain> domain,
                                 const Cardinality &cardinality,
                                 Handle<StructType> attributesDescriptor,
                                 Handle<StructuredClass> superclass,
                                 bool transparent, bool root)
    : Descriptor(mgr, std::move(name), domain, attributesDescriptor),
      cardinality(cardinality),
      superclass(acquire(superclass)),
      subclasses(this),
      transparent(transparent),
      root(root)
{
	if (superclass != nullptr) {
		superclass->subclasses.push_back(this);
	}
	if (!domain.isNull()) {
		domain->addStructuredClass(this);
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

NodeVector<FieldDescriptor> StructuredClass::getEffectiveFieldDescriptors()
    const
{
	// in this case we return a NodeVector of Rooted entries without owner.
	NodeVector<FieldDescriptor> vec;
	std::set<std::string> overriddenFields;
	gatherFieldDescriptors(vec, overriddenFields);
	return std::move(vec);
}

/* Class AnnotationClass */

AnnotationClass::AnnotationClass(
    Manager &mgr, std::string name, Handle<Domain> domain,
    // TODO: What would be a wise default value for attributes?
    Handle<StructType> attributesDescriptor)
    : Descriptor(mgr, std::move(name), domain, attributesDescriptor)
{
	if (!domain.isNull()) {
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

void Domain::addStructuredClass(Handle<StructuredClass> s)
{
	checkUniqueName(this, structuredClasses, s, "Domain", "StructuredClass");
	structuredClasses.push_back(s);
}

void Domain::addAnnotationClass(Handle<AnnotationClass> a)
{
	checkUniqueName(this, annotationClasses, a, "Domain", "AnnotationClass");
	annotationClasses.push_back(a);
}
}
/* Type registrations */

namespace RttiTypes {
const RttiType FieldDescriptor =
    RttiBuilder<model::FieldDescriptor>("FieldDescriptor").parent(&Node);
const RttiType Descriptor =
    RttiBuilder<model::Descriptor>("Descriptor").parent(&Node);
const RttiType StructuredClass =
    RttiBuilder<model::StructuredClass>("StructuredClass").parent(&Descriptor).composedOf(
        &FieldDescriptor);
const RttiType AnnotationClass =
    RttiBuilder<model::AnnotationClass>("AnnotationClass").parent(&Descriptor);
const RttiType Domain =
    RttiBuilder<model::Domain>("Domain").parent(&Node).composedOf(
        {&StructuredClass, &AnnotationClass});
}
}

