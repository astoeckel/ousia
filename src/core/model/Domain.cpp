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

