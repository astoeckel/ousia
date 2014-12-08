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
 * @file CharReader.hpp
 *
 * Used within all parsers to read single characters from an underlying stream.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_CHAR_READER_HPP_
#define _OUSIA_CHAR_READER_HPP_

#include <list>
#include <vector>

namespace ousia {
namespace utils {

/**
 * A chunked ring buffer used in CharReader to provide access to an input stream
 * with multiple read cursors. The Buffer automatically expands to the
 * size of the spanned by the read cursors while reusing already allocated
 * memory.
 */
class Buffer {
public:
	/**
	 * Callback function which is called whenever new data is requested from the
	 * input stream.
	 *
	 * @param buf is a pointer at the memory region to which the data should be
	 * writtern.
	 * @param size is the size of the
	 * @param userData is a pointer at some user defined data given in the
	 * constructor.
	 * @return the actual number of bytes read. If the result is smaller than
	 * the requested size, this tells the Buffer that the end of the input
	 * stream is reached.
	 */
	using ReadCallback = size_t (*)(char *buf, size_t size, void *userData);

	/**
	 * Handle used to identify a cursor.
	 */
	using CursorId = size_t;

private:
	/**
	 * Number of bytes to request from the input stream. Set to 64 KiB because
	 * this seems to be a nice value for I/O operations according to multiple
	 * sources.
	 */
	static constexpr size_t REQUEST_SIZE = 64 * 1024;

	/**
	 * Number of bytes the buffer guarantees to be capable of looking back
	 * for extracting the current context.
	 */
	static constexpr size_t LOOKBACK_SIZE = 128;

	/**
	 * Type used internally to represent one chunk of memory.
	 */
	using Bucket = std::vector<char>;

	/**
	 * Type used internally to represent a bucket container.
	 */
	using BucketList = std::list<Bucket>;

	/**
	 * Type used internally for representing iterators in the bucket list.
	 */
	using BucketIterator = BucketList::iterator;

	/**
	 * Type used internally to represent a read cursor.
	 */
	struct Cursor {
		/**
		 * Iterator pointing at the current bucket.
		 */
		BucketIterator bucket;

		/**
		 * Index of the bucket relative to the start bucket.
		 */
		size_t bucketIdx;

		/**
		 * Current offset within that bucket.
		 */
		size_t bucketOffs;
	};

	/**
	 * List of buckets containing the buffered memory.
	 */
	BucketList buckets;

	/**
	 * List of cursors used to access the memory. Note that cursors can be
	 * marked as inactive and reused lateron (to avoid having to resize the
	 * vector).
	 */
	std::vector<Cursor> cursors;

	/**
	 * Bitfield specifying which of the cursors is actually valid.
	 */
	std::vector<bool> alive;

	/**
	 * Function to be called whenever new data is needed. Set to nullptr if the
	 * Buffer is not backed by an input stream.
	 */
	const ReadCallback callback;

	/**
	 * User data given in the constructor.
	 */
	void *userData;

	/**
	 * Set to true if the input stream is at its end.
	 */
	bool reachedEnd;

	/**
	 * Iterator pointing at the current start bucket.
	 */
	BucketIterator startBucket;

	/**
	 * Iterator pointing at the last bucket.
	 */
	BucketIterator endBucket;

	/**
	 * Byte offset of the start bucket relative to the beginning of the stream.
	 */
	size_t startOffset;

	/**
	 * Points at the smallest possible available cursor index, yet does not
	 * guarantee that this cursor index actuall is free.
	 */
	CursorId firstDead;

	/**
	 * Advances the bucket iterator, cares about wrapping around in the ring.
	 */
	void advance(BucketIterator &it);

	/**
	 * Advances the bucket iterator, cares about wrapping around in the ring.
	 */
	void advance(BucketList::const_iterator &it) const;

	/**
	 * Internally used to find the next free cursor in the cursors vector. The
	 * cursor is marked as active.
	 *
	 * @return the next free cursor index.
	 */
	CursorId nextCursor();

	/**
	 * Returns a reference at the next bucket into which data should be
	 * inserted.
	 *
	 * @return a bucket into which the data can be inserted.
	 */
	Bucket &nextBucket();

	/**
	 * Reads data from the input stream and places it in the next free buffer.
	 */
	void stream();

public:
	/**
	 * Intializes the Buffer with a reference to a ReadCallback that is used
	 * to fetch data from an underlying input stream.
	 *
	 * @param callback is the function that will be called whenever data is read
	 * from the ring buffer and the buffer does not hold enough data to fulfill
	 * this read request.
	 * @param userData is a pointer to user defined data which will be passed to
	 * the callback function.
	 */
	Buffer(ReadCallback callback, void *userData);

	/**
	 * Initializes the Buffer with the contents of the given string, after
	 * this operation the Buffer has a fixed size.
	 *
	 * @param str is the string containing the data that should be copied into
	 * the ring buffer.
	 */
	Buffer(const std::string &str);

	// No copy
	Buffer(const Buffer &) = delete;

	// No assign
	Buffer &operator=(const Buffer &) = delete;

	/**
	 * Creates a new read cursor positioned at the smallest possible position
	 * in the ring buffer.
	 */
	CursorId createCursor();

	/**
	 * Creates a new read cursor positioned at the same position as the given
	 * read cursor.
	 *
	 * @param ref is the read cursor that should be used as reference for the
	 * new read cursor.
	 */
	CursorId createCursor(CursorId ref);

	/**
	 * Copies the position of one cursor to another cursor.
	 *
	 * @param from is the cursor id of which the position should be copied.
	 * @param to is the cursor id to which the position should be copied.
	 */
	void copyCursor(CursorId from, CursorId to);

	/**
	 * Deletes the cursor with the given id. The cursor may no longer be used
	 * after this function has been called.
	 *
	 * @param cursor is the id of the cursor that should be freed.
	 */
	void deleteCursor(CursorId cursor);

	/**
	 * Returns the current byte offset of the given cursor relative to the
	 * beginning of the stream.
	 *
	 * @param cursor is the cursor for which the byte offset relative to the
	 * beginning of the stream should be returned.
	 * @return the number of bytes since the beginning of the stream for the
	 * given cursor.
	 */
	size_t offset(CursorId cursor) const;

	/**
	 * Returns true if the given cursor currently is at the end of the stream.
	 *
	 * @param cursor is the cursor for which the atEnd flag should be returned.
	 * @return true if the there are no more bytes for this cursor. If false
	 * is returned, this means that there may be more bytes in the stream,
	 * nevertheless the end of the stream may be hit once the next read function
	 * is called.
	 */
	bool atEnd(CursorId cursor) const;

	/**
	 * Reads a single character from the ring buffer from the given cursor.
	 *
	 * @param cursor specifies the cursor from which the data should be read.
	 * The cursor will be advanced by one byte.
	 * @param c is the character into which the data needs to be read.
	 * @return true if a character was read, false if the end of the stream has
	 * been reached.
	 */
	bool read(CursorId cursor, char &c);

//	/**
//	 * Reads string from the ring buffer from the given cursor.
//	 *
//	 * @param cursor specifies the cursor from which the data should be read.
//	 * The cursor will be advanced by the specified number of bytes (or to the
//	 * end of the stream).
//	 * @param res is the vector into which the data should be read. Any already
//	 * present data will be overridden.
//	 * @param len is number of bytes that should be read from the buffer.
//	 * @return true if len bytes were read, false if less the len bytes have
//	 * been read because the end of the stream has been reached.
//	 */
//	bool read(CursorId cursor, std::vector<char> &res, size_t len);
};

}
}

#endif /* _OUSIA_CHAR_READER_HPP_ */

