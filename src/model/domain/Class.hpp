/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#ifndef _OUSIA_MODEL_DOMAIN_CLASS_HPP_
#define _OUSIA_MODEL_DOMAIN_CLASS_HPP_

#include <memory>
#include <string>
#include <vector>

#include <model/GraphNode.hpp>
#include "ClassReferenceSet.hpp"
#include "Field.hpp"
#include "Layer.hpp"

namespace ousia {
namespace model {
namespace domain {

/**
 * A class represents some semantic concept in a given domain that has
 * structural relevance, like headings in a text. Classes are usually expected
 * to be in a "tree-esque" structure: It is not really a tree, but we still
 * think about classes as nodes with children, even though children might be
 * nodes higher up the tree, which leads to cycles.
 */
class Class : public GraphNode {

private:

	std::vector<std::shared_ptr<ClassReferenceSet>> children;
	std::vector<std::shared_ptr<Field>> fields;
	std::vector<std::shared_ptr<Layer>> layers;

public:
	
	Class(std::shared_ptr<GraphNode> parent = nullptr,
			const std::string &name = "") :
		GraphNode(GraphNodeType::Class, parent, name)
	{
		// Do nothing here
	}

	/**
	 * The children of a given class are not resolved on parsing time but lazily
	 * during document creation and validation time. This circumvents some
	 * problems we would have otherwise like: How do we treat the case that
	 * merging two domains adds more possible classes to some given category?
	 * How do we treat references to linked domains?
	 *
	 * Thus we do not specify the children that are allowed but a sequence of
	 * sets that define what set of classes is allowed at each point in the
	 * children sequence. Please note that each ClassReferenceSet also stores
	 * a cardinality, how many children, that are members of this set, have to
	 * exist. Therefore this construction can be interpreted as a quasi finite
	 * state automaton, e.g.:
	 *
	 * (class1|class2)* (class3){1,4}
	 */
	std::vector<std::shared_ptr<ClassReferenceSet>>& getChildren()
	{
		return children;
	}

	std::vector<std::shared_ptr<Field>>& getFields()
	{
		return fields;
	}

	/**
	 * Layers specify the annotations that are allowed upon instances of this
	 * class and its children.
	 */
	std::vector<std::shared_ptr<Layer>>& getLayers()
	{
		return layers;
	}
};
}
}
}

#endif /* _OUSIA_MODEL_DOMAIN_CLASS_HPP_ */

