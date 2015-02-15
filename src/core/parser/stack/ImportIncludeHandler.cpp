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

#include <core/model/RootNode.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

#include "DomainHandler.hpp"
#include "DocumentHandler.hpp"
#include "ImportIncludeHandler.hpp"
#include "State.hpp"
#include "TypesystemHandler.hpp"

namespace ousia {
namespace parser_stack {

/* ImportHandler */

void ImportHandler::doHandle(const Variant &fieldData, Variant::mapType &args)
{
	// Fetch the last node and check whether an import is valid at this
	// position
	Rooted<Node> leaf = scope().getLeaf();
	if (leaf == nullptr || !leaf->isa(&RttiTypes::RootNode)) {
		logger().error(
		    "Import not supported here, must be inside a document, domain "
		    "or typesystem command.",
		    location());
		return;
	}
	Rooted<RootNode> leafRootNode = leaf.cast<RootNode>();

	// Perform the actual import, register the imported node within the leaf
	// node
	Rooted<Node> imported = context().import(
	    fieldData.asString(), args["type"].asString(), args["rel"].asString(),
	    leafRootNode->getReferenceTypes());
	if (imported != nullptr) {
		leafRootNode->reference(imported);
	}
}

/* IncludeHandler */

void IncludeHandler::doHandle(const Variant &fieldData, Variant::mapType &args)
{
	context().include(fieldData.asString(), args["type"].asString(),
	                  args["rel"].asString(), {&RttiTypes::Node});
}

namespace States {
const State Import =
    StateBuilder()
        .parents({&Document, &Typesystem, &Domain})
        .elementHandler(ImportHandler::create)
        .arguments({Argument::String("rel", ""), Argument::String("type", ""),
                    Argument::String("src", "")});

const State Include =
    StateBuilder()
        .parent(&All)
        .elementHandler(IncludeHandler::create)
        .arguments({Argument::String("rel", ""), Argument::String("type", ""),
                    Argument::String("src", "")});
}
}
}
