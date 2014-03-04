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

#ifndef _CLASS_HPP_
#define _CLASS_HPP_

#include <memory>
#include <vector>

#include <model/GraphNode.hpp>

namespace ousia {
namespace domain {

class Class : public GraphNode {

private:
	std::vector<std::shared_ptr<Class>> children;

};

}
}

#endif /* _CLASS_HPP_ */

