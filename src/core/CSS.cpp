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

#include "CSS.hpp"

namespace ousia {

/**
 * This returns all children of this SelectorNode that are connected by
 * the given operator, have the given className and the given
 * PseudoSelector.
 */
std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator &op, const std::string &className,
    const PseudoSelector &select)
{
	std::vector<Rooted<SelectorNode>> out;
	for(auto& e : edges){
		if(e->getSelectionOperator() != op){
			continue;
		}
		if(e->getTarget()->getName() != className){
			continue;
		}
		if(e->getTarget()->getPseudoSelector() != select){
			continue;
		}
		out.push_back(e->getTarget());
	}
	return out;
}


}
