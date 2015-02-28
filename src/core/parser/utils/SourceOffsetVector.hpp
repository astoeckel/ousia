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
 * @file SourceOffsetVector.hpp
 *
 * Contains a helper class used for storing the SourceOffset of each character
 * in a character vector in a compressed manner.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_SOURCE_OFFSET_VECTOR_HPP_
#define _OUSIA_SOURCE_OFFSET_VECTOR_HPP_

#include <cstdint>
#include <cassert>
#include <limits>
#include <vector>
#include <utility>

#include <core/common/Location.hpp>

namespace ousia {

/**
 * Class used for storing the SourceOffset of each character in the buffer using
 * a delta compression.
 */
class SourceOffsetVector {
private:
	/**
	 * Type used for representing the length of a character.
	 */
	using Length = uint8_t;

	/**
	 * Maximum length that can be represented using the Length type.
	 */
	static constexpr size_t MAX_LEN = std::numeric_limits<Length>::max();

	/**
	 * Interval in which the actual offset is stored, expressed as the binary
	 * logarithm.
	 */
	static constexpr size_t LOG2_OFFSET_INTERVAL = 6;

	/**
	 * Interval in which the actual offset is stored.
	 */
	static constexpr size_t OFFSET_INTERVAL = (1 << LOG2_OFFSET_INTERVAL);

	/**
	 * Bitmask for the bits that are set to zero when a new offset needs to be
	 * written.
	 */
	static constexpr size_t OFFSET_INTERVAL_MASK = OFFSET_INTERVAL - 1;

	/**
	 * Vector containing the delta compressed offset information.
	 */
	std::vector<Length> lens;

	/**
	 * Offsets containing the absolute offset for all OFFSET_INTERVAL elements.
	 */
	std::vector<SourceOffset> offsets;

	/**
	 * Last position given as "end" position in the storeOffset() method.
	 * Used to adapt the length of the previous element in case start and end
	 * positions do not match.
	 */
	SourceOffset lastEnd;

public:
	/**
	 * Default constructor of the SourceOffsetVector class.
	 */
	SourceOffsetVector() : lastEnd(0) {}

	/**
	 * Stores the location of a character in this SourceOffsetVector.
	 *
	 * @param start is the start location of the chracter in the source file.
	 * @param end is the end location of the character in the source file.
	 */
	void storeOffset(SourceOffset start, SourceOffset end)
	{
		// Make sure (end - start) is smaller than MAX_LEN
		assert(end - start < MAX_LEN);

		// Adapt the length of the previous character in case there is a gap
		if (!lens.empty() && start > lastEnd) {
			lens.back() += start - lastEnd;
		}
		lastEnd = end;

		// Store an absolute offset every OFFSET_INTERVAL elements
		if ((lens.size() & OFFSET_INTERVAL_MASK) == 0) {
			offsets.push_back(start);
		}

		// Store the length
		lens.push_back(end - start);
	}

	/**
	 * Loads the location of the character with the given index.
	 *
	 * @param idx is the index of the character for which the location should be
	 * read.
	 * @return a pair containing start and end source offset.
	 */
	std::pair<SourceOffset, SourceOffset> loadOffset(size_t idx) const
	{
		// Special treatment for the last character
		const size_t count = lens.size();
		if (idx > 0 && idx == count) {
			auto offs = loadOffset(count - 1);
			return std::pair<SourceOffset, SourceOffset>(offs.second,
			                                             offs.second);
		}

		// Calculate the start index in the lens vector and in the offsets
		// vector
		const size_t offsetIdx = idx >> LOG2_OFFSET_INTERVAL;
		const size_t sumStartIdx = idx & ~OFFSET_INTERVAL_MASK;

		// Make sure the index is valid
		assert(idx < count);
		assert(offsetIdx < offsets.size());

		// Sum over the length starting with the start offset
		SourceOffset start = offsets[offsetIdx];
		for (size_t i = sumStartIdx; i < idx; i++) {
			start += lens[i];
		}
		return std::pair<SourceOffset, SourceOffset>(start, start + lens[idx]);
	}

	/**
	 * Returns the number of characters for which offsets are stored.
	 */
	size_t size() const { return lens.size(); }

	/**
	 * Trims the length of the TokenizedData instance to the given length.
	 * Removes all token matches that lie within the trimmed region.
	 *
	 * @param length is the number of characters to which the TokenizedData
	 * instance should be trimmed.
	 */
	void trim(size_t length) {
		if (length < size()) {
			lens.resize(length);
			offsets.resize((length >> LOG2_OFFSET_INTERVAL) + 1);
			if (length > 0) {
				lastEnd = loadOffset(length - 1).second;
			} else {
				lastEnd = 0;
			}
		}
	}

	/**
	 * Resets the SourceOffsetVector to the state it had when it was
	 * constructed.
	 */
	void clear() {
		lens.clear();
		offsets.clear();
		lastEnd = 0;
	}
};
}

#endif /* _OUSIA_SOURCE_OFFSET_VECTOR_HPP_ */

