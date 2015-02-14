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

#include "ImportIncludeHandler.hpp"

#include <core/parser/ParserScope.hpp>

namespace ousia {

/* ImportIncludeHandler */

void ImportIncludeHandler::start(Variant::mapType &args)
{
	rel = args["rel"].asString();
	type = args["type"].asString();
	src = args["src"].asString();
	srcInArgs = !src.empty();
}

void ImportIncludeHandler::data(const std::string &data, int field)
{
	if (srcInArgs) {
		logger().error("\"src\" attribute has already been set");
		return;
	}
	if (field != 0) {
		logger().error("Command has only one field.");
		return;
	}
	src.append(data);
}

/* ImportHandler */

void ImportHandler::start(Variant::mapType &args)
{
	ImportIncludeHandler::start(args);

	// Make sure imports are still possible
	if (scope().getFlag(ParserFlag::POST_HEAD)) {
		logger().error("Imports must be listed before other commands.",
		               location());
		return;
	}
}

void ImportHandler::end()
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
	Rooted<Node> imported =
	    context().import(src, type, rel, leafRootNode->getReferenceTypes());
	if (imported != nullptr) {
		leafRootNode->reference(imported);
	}
}

/* IncludeHandler */

void IncludeHandler::start(Variant::mapType &args)
{
	ImportIncludeHandler::start(args);
}

void IncludeHandler::end()
{
	context().include(src, type, rel, {&RttiTypes::Node});
}
}
