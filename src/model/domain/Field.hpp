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

#ifndef _OUSIA_MODEL_DOMAIN_FIELD_HPP_
#define _OUSIA_MODEL_DOMAIN_FIELD_HPP_

#include <memory>

#include <model/GraphNode.hpp>
#include <model/types/Type.hpp>

namespace ousia{

//namespace types {
//	class Type;
//	class Value;
//}
namespace model {
namespace domain {

class Field : public GraphNode {

private:
	std::shared_ptr<types::Type> type;
	bool optional;

public:
	using GraphNode::GraphNode;

	std::shared_ptr<types::Type> getType()
	{
		return type;
	}

	void setType(std::shared_ptr<types::Type> type)
	{
		this->type = type;
	}

	bool getOptional()
	{
		return optional;
	}

	void setOptional(bool optional)
	{
		this->optional = optional;
	}
};
}
}
}

#endif /* _OUSIA_MODEL_DOMAIN_FIELD_HPP_ */
