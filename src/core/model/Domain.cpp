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

#include <core/common/Rtti.hpp>
#include <core/common/Exceptions.hpp>

#include "Domain.hpp"

namespace ousia {
namespace model {

/* Class FieldDescriptor */

/* Class Descriptor */

void Descriptor::continueResolve(ResolutionState &state)
{
	if (attributesDescriptor != nullptr) {
		const NodeVector<Attribute> &attributes =
		    attributesDescriptor->getAttributes();
		continueResolveComposita(attributes, attributes.getIndex(), state);
	}
	continueResolveComposita(fieldDescriptors, fieldDescriptors.getIndex(),
	                         state);
}

std::vector<Rooted<Node>> Descriptor::pathTo(
    Handle<StructuredClass> target) const
{
	std::vector<Rooted<Node>> path;
	continuePath(target, path);
	return path;
}

static bool pathEquals(const Descriptor &a, const Descriptor &b)
{
	// We assume that two Descriptors are equal if their names and domain names
	// are equal.
	if (a.getName() != b.getName()) {
		return false;
	}
	Handle<Domain> aDom = a.getParent().cast<Domain>();
	Handle<Domain> bDom = b.getParent().cast<Domain>();
	return aDom->getName() == bDom->getName();
}

bool Descriptor::continuePath(Handle<StructuredClass> target,
                              std::vector<Rooted<Node>> &currentPath,
                              std::set<std::string> ignoredFields,
                              bool exploreSuperclass,
                              bool exploreSubclasses) const
{
	// TODO: REMOVE
	std::string targetName = target->getName();
	std::string thisName = getName();
	std::string currentPathName;
	for (auto &n : currentPath) {
		currentPathName += ".";
		currentPathName += n->getName();
	}
	// a variable to determine if we already found a solution
	bool found = false;
	// the currently optimal path.
	std::vector<Rooted<Node>> optimum;
	// use recursive depth-first search from the top to reach the given child
	for (auto &fd : fieldDescriptors) {
		if (!(ignoredFields.insert(fd->getName()).second)) {
			// if we want to ignore that field, we continue.
			continue;
		}
		for (auto &c : fd->getChildren()) {
			if (pathEquals(*c, *target)) {
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
		/*
		 * if this is a StructuredClass, we can also use the super class (at
		 * least for fields that are not overridden)
		 */
		if (exploreSuperclass && !tis->getIsA().isNull()) {
			// copy the path.
			std::vector<Rooted<Node>> cPath = currentPath;
			if (tis->getIsA()->continuePath(target, cPath, ignoredFields, true,
			                                false) &&
			    (!found || optimum.size() > cPath.size())) {
				// look if this path is better than the current optimum.
				optimum = std::move(cPath);
				found = true;
			}
		}

		// we also can call the subclasses.
		if (exploreSubclasses) {
			for (auto &c : tis->getSubclasses()) {
				// copy the path.
				std::vector<Rooted<Node>> cPath = currentPath;
				if (c->continuePath(target, cPath, {}, false) &&
				    (!found || optimum.size() > cPath.size())) {
					// look if this path is better than the current optimum.
					optimum = std::move(cPath);
					found = true;
				}
			}
		}
	}

	// put the optimum in the given path reference.
	currentPath = std::move(optimum);

	// return if we found something.
	return found;
}

/* Class Domain */

void Domain::continueResolve(ResolutionState &state)
{
	if (!continueResolveComposita(structuredClasses,
	                              structuredClasses.getIndex(), state) |
	    continueResolveComposita(annotationClasses,
	                             annotationClasses.getIndex(), state)) {
		continueResolveReferences(typesystems, state);
	}
}
}
/* Type registrations */

namespace RttiTypes {
const Rtti<model::FieldDescriptor> FieldDescriptor =
    RttiBuilder("FieldDescriptor").parent(&Node);
const Rtti<model::Descriptor> Descriptor =
    RttiBuilder("Descriptor").parent(&Node);
const Rtti<model::StructuredClass> StructuredClass =
    RttiBuilder("StructuredClass").parent(&Descriptor).composedOf(
        &FieldDescriptor);
const Rtti<model::AnnotationClass> AnnotationClass =
    RttiBuilder("AnnotationClass").parent(&Descriptor);
const Rtti<model::Domain> Domain =
    RttiBuilder("Domain").parent(&Node).composedOf(
        {&StructuredClass, &AnnotationClass});
}
}

