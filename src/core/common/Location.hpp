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

#ifndef _OUSIA_LOCATION_HPP_
#define _OUSIA_LOCATION_HPP_

namespace ousia {

/**
 * Struct representing a location within a source file. A position is defined by
 * a byte offset (which is always reproducable), a line number and a column
 * number (which may differ depending on the encoding used).
 */
struct SourceLocation {
	/**
	 * Current line, starting with one.
	 */
	int line;

	/**
	 * Current column, starting with one.
	 */
	int column;

	/**
	 * Current byte offset.
	 */
	size_t offs;

	/**
	 * Default constructor of the SourceLocation struct, initializes all
	 * memebers with zero.
	 */
	SourceLocation() : line(0), column(0), offs(0) {}

	/**
	 * Creates a new SourceLocation struct with only a line and no column.
	 *
	 * @param line is the line number.
	 * @param column is the column number.
	 */
	SourceLocation(int line) : line(line), column(0), offs(0) {}

	/**
	 * Creates a new SourceLocation struct with a line and column.
	 *
	 * @param line is the line number.
	 * @param column is the column number.
	 */
	SourceLocation(int line, int column) : line(line), column(column), offs(0)
	{
	}

	/**
	 * Creates a new SourceLocation struct with a line, column and byte offset.
	 *
	 * @param line is the line number.
	 * @param column is the column number.
	 * @param offs is the byte offset.
	 */
	SourceLocation(int line, int column, size_t offs)
	    : line(line), column(column), offs(offs)
	{
	}

	/**
	 * Returns true, if the line number is valid, false otherwise.
	 *
	 * @return true for valid line numbers.
	 */
	bool hasLine() const { return line > 0; }

	/**
	 * Returns true, if the column number is valid, false otherwise.
	 *
	 * @return true for valid column numbers.
	 */
	bool hasColumn() const { return column > 0; }

	/**
	 * Returns true, if the position is valid, false otherwise. This function is
	 * equivalent to the hasLine() function.
	 *
	 * @return true if the Position struct is valid.
	 */
	bool valid() const { return hasLine(); }
};

/**
 * Represents the context of a SourceLocation instance. Used to build error
 * messages.
 */
struct SourceContext {
	/**
	 * Set to the content of the current line.
	 */
	std::string text;

	/**
	 * Relative position (in characters) within that line. May point to
	 * locations beyond the text content.
	 */
	int relPos;

	/**
	 * Set to true if the beginning of the line has been truncated (because
	 * the reader position is too far away from the actual position of the
	 * line).
	 */
	bool truncatedStart;

	/**
	 * Set to true if the end of the line has been truncated (because the
	 * reader position is too far away from the actual end position of the
	 * line.
	 */
	bool truncatedEnd;

	/**
	 * Default constructor, initializes all members with zero values.
	 */
	SourceContext()
	    : text(), relPos(0), truncatedStart(false), truncatedEnd(false)
	{
	}

	/**
	 * Constructor of the SourceContext class.
	 *
	 * @param text is the current line the text cursor is at.
	 * @param relPos is the relative position of the text cursor within that
	 * line.
	 * @param truncatedStart specifies whether the text was truncated at the
	 * beginning.
	 * @param truncatedEnd specifies whether the text was truncated at the
	 * end.
	 */
	SourceContext(std::string text, size_t relPos, bool truncatedStart,
	              bool truncatedEnd)
	    : text(std::move(text)),
	      relPos(relPos),
	      truncatedStart(truncatedStart),
	      truncatedEnd(truncatedEnd)
	{
	}

	/**
	 * Returns true the context text is not empty.
	 *
	 * @return true if the context is valid and e.g. should be printed.
	 */
	bool valid() const { return !text.empty(); }
};

/**
 * Callback used to lookup the context corresponding to the given source
 * location.
 *
 * @param location is the location for which the context should be looked up.
 * @param data is used defined data associated with the callback.
 */
using SourceContextCallback = SourceContext (*)(const SourceLocation &location,
                                                void *data);
}

#endif /* _OUSIA_LOCATION_HPP_ */

