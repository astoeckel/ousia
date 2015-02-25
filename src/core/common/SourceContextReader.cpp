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

#include <algorithm>

#include <core/common/CharReader.hpp>
#include <core/common/Utils.hpp>

#include "SourceContextReader.hpp"

namespace ousia {

SourceContextReader::SourceContextReader() : cache{0} {}

SourceContext SourceContextReader::readContext(CharReader &reader,
                                               const SourceRange &range,
                                               size_t maxContextLength,
                                               const std::string &filename)
{
	// Abort if the given range is not valid
	if (!range.isValid()) {  // (I2)
		return SourceContext{};
	}

	// Set the filename and the range
	SourceContext ctx;
	ctx.startLine = 1;
	ctx.startColumn = 1;
	ctx.endLine = 1;
	ctx.endColumn = 1;
	ctx.range = range;
	ctx.filename = filename;

	// Some constants for convenience
	const SourceOffset start = range.getStart();
	const SourceOffset end = range.getEnd();
	const SourceOffset lastCacheOffs = cache.back();

	// Find the entry in the cache that is just below the given start offset
	// and jump to this location
	size_t offs = 0;
	auto it = std::lower_bound(cache.begin(), cache.end(), start);
	if (it != cache.begin()) {
		it--;        // Go to the previous entry
		offs = *it;  // Read the corresponding byte offset
		size_t line = it - cache.begin() + 1;
		ctx.startLine = line;
		ctx.endLine = line;
	}

	// Move the char reader to the specified offset, abort if this did not work
	// out
	if (offs != reader.seek(offs)) {
		return SourceContext{};
	}

	// TODO: Handle skew introduced by linebreak processing \n\r => \n

	// Read until the requested byte offset is reached, track linebreaks in the
	// linebreak cache
	std::vector<char> lineBuf;
	size_t lineBufStart = offs;
	size_t lastLineStart = offs;
	char c;
	while (reader.read(c)) {
		// Fetch the offset after this character
		const size_t nextOffs = reader.getOffset();

		// Fetch the current offset, check whether start was reached
		const bool reachedStart = offs >= start;
		const bool reachedEnd = offs >= end;

		// Handle linebreaks and update the linebreak cache
		if (c == '\n') {
			// Update the linebreak cache if we are in uncached regions
			if (offs > lastCacheOffs) {
				cache.push_back(nextOffs);
			}
			if (!reachedStart) {
				ctx.startLine++;
				ctx.startColumn = 1;
				lineBuf.clear();
				lineBufStart = nextOffs;
				lastLineStart = nextOffs;
			} else {
				lineBuf.push_back('\n');
			}
			if (!reachedEnd) {
				ctx.endLine++;
				ctx.endColumn = 1;
			} else {
				// This was the last character, abort
				break;
			}
		} else {
			// Increment the start and the end column if this is not an
			// UTF8-continuation byte (note that we count unicode codepoints not
			// actual characters, which may be more than one codepoint)
			if (!((c & 0x80) && !(c & 0x40))) {
				if (!reachedStart) {
					ctx.startColumn++;
				}
				if (!reachedEnd) {
					ctx.endColumn++;
				}
			}

			// Record all characters when start is reached or at least when
			// the distance to start is smaller than the maximum context length
			// TODO: This is suboptimal as parts of lineBuf are thrown away
			// later. If the given range is really large, this will waste huge
			// amounts of RAM.
			if (reachedStart || (start - offs <= maxContextLength)) {
				if (lineBuf.empty()) {
					lineBufStart = offs;
				}
				lineBuf.push_back(c);
			}
		}

		// Set the new offset
		offs = nextOffs;
	}

	// If we did not reach the end or for some reason the lineBufStart is larger
	// than start (to assure invariant I1 is fulfilled), abort
	offs = reader.getOffset();
	if (offs < end || lineBufStart > start) {  // (I1)
		return SourceContext{};
	}

	// Calculate a first relative position and length
	ctx.relPos = start - lineBufStart;  // lineBufStart > start (I1)
	ctx.relLen = end - start;           // end >= start (I2)

	// Remove linebreaks at the beginning and the end
	const std::pair<size_t, size_t> b = Utils::trim(
	    lineBuf,
	    [&lineBuf](size_t i) { return Utils::isLinebreak(lineBuf[i]); });
	ssize_t s = b.first, e = b.second;
	s = std::min(s, static_cast<ssize_t>(ctx.relPos));

	// Remember the trimmed positions, only continue if the context text did
	// not entirely consist of linebreaks
	const ssize_t ts = s, te = e;  // s >= 0, e >= 0, ts >= 0, te >= 0 (I3)
	if (te > ts) {
		// Trim the line further if it is longer than the maxContextLength
		if (static_cast<size_t>(te - ts) > maxContextLength &&
		    maxContextLength != MAX_MAX_CONTEXT_LENGTH) {
			ssize_t c = (ctx.relPos + ctx.relLen / 2);
			s = c - maxContextLength / 2;
			e = c + maxContextLength / 2;

			// Account for rounding error
			if (static_cast<size_t>(e - s) < maxContextLength) {
				e++;
			}

			// Redistribute available characters at the beginning or the end
			if (s < ts) {
				e = e + (ts - s);
				s = ts;  // ts >= 0 => s >= 0 (I3)
			}
			if (e > te) {
				s = s - std::min(s - ts, e - te);  // ts - s <= s => s >= 0
				e = te;                            // te >= 0 => e >= 0 (I3)
			}
		}

		// Update the relative position and length, set the "truncated" flags
		ctx.relPos = std::max<ssize_t>(0, start - lineBufStart - s);
		ctx.relLen = std::min<ssize_t>(ctx.relLen, e - s);
		ctx.truncatedStart = s > ts || lastLineStart < lineBufStart;
		ctx.truncatedEnd = e < te;

		// Copy the selected area to the output string
		ctx.text = std::string{&lineBuf[s], static_cast<size_t>(e - s)};
	}

	return ctx;
}

SourceContext SourceContextReader::readContext(CharReader &reader,
                                               const SourceRange &range,
                                               const std::string &filename)
{
	return readContext(reader, range, MAX_MAX_CONTEXT_LENGTH, filename);
}
}

