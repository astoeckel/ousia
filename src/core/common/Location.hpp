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
 * @file Location.hpp
 *
 * Types used for describing positions, ranges and excerpts of source files used
 * for describing log messages.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_LOCATION_HPP_
#define _OUSIA_LOCATION_HPP_

#include <cstdint>
#include <functional>
#include <limits>
#include <string>

namespace ousia {

/**
 * Type used for referencing a source file currently opened in a Project.
 */
using SourceId = uint32_t;

/**
 * Maximum value for a SourceId. Indicates invalid entries.
 */
constexpr SourceId InvalidSourceId = std::numeric_limits<SourceId>::max();

/**
 * Type used for specifying an offset within a source file.
 */
using SourceOffset = uint32_t;

/**
 * Maximum value for a SourceOffset. As SourceOffset is a 32 Bit unsigned
 * integer, the maximum value is 2^32-1, which means that 4 GiB are addressable
 * by SourceOffset.
 */
constexpr SourceOffset InvalidSourceOffset =
    std::numeric_limits<SourceOffset>::max();

/**
 * Function for clamping a size_t to a valid SourceOffset value.
 *
 * @param pos is the size_t value that should be converted to a SourceOffset
 * value. If pos is larger than the maximum value that can be represented by
 * SourceOffset, the result is set to this maximum value, which is interpreted
 * as "invalid" by functions dealing with the SourceOffset type.
 * @return the clamped position value.
 */
inline SourceOffset clampToSourcePosition(size_t pos)
{
	return pos > InvalidSourceOffset ? InvalidSourceOffset : pos;
}

/**
 * Class specifying a position within an (unspecified) source file.
 */
class SourcePosition {
private:
	/**
	 * Offset position in bytes relative to the start of the document.
	 */
	SourceOffset pos;

public:
	/**
	 * Default constructor of the SourcePosition class. Sets the position to
	 * InvalidSourceOffset and thus marks the SourcePosition as invalid.
	 */
	SourcePosition() : pos(InvalidSourceOffset) {}

	/**
	 * Creates a new SourcePosition instance with the given byte offset.
	 */
	SourcePosition(size_t pos) : pos(clampToSourcePosition(pos)) {}

	/**
	 * Sets the position of the SourcePosition value to the given value. Clamps
	 * the given size_t to the valid range.
	 *
	 * @param pos is the position value that should be set.
	 */
	void setPosition(size_t pos) { this->pos = clampToSourcePosition(pos); }

	/**
	 * Returns the position value. Only use the value if "valid" returns true.
	 *
	 * @return the current position.
	 */
	SourceOffset getPosition() const { return pos; }

	/**
	 * Returns true if the source position is valid, false otherwise. Invalid
	 * positions are set to the maximum representable number.
	 *
	 * @return true if the SourcePosition instance is value, false otherwise.
	 */
	bool isValid() const { return pos != InvalidSourceOffset; }
};

/**
 * The SourceRange class represents a range within an (unspecified) source file.
 */
class SourceRange {
private:
	/**
	 * Start byte offset.
	 */
	SourcePosition start;

	/**
	 * End byte offset.
	 */
	SourcePosition end;

public:
	/**
	 * Default constructor. Creates an invalid range.
	 */
	SourceRange(){};

	/**
	 * Constructor for a zero-length range.
	 *
	 * @param pos is the byte offset at which the SourceRange instance should be
	 * located.
	 */
	SourceRange(SourcePosition pos) : start(pos), end(pos) {}

	/**
	 * Constructor of a SourceRange instance.
	 *
	 * @param start is the byte offset of the first character in the range
	 * (start is inclusive).
	 * @param end points at the end of the range (end is non-inclusive).
	 */
	SourceRange(SourcePosition start, SourcePosition end)
	    : start(start), end(end)
	{
	}

	/**
	 * Sets the start of the SourceRange value to the given value. This
	 * operation might render the SourceRange invalid (if the given position is
	 * larger than the end position).
	 *
	 * @param pos is the start position value that should be set.
	 */
	void setStart(SourcePosition pos) { this->start = pos; }

	/**
	 * Sets the end of the SourceRange value to the given value. This operation
	 * might render the SourceRange invalid (if the given position is smaller
	 * than the start position).
	 *
	 * @param pos is the end position that should be set.
	 */
	void setEnd(SourcePosition pos) { this->end = pos; }

	/**
	 * Sets the start and end of the SourceRange value to the given values.
	 * This operation might render the SourceRange invalid (if the given end
	 * position is smaller than the start position).
	 *
	 * @param start is the start position that should be set.
	 * @param end is the end position that should be set.
	 */
	void setRange(SourcePosition start, SourcePosition end)
	{
		this->start = start;
		this->end = end;
	}

	/**
	 * Makes the Range represent a zero-length range that is located at the
	 * given position. The given position should be interpreted as being located
	 * "between the character just before the start offset and the start
	 * offset".
	 *
	 * @param pos is the position to which start and end should be set.
	 */
	void setPosition(SourcePosition pos)
	{
		this->start = pos;
		this->end = pos;
	}

	/**
	 * Returns the start position of the SourceRange instance.
	 *
	 * @return the start offset in bytes.
	 */
	SourceOffset getStart() const { return start.getPosition(); }

	/**
	 * Returns the end position of the SourceRange instance.
	 *
	 * @return the end offset in bytes (non-inclusive).
	 */
	SourceOffset getEnd() const { return end.getPosition(); }

	/**
	 * Returns a copy of the underlying SourcePosition instance representing the
	 * start position.
	 *
	 * @return a copy of the start SourcePosition instance.
	 */
	SourcePosition getStartPosition() const { return start; }

