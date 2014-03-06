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

#include <memory>
#include <string>
#include <vector>

#include <model/GraphNode.hpp>

#include "Structure.hpp"
#include "ClassCategory.hpp"
#include "AnnotationCategory.hpp"
#include "ClassReferenceSet.hpp"

namespace ousia {
namespace model {
namespace domain {

class Domain : public GraphNode {

private:
	std::shared_ptr<ClassReferenceSet> root;
	std::vector<std::shared_ptr<Structure>> structures;
	std::vector<std::shared_ptr<ClassCategory>> classCategories;
	std::vector<std::shared_ptr<AnnotationCategory>> annotationCategories;

public:

	Domain(std::shared_ptr<GraphNode> parent = nullptr,
			const std::string &name = "") :
		GraphNode(GraphNodeType::Domain, parent, name)
	{
		// Do nothing here
	}

	std::shared_ptr<ClassReferenceSet>& getRoot()
	{
		return root;
	}

	std::vector<std::shared_ptr<Structure>>& getStructures()
	{
		return structures;
	}

	std::vector<std::shared_ptr<ClassCategory>>& getClassCategories()
	{
		return classCategories;
	}

	std::vector<std::shared_ptr<AnnotationCategory>>& getAnnotationCategories()
	{
		return annotationCategories;
	}

};

}
}
}

#endif /* _OUSIA_MODEL_DOMAIN_DOMAIN_HPP_ */

