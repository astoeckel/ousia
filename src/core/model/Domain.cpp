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
                              std::vector<Rooted<Node>> &path) const
{
	bool found = false;
	// use recursive depth-first search from the top to reach the given child
	if (fieldDescriptors.size() > 0) {
		for (auto &fd : fieldDescriptors) {
			for (auto &c : fd->getChildren()) {
				if (pathEquals(*c, *target)) {
					// if we have made the connection, stop the search.
					path.push_back(fd);
					return true;
				}
				// look for transparent intermediate nodes.
				if (c->transparent) {
					// copy the path.
					std::vector<Rooted<Node>> cPath = path;
					cPath.push_back(fd);
					cPath.push_back(c);
					// recursion.
					if (c->continuePath(target, cPath) &&
					    (!found || path.size() > cPath.size())) {
						// look if this path is better than the current optimum.
						path = std::move(cPath);
						found = true;
					}
				}
			}
		}
	} else {
		// if this is a StructuredClass and if it did not formulate own
		// fieldDescriptors (overriding the parent), we can also use the
		// (inheritance-wise) parent
		if (isa(RttiTypes::StructuredClass)) {
			const StructuredClass *tis =
			    static_cast<const StructuredClass *>(this);
			if (!tis->getIsA().isNull()) {
				// copy the path.
				std::vector<Rooted<Node>> cPath = path;
				if (tis->getIsA()->continuePath(target, cPath) &&
				    (found || path.size() > cPath.size())) {
					// look if this path is better than the current optimum.
					path = std::move(cPath);
					found = true;
				}
			}
		}
	}
	// either way we can try to find the targets parent (inheritance-wise)
	// instead of the target itself.
	if (!target->getIsA().isNull()) {
		// copy the path.
		std::vector<Rooted<Node>> cPath = path;
		if (continuePath(target->getIsA(), cPath) &&
		    (!found || path.size() > cPath.size())) {
			// look if this path is better than the current optimum.
			path = std::move(cPath);
			found = true;
		}
	}
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

