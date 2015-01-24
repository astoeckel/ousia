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
 * @file SourceContextReader.hpp
 *
 * The SourceContextReader class is used to read a SourceContext struct from
 * a SourcePosition instance and an input stream.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_SOURCE_CONTEXT_READER_HPP_
#define _OUSIA_SOURCE_CONTEXT_READER_HPP_

#include <string>
#include <vector>
#include <limits>

#include "Location.hpp"

namespace ousia {

// Forward declarations
class CharReader;

/**
 * The SourceContextReader can read SourceContext structures given a
 * SourcePosition or SourceRange and a char reader. It is capable of managing
 * a line number cache which speeds up repeated context lookups.
 */
class SourceContextReader {
private:
	/**
	 * Cache containing the byte offset of each line break.
	 */
	std::vector<SourceOffset> cache;

public:
	/**
	 * Maximum context size. Used to indicate that the context should have an
	 * unlimited size.
	 */
	static constexpr size_t MAX_MAX_CONTEXT_LENGTH =
	    std::numeric_limits<ssize_t>::max();

	/**
	 * Default constructor. Initializes the internal lineNumberCache with a
	 * single zero entry.
	 */
	SourceContextReader();

	/**
	 * Returns the context for the char reader and the given SourceRange.
	 * Returns an invalid source context if either the given range is invalid
	 * or the byte offset described in the SourceRange cannot be reached because
	 * the CharReader cannot be seeked back to this position.
	 *
	 * @param reader is the CharReader instance from which the context should be
	 * read.
	 * @param range describes the Range within the source file for which the
	 * context should be extraced.
	 * @param filename is the filename that should be stored in the returned
	 * context.
	 * @param maxContextLength is the maximum number of characters that should
	 * be stored in the returned context.
	 * @return a SourceContext instance describing the
	 */
	SourceContext readContext(CharReader &reader, const SourceRange &range,
	                          size_t maxContextLength = MAX_MAX_CONTEXT_LENGTH,
	                          const std::string &filename = "");
};
}

#endif /* _OUSIA_SOURCE_CONTEXT_READER_HPP_ */

