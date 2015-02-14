/*
    Ousía
    Copyright (C) 2014, 2015  Benjamin Paaßen, Andreas Stöckel

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
 * @file OsxmlAttributeLocator.hpp
 *
 * Contains a class used for locating the byte offsets of the attributes given
 * in a XML tag.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_OSXML_ATTRIBUTE_LOCATOR_HPP_
#define _OUSIA_OSXML_ATTRIBUTE_LOCATOR_HPP_

#include <map>

namespace ousia {

// Forward declarations
class CharReader;
class SourceLocation;

/**
 * Class containing one static function for locating the byte offsets of the
 * attributes in a XML tag. This are not retrieved by our xml parser, so we have
 * to do this manually.
 */
class OsxmlAttributeLocator {
public:
	/**
	 * Function used to reconstruct the location of the attributes of a XML tag
	 * in the source code. This is necessary, as the xml parser only returns an
	 * offset to the begining of a tag and not to the position of the individual
	 * arguments.
	 *
	 * @param reader is the char reader from which the character data should be
	 * read.
	 * @param offs is a byte offset in the xml file pointing at the "<"
	 * character of the tag.
	 * @return a map from attribute keys to the corresponding location
	 * (including range) of the atribute. Also contains the location of the
	 * tagname in the form of the virtual attribute "$tag".
	 */
	static std::map<std::string, SourceLocation> locate(CharReader &reader,
	                                                    size_t offs);
};

}

#endif /* _OUSIA_OSXML_ATTRIBUTE_LOCATOR_HPP_ */