	/**
	 * Returns a copy of the underlying SourcePosition instance representing the
	 * end position.
	 *
	 * @return a copy of the end SourcePosition instance.
	 */
	SourcePosition getEndPosition() const { return end; }

	/**
	 * Returns the length of the range. A range may have a zero value length, in
	 * which case it should be interpreted as "between the character before
	 * the start offset and the start offset". The returned value is only valid
	 * if the isValid() method returns true!
	 *
	 * @return the length of the range in bytes.
	 */
	size_t getLength() const { return end.getPosition() - start.getPosition(); }

	/**
	 * Returns true if this range is actually valid. This is the case if the
	 * start position is smaller or equal to the end position and start and end
	 * position themself are valid.
	 *
	 * @return true if the Range is valid.
	 */
	bool isValid() const
	{
		return start.isValid() && end.isValid() &&
		       start.getPosition() <= end.getPosition();
	}
};

/**
 * The SourceLocation class describes a range within a specific source file.
 */
class SourceLocation : public SourceRange {
private:
	/**
	 * Id of the source file.
	 */
	SourceId sourceId;

public:
	/**
	 * Default constructor.
	 */
	SourceLocation() : sourceId(InvalidSourceId){};

	/**
	 * Constructor, binds the SourceLocation to the given source file.
	 *
	 * @param sourceId specifies the file this location refers to.
	 */
	SourceLocation(SourceId sourceId) : sourceId(sourceId){};

	/**
	 * Constructor for a zero-length range.
	 *
	 * @param sourceId specifies the file this location refers to.
	 * @param pos is the byte offset at which the SourceRange instance should be
	 * located.
	 */
	SourceLocation(SourceId sourceId, SourcePosition pos)
	    : SourceRange(pos), sourceId(sourceId)
	{
	}

	/**
	 * Constructor of a SourceRange instance.
	 *
	 * @param sourceId specifies the file this location refers to.
	 * @param start is the byte offset of the first character in the range
	 * (start is inclusive).
	 * @param end points at the end of the range (end is non-inclusive).
	 */
	SourceLocation(SourceId sourceId, SourcePosition start, SourcePosition end)
	    : SourceRange(start, end), sourceId(sourceId)
	{
	}

	/**
	 * Constructor of a SourceRange instance.
	 *
	 * @param sourceId specifies the file this location refers to.
	 * @param start is the byte offset of the first character in the range
	 * (start is inclusive).
	 * @param end points at the end of the range (end is non-inclusive).
	 */
	SourceLocation(SourceId sourceId, const SourceRange &range)
	    : SourceRange(range), sourceId(sourceId)
	{
	}

	/**
	 * Sets the source id to the given value.
	 *
	 * @param sourceId specifies the file this location refers to.
	 */
	void setSourceId(SourceId sourceId) { this->sourceId = sourceId; }

	/**
	 * Returns the id of the source file this SourceLocation instance is bound
	 * to.
	 *
	 * @return the id of the source file this instance is bound to.
	 */
	SourceId getSourceId() const { return sourceId; }

	/**
	 * Returns true if this location is actually valid. This is the case if
	 * the underlying range is valid and the source id is valid.
	 *
	 * @return true if the Range is valid.
	 */
	bool isValid() const
	{
		return SourceRange::isValid() && sourceId != InvalidSourceId;
	}
};

/**
 * NullSourceLocation is an empty SourceLocation instance.
 */
extern const SourceLocation NullSourceLocation;

/**
 * Represents the context of a SourceLocation instance. Used to build error
 * messages.
 */
struct SourceContext {
	/**
	 * Underlying source range (contains the byte start and end offsets in
	 * bytes).
	 */
	SourceRange range;

	/**
	 * Name of the underlying resource.
	 */
	std::string filename;

	/**
	 * Start line, starting with one.
	 */
	int startLine;

	/**
	 * Start column, starting with one.
	 */
	int startColumn;

	/**
	 * End line, starting with one.
	 */
	int endLine;

	/**
	 * End column, starting with one.
	 */
	int endColumn;

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
	 * Relative length (in characters) within that line. May end beyond the
	 * text given in the context.
	 */
	int relLen;

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
	 * Default constructor, initializes primitive members with zero values.
	 */
	SourceContext()
	    : startLine(0),
	      startColumn(0),
	      endLine(0),
	      endColumn(0),
	      relPos(0),
	      relLen(0),
	      truncatedStart(false),
	      truncatedEnd(false)
	{
	}

	/**
	 * Returns true the context text is not empty.
	 *
	 * @return true if the context is valid and e.g. should be printed.
	 */
	bool isValid() const { return range.isValid() && hasLine() && hasColumn(); }

	/**
	 * Returns true if a valid (non-empty) filename is set.
	 */
	bool hasFile() const { return !filename.empty(); }

	/**
	 * Returns true, if the start line number is valid, false otherwise.
	 *
	 * @return true for valid line numbers.
	 */
	bool hasLine() const { return startLine > 0; }

	/**
	 * Returns true, if the start column number is valid, false otherwise.
	 *
	 * @return true for valid column numbers.
	 */
	bool hasColumn() const { return startColumn > 0; }
};

/**
 * Callback used to lookup the context corresponding to the given source
 * location.
 *
 * @param location is the location for which the context should be looked up.
 * @return the corresponding SourceContext.
 */
using SourceContextCallback =
    std::function<SourceContext(const SourceLocation &)>;

/**
 * Function to be used as default value for the SourceContextCallback. Returns
 * an invalid SourceContext.
 *
 * @param location is the location for which the context should be looked up.
 * @return an empty, invalid SourceContext.
 */
SourceContext NullSourceContextCallback(const SourceLocation &location);

}

#endif /* _OUSIA_LOCATION_HPP_ */

