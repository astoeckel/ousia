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

#ifndef _OUSIA_MODEL_DOMAIN_LAYER_HPP_
#define _OUSIA_MODEL_DOMAIN_LAYER_HPP_

#include <memory>
#include <vector>

#include "AnnotationReference.hpp"

namespace ousia {
namespace model {
namespace domain {

/**
 * A Layer lazily defines references to annotations that are allowed upon
 * certain classes. You can interpret a layer as a ClassReferenceSet minus the
 * cardinality.
 */
class Layer {

private:
	std::vector<std::shared_ptr<AnnotationReference>> conjunctions;

public:

	/**
	 * This defines the conjunctions of references to annotations that are allowed
	 * Please note that each AnnotationReference again does not have to reference to
	 * a single class but can also reference to multiple classes in a *
	 * expression.
	 */
	std::vector<std::shared_ptr<AnnotationReference>>& getConjunctions()
	{
		return conjunctions;
	}
};

}
}
}

#endif /* _OUSIA_MODEL_DOMAIN_LAYER_HPP_ */

