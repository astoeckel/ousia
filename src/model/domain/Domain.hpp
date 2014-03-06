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

#ifndef _OUSIA_MODEL_DOMAIN_DOMAIN_HPP_
#define _OUSIA_MODEL_DOMAIN_DOMAIN_HPP_

//#include <memory>
//#include <string>
//#include <vector>

#include <model/GraphNode.hpp>

//#include "Class.hpp"
//#include "Structure.hpp"
//#include "Category.hpp"
//#include "Layer.hpp"

namespace ousia {
namespace model {
namespace domain {

class Domain : public GraphNode {

private:
//	std::shared_ptr<Class> root;
//	std::vector<std::shared_ptr<Structure>> structures;
//	std::vector<std::shared_ptr<Category>> categories;
//	std::vector<std::shared_ptr<Layer>> layers;

public:

	Domain(std::shared_ptr<GraphNode> parent = nullptr,
			const std::string &name = "") :
		GraphNode(GraphNodeType::Domain, parent, name)
	{
		// Do nothing here
	}

/*	std::shared_ptr<Class>& getRoot()
	{
		return root;
	}

	std::vector<std::shared_ptr<Structure>>& getStructures()
	{
		return structures;
	}

	std::vector<std::shared_ptr<Category>>& getCategories()
	{
		return categories;
	}

	std::vector<std::shared_ptr<Layer>>& getLayers()
	{
		return layers;
	}*/

};

}
}
}

#endif /* _OUSIA_MODEL_DOMAIN_DOMAIN_HPP_ */

