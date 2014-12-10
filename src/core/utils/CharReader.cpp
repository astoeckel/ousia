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

#include <algorithm>
#include <limits>

#include <core/Utils.hpp>

#include "CharReader.hpp"

namespace ousia {
namespace utils {

/* Helper functions */

/**
 * istreamReadCallback is used internally by the Buffer calss to stream data
 * from an input stream.
 *
 * @param buf is points a the target memory region.
 * @param size is the requested number of bytes.
 * @param userData is a pointer at some user defined data.
 * @return the actual number of bytes read. If the result is smaller than
 * the requested size, this tells the Buffer that the end of the input
 * stream is reached.
 */
static size_t istreamReadCallback(char *buf, size_t size, void *userData)
{
	return (static_cast<std::istream *>(userData))->read(buf, size).gcount();
}

/* Class Buffer */

Buffer::Buffer(ReadCallback callback, void *userData)
    : callback(callback),
      userData(userData),
      reachedEnd(false),
      startBucket(buckets.end()),
      endBucket(buckets.end()),
      startOffset(0),
      firstDead(0)
{
	// Load a first block of data from the stream
	stream();
	startBucket = buckets.begin();
}

Buffer::Buffer(std::istream &istream) : Buffer(istreamReadCallback, &istream) {}

Buffer::Buffer(const std::string &str)
    : callback(nullptr),
      userData(nullptr),
      reachedEnd(true),
      startBucket(buckets.end()),
      endBucket(buckets.end()),
      startOffset(0),
      firstDead(0)
{
	// Copy the given string into a first buffer and set the start buffer
	// correctly
	Bucket &bucket = nextBucket();
	bucket.resize(str.size());
	std::copy(str.begin(), str.end(), bucket.begin());
	startBucket = buckets.begin();
}

void Buffer::advance(BucketIterator &it)
{
	it++;
	if (it == buckets.end()) {
		it = buckets.begin();
	}
}

void Buffer::advance(BucketList::const_iterator &it) const
{
	it++;
	if (it == buckets.cend()) {
		it = buckets.cbegin();
	}
}

Buffer::Bucket &Buffer::nextBucket()
{
	constexpr size_t MAXVAL = std::numeric_limits<size_t>::max();

	// Fetch the minimum bucket index
	size_t minBucketIdx = MAXVAL;
	for (size_t i = 0; i < cursors.size(); i++) {
		if (alive[i]) {
			// Fetch references to the bucket and the cursor
			const Cursor &cur = cursors[i];
			const Bucket &bucket = *(cur.bucket);

			// Increment the bucket index by one, if the cursor is at the end
			// of the bucket (only valid if the LOOKBACK_SIZE is set to zero)
			size_t bIdx = cur.bucketIdx;
			if (LOOKBACK_SIZE == 0 && cur.bucketOffs == bucket.size()) {
				bIdx++;
			}

			// Decrement the bucket index by one, if the previous bucket still
			// needs to be reached and cannot be overridden
			if (bIdx > 0 && cur.bucketOffs < LOOKBACK_SIZE) {
				bIdx--;
			}

			// Set the bucket index to the minium
			minBucketIdx = std::min(minBucketIdx, bIdx);
		}
	}

	// If there is space between the current start bucket and the read
	// cursor, the start bucket can be safely overridden.
	if (minBucketIdx > 0 && minBucketIdx != MAXVAL) {
		// All cursor bucket indices will be decreased by one
		for (size_t i = 0; i < cursors.size(); i++) {
			cursors[i].bucketIdx--;
		}

		// Increment the start offset
		startOffset += startBucket->size();

		// The old start bucket is the new end bucket
		endBucket = startBucket;

		// Advance the start bucket, wrap around at the end of the list
		advance(startBucket);
	} else {
		// No free bucket, insert a new one before the start bucket
		endBucket = buckets.emplace(startBucket);
	}
	return *endBucket;
}

Buffer::CursorId Buffer::nextCursor()
{
	bool hasCursor = false;
	CursorId res = 0;

	// Search for the next free cursor starting with minNextCursorId
	for (size_t i = firstDead; i < alive.size(); i++) {
		if (!alive[i]) {
			res = i;
			hasCursor = true;
			break;
		}
	}

	// Add a new cursor to the cursor list if no cursor is currently free
	if (!hasCursor) {
		res = cursors.size();
		cursors.resize(res + 1);
		alive.resize(res + 1);
	}

	// The next dead cursor is at least the next cursor
	firstDead = res + 1;

	// Mark the new cursor as alive
	alive[res] = true;

	return res;
}

void Buffer::stream()
{
	// Fetch the bucket into which the data should be inserted, make sure it
	// has the correct size
	Bucket &tar = nextBucket();
	tar.resize(REQUEST_SIZE);

	// Read data from the stream into the target buffer
	size_t size = callback(tar.data(), REQUEST_SIZE, userData);

	// If not enough bytes were returned, we're at the end of the stream
	if (size < REQUEST_SIZE) {
		tar.resize(size);
		reachedEnd = true;
	}
}

Buffer::CursorId Buffer::createCursor()
{
	CursorId res = nextCursor();
	cursors[res].bucket = startBucket;
	cursors[res].bucketIdx = 0;
	cursors[res].bucketOffs = 0;
	return res;
}

Buffer::CursorId Buffer::createCursor(Buffer::CursorId ref)
{
	CursorId res = nextCursor();
	cursors[res] = cursors[ref];
	return res;
}

void Buffer::copyCursor(Buffer::CursorId from, Buffer::CursorId to)
{
	cursors[to] = cursors[from];
}

void Buffer::deleteCursor(Buffer::CursorId cursor)
{
	alive[cursor] = false;
	firstDead = std::min(firstDead, cursor);
}

size_t Buffer::offset(Buffer::CursorId cursor) const
{
	const Cursor &cur = cursors[cursor];
	size_t offs = startOffset + cur.bucketOffs;
	BucketList::const_iterator it = startBucket;
	while (it != cur.bucket) {
		offs += it->size();
		advance(it);
	}
	return offs;
}

size_t Buffer::moveForward(CursorId cursor, size_t relativeOffs)
{
	size_t offs = relativeOffs;
	Cursor &cur = cursors[cursor];
	while (offs > 0) {
		// Fetch the current bucket of the cursor
		Bucket &bucket = *(cur.bucket);

		// If there is enough space in the bucket, simply increment the bucket
		// offset by the given relative offset
		const size_t space = bucket.size() - cur.bucketOffs;
		if (space >= offs) {
			cur.bucketOffs += offs;
			break;
		} else {
			// Go to the end of the current bucket otherwise
			offs -= space;
			cur.bucketOffs = bucket.size();

			// Go to the next bucket
			if (cur.bucket != endBucket) {
				// Go to the next bucket
				advance(cur.bucket);
				cur.bucketIdx++;
				cur.bucketOffs = 0;
			} else {
				// Abort, if there is no more data to stream, otherwise just
				// load new data
				if (reachedEnd) {
					return relativeOffs - offs;
				}
				stream();
			}
		}
	}
	return relativeOffs;
}

size_t Buffer::moveBackward(CursorId cursor, size_t relativeOffs)
{
	size_t offs = relativeOffs;
	Cursor &cur = cursors[cursor];
	while (offs > 0) {
		// If there is enough space in the bucket, simply decrement the bucket
		// offset by the given relative offset
		if (cur.bucketOffs >= offs) {
			cur.bucketOffs -= offs;
			break;
		} else {
			// Go to the beginning of the current bucket otherwise
			offs -= cur.bucketOffs;
			cur.bucketOffs = 0;

			// Abort if there is no more bucket to got back to
			if (cur.bucketIdx == 0) {
				return relativeOffs - offs;
			}

			// Go to the previous bucket (wrap around at the beginning of the
			// list)
			if (cur.bucket == buckets.begin()) {
				cur.bucket = buckets.end();
			}
			cur.bucket--;

			// Decrement the bucket index, and set the current offset to the
			// end of the new bucket
			cur.bucketIdx--;
			cur.bucketOffs = cur.bucket->size();
		}
	}
	return relativeOffs;
}

ssize_t Buffer::moveCursor(CursorId cursor, ssize_t relativeOffs)
{
	if (relativeOffs > 0) {
		return moveForward(cursor, relativeOffs);
	} else if (relativeOffs < 0) {
		return -moveBackward(cursor, -relativeOffs);
	} else {
		return 0;
	}
}

bool Buffer::atEnd(Buffer::CursorId cursor) const
{
	const Cursor &c = cursors[cursor];
	return reachedEnd &&
	       (c.bucket == endBucket && c.bucketOffs == endBucket->size());
}

bool Buffer::read(Buffer::CursorId cursor, char &c)
{
	Cursor &cur = cursors[cursor];
	while (true) {
		// Reference at the current bucket
		Bucket &bucket = *(cur.bucket);

		// If there is still data in the current bucket, return this data
		if (cur.bucketOffs < bucket.size()) {
			c = bucket[cur.bucketOffs];
			cur.bucketOffs++;
			return true;
		} else if (cur.bucket == endBucket) {
			// Return false if the end of the stream has been reached, otherwise
			// load new data
			if (reachedEnd) {
				return false;
			}
			stream();
		}

		// Go to the next bucket
		cur.bucketIdx++;
		cur.bucketOffs = 0;
		advance(cur.bucket);
	}
}

/* CharReader::Cursor class */

void CharReader::Cursor::assign(std::shared_ptr<Buffer> buffer,
                                CharReader::Cursor &cursor)
{
	// Copy the cursor position
	buffer->copyCursor(cursor.cursor, this->cursor);

	// Copy the state
	line = cursor.line;
	column = cursor.column;
	state = cursor.state;
	lastLinebreak = cursor.lastLinebreak;
}

/* CharReader class */

CharReader::CharReader(std::shared_ptr<Buffer> buffer, size_t line,
                       size_t column)
    : buffer(buffer),
      readCursor(buffer->createCursor(), line, column),
      peekCursor(buffer->createCursor(), line, column),
      coherent(true)
{
}

CharReader::CharReader(const std::string &str, size_t line, size_t column)
    : CharReader(std::shared_ptr<Buffer>{new Buffer{str}}, line, column)
{
}

CharReader::CharReader(std::istream &istream, size_t line, size_t column)
    : CharReader(std::shared_ptr<Buffer>{new Buffer{istream}}, line, column)
{
}

CharReader::~CharReader()
{
	buffer->deleteCursor(readCursor.cursor);
	buffer->deleteCursor(peekCursor.cursor);
}

bool CharReader::substituteLinebreaks(Cursor &cursor, char &c)
{
	if (c == '\n' || c == '\r') {
		switch (cursor.state) {
			case LinebreakState::NONE:
				// We got a first linebreak character -- output a '\n'
				if (c == '\n') {
					cursor.state = LinebreakState::HAS_LF;
				} else {
					cursor.state = LinebreakState::HAS_CR;
				}
				c = '\n';
				return true;
			case LinebreakState::HAS_LF:
				// If a LF is followed by a LF, output a new linefeed
				if (c == '\n') {
					cursor.state = LinebreakState::HAS_LF;
					return true;
				}

				// Otherwise, don't handle this character (part of "\n\r")
				cursor.state = LinebreakState::NONE;
				return false;
			case LinebreakState::HAS_CR:
				// If a CR is followed by a CR, output a new linefeed
				if (c == '\r') {
					cursor.state = LinebreakState::HAS_CR;
					c = '\n';
					return true;
				}

				// Otherwise, don't handle this character (part of "\r\n")
				cursor.state = LinebreakState::NONE;
				return false;
		}
	}

	// No linebreak character, reset the linebreak state
	cursor.state = LinebreakState::NONE;
	return true;
}

bool CharReader::readAtCursor(Cursor &cursor, char &c)
{
	while (true) {
		// Return false if we're at the end of the stream
		if (!buffer->read(cursor.cursor, c)) {
			return false;
		}

		// Substitute linebreak characters with a single '\n'
		if (substituteLinebreaks(cursor, c)) {
			if (c == '\n') {
				// A linebreak was reached, go to the next line
				cursor.line++;
				cursor.column = 1;
				cursor.lastLinebreak = buffer->offset(cursor.cursor);
			} else {
				// Ignore UTF-8 continuation bytes
				if (!((c & 0x80) && !(c & 0x40))) {
					cursor.column++;
				}
			}

			return true;
		}
	}
}

bool CharReader::peek(char &c)
{
	// If the reader was coherent, update the peek cursor state
	if (coherent) {
		peekCursor.assign(buffer, readCursor);
		coherent = false;
	}

	// Read a character from the peek cursor
	return readAtCursor(peekCursor, c);
}

bool CharReader::read(char &c)
{
	// Read a character from the buffer at the current read cursor
	bool res = readAtCursor(readCursor, c);

	// Set the peek position to the current read position, if reading was not
	// coherent
	if (!coherent) {
		peekCursor.assign(buffer, readCursor);
		coherent = true;
	} else {
		buffer->copyCursor(readCursor.cursor, peekCursor.cursor);
	}

	// Return the result of the read function
	return res;
}

void CharReader::resetPeek()
{
	if (!coherent) {
		peekCursor.assign(buffer, readCursor);
		coherent = true;
	}
}

void CharReader::consumePeek()
{
	if (!coherent) {
		readCursor.assign(buffer, peekCursor);
		coherent = true;
	}
}

bool CharReader::consumeWhitespace()
{
	char c;
	while (peek(c)) {
		if (!Utils::isWhitespace(c)) {
			resetPeek();
			return true;
		}
		consumePeek();
	}
	return false;
}

CharReaderFork CharReader::fork()
{
	return CharReaderFork(buffer, readCursor, peekCursor, coherent);
}

/* Class CharReaderFork */

CharReaderFork::CharReaderFork(std::shared_ptr<Buffer> buffer,
                               CharReader::Cursor &parentReadCursor,
                               CharReader::Cursor &parentPeekCursor,
                               bool coherent)
    : CharReader(buffer, 1, 1),
      parentReadCursor(parentReadCursor),
      parentPeekCursor(parentPeekCursor)
{
	readCursor.assign(buffer, parentReadCursor);
	peekCursor.assign(buffer, parentPeekCursor);
	this->coherent = coherent;
}

void CharReaderFork::commit()
{
	parentReadCursor.assign(buffer, readCursor);
	parentPeekCursor.assign(buffer, peekCursor);
}
}
}

