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

#ifndef _OUSIA_MODEL_DOMAIN_ANNOTATION_HPP_
#define _OUSIA_MODEL_DOMAIN_ANNOTATION_HPP_

#include <memory>
#include <string>
#include <vector>

#include <model/GraphNode.hpp>
#include "Structure.hpp"
#include "Field.hpp"

namespace ousia {
namespace model {
namespace domain {

//class Structure;
//class Field;

class Annotation : public GraphNode {

private:
	std::vector<std::shared_ptr<Structure>> structures;
	std::vector<std::shared_ptr<Field>> fields;

public:

	Annotation(std::shared_ptr<GraphNode> parent = nullptr,
			const std::string &name = "") :
		GraphNode(GraphNodeType::Annotation, parent, name)
	{
		// Do nothing here
	}

	std::vector<std::shared_ptr<Structure>>& getStructures()
	{
		return structures;
	}

	std::vector<std::shared_ptr<Field>>& getFields()
	{
		return fields;
	}
};
}
}
}
#endif /* _OUSIA_MODEL_DOMAIN_ANNOTATION_HPP_ */
