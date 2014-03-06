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

#ifndef _OUSIA_MODEL_DOMAIN_STRUCTURE_HPP_
#define _OUSIA_MODEL_DOMAIN_STRUCTURE_HPP_

#include <memory>
#include <string>

#include <model/GraphNode.hpp>

namespace ousia {
namespace model {
namespace domain {

class Structure : public GraphNode {

public:
	Structure(std::shared_ptr<GraphNode> parent = nullptr,
			const std::string &name = "") :
		GraphNode(GraphNodeType::Structure, parent, name)
	{
		// Do nothing here
	}


};
}
}
}

#endif /* _OUSIA_MODEL_DOMAIN_STRUCTURE_HPP_ */
