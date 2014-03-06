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

#ifndef _OUSIA_MODEL_DOMAIN_CLASSREFERENCESET_HPP_
#define _OUSIA_MODEL_DOMAIN_CLASSREFERENCESET_HPP_

#include "ClassReference.hpp"
#include <model/RangeSet.hpp>

namespace ousia {
namespace model {
namespace domain {

/**
 * A ClassReferenceSet lazily defines references to classes that are allowed at
 * this point of the domain description. It specifies a set in a twofold meaning:
 * 1.) It defines the set of classes, that are allowed.
 * 2.) It defines how many instances of those classes have to be instantiated
 *		in a document that implements this domain standard (cardinality).
 */
class ClassReferenceSet {

private:
	std::vector<std::shared_ptr<ClassReference>> conjunctions;
	std::shared_ptr<RangeSet<unsigned int>> cardinality;

public:

	/**
	 * This defines the conjunctions of references to classes that are allowed
	 * Please note that each ClassReference again does not have to reference to
	 * a single class but can also reference to multiple classes in a *
	 * expression.
	 */
	std::vector<std::shared_ptr<ClassReference>>& getConjunctions()
	{
		return conjunctions;
	}

	std::shared_ptr<RangeSet<unsigned int>> getCardinality()
	{
		return cardinality;
	}

	void setCardinality(std::shared_ptr<RangeSet<unsigned int>>)
	{
		this->cardinality = cardinality;
	}

};

}
}
}

#endif /* _OUSIA_MODEL_DOMAIN_CLASSREFERENCESET_HPP_ */

