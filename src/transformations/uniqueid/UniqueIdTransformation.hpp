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

/**
 * @file UniqueIdTransformation.hpp
 *
 * Contains a transformation capable of generating unique ids for referenced
 * document nodes.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_UNIQUE_ID_TRANSFORMATION_HPP_
#define _OUSIA_UNIQUE_ID_TRANSFORMATION_HPP_

#include <core/model/Document.hpp>

namespace ousia {

/**
 * The UniqueIdTransformation class implements a transformation that attaches
 * unique ids to elements that are being referenced in the document. These
 * unique ids can for example be used in XML or HTML output.
 *
 * TODO: Write an actual base class for transformations and derive from it
 */
class UniqueIdTransformation {
public:
	/**
	 * Applys the transformation to the given document.
	 *
	 * @param doc is the document for which unique IDs should be generated.
	 */
	static void transform(Handle<Document> doc);
};

}

#endif /* _OUSIA_UNIQUE_ID_TRANSFORMATION_HPP_ */

