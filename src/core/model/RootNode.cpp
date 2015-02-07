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

#include <core/common/RttiBuilder.hpp>

#include "RootNode.hpp"

namespace ousia {

void RootNode::reference(Handle<Node> node)
{
	if (!node->type()->isOneOf(getReferenceTypes())) {
		throw OusiaException(
		    std::string("Node with type ") + node->type()->name +
		    std::string(" cannot be referenced in a ") + type()->name);
	}
	doReference(node);
}

RttiSet RootNode::getReferenceTypes() const { return doGetReferenceTypes(); }

namespace RttiTypes {
const Rtti RootNode = RttiBuilder<ousia::RootNode>("RootNode").parent(&Node);
}
}

