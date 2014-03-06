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

#ifndef _OUSIA_MODEL_DOMAIN_ANNOTATIONREFERENCE_HPP_
#define _OUSIA_MODEL_DOMAIN_ANNOTATIONREFERENCE_HPP_

#include <string>

namespace ousia {
namespace model {
namespace domain {

/**
 * A AnnotationReference is an expression resolvable to either a single
 * annotation or any annotation in some given category (* expression).
 */
class AnnotationReference {

private:
	std::string domainName;
	std::string categoryName;
	/**
	 * The annotation name might also be "any".
	 */
	std::string className;

public:

	const std::string& getDomainName()
	{
		return domainName;
	}

	const std::string& getCategoryName()
	{
		return categoryName;
	}

	const std::string& getClassName()
	{
		return className;
	}

};

}
}
}

#endif /* _OUSIA_MODEL_DOMAIN_ANNOTATIONREFERENCE_HPP_ */

