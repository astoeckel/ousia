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

#ifndef _ANNOTATION_HPP_
#define _ANNOTATION_HPP_

#include <memory>
#include <vector>

#include <model/GraphNode.hpp>
#include <model/domain/Anchor.hpp>
#include <model/domain/Structure.hpp>
#include <model/domain/Field.hpp>

namespace ousia {
namespace domain {

//class Structure;
//class Anchor;
//class Field;

class Annotation : public GraphNode {

private:
	std::vector<std::shared_ptr<Structure>> structures;
	std::vector<std::shared_ptr<Field>> fields;
	std::shared_ptr<Anchor> start;
	std::shared_ptr<Anchor> end;

public:
	using GraphNode::GraphNode;

	std::vector<std::shared_ptr<Structure>>& getStructures()
	{
		return structures;
	}

	std::vector<std::shared_ptr<Field>>& getFields()
	{
		return fields;
	}

	std::shared_ptr<Anchor> getStart()
	{
		return start;
	}

	void setStart(std::shared_ptr<Anchor> start)
	{
		this->start = start;
	}

	std::shared_ptr<Anchor> getEnd()
	{
		return end;
	}

	void setEnd(std::shared_ptr<Anchor> end)
	{
		this->end = end;
	}
};
}
}

#endif /* _ANNOTATION_HPP_ */
