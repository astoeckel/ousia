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

#include <istream>
#include <list>
#include <memory>
#include <vector>

#include "Location.hpp"

namespace ousia {

/**
 * A chunked ring buffer used in CharReader to provide access to an input stream
 * with multiple read cursors. The Buffer automatically expands to the size of
 * the spanned by the read cursors while reusing already allocated memory.
 */
class Buffer {
public:
	/**
	 * Callback function which is called whenever new data is requested from the
	 * input stream.
	 *
	 * @param buf is points a the target memory region.
	 * @param size is the requested number of bytes.
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

	/**
	 * Moves the given cursor forward.
	 */
	size_t moveForward(CursorId cursor, size_t relativeOffs);

	/**
	 * Moves the given cursor backward.
	 */
	size_t moveBackward(CursorId cursor, size_t relativeOffs);

	/**
	 * Reads a character from the current cursor position and optionally
	 * advances.
	 */
	bool fetchCharacter(CursorId cursor, char &c, bool incr);

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
	 * Initializes the Buffer with a reference to an std::istream from which
	 * data will be read.
	 *
	 * @param istream is the input stream from which the data should be read.
	 */
	Buffer(std::istream &istream);

	/**
	 * Initializes the Buffer with the contents of the given string, after
	 * this operation the Buffer has a fixed size.
	 *
	 * @param str is the string containing the data that should be copied into
	 * the ring buffer.
	 */
	Buffer(const std::string &str);

#ifndef NDEBUG
	/**
	 * Destructor of the Buffer class. Makes sure that all cursors have been
	 * freed.
	 */
	~Buffer();
#endif

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
	 * Moves a cursor by offs bytes. Note that moving backwards is theoretically
	 * limited by the LOOKBACK_SIZE of the Buffer, practically it will most
	 * likely be limited by the REQUEST_SIZE, so you can got at most 64 KiB
	 * backwards.
	 *
	 * @param cursor is the cursor that should be moved.
	 * @param relativeOffs is a positive or negative integer number specifying
	 * the number of bytes the cursor should be moved forward (positive numbers)
	 * or backwards (negative numbers).
	 * @return the actual number of bytes the cursor was moved.
	 */
	ssize_t moveCursor(CursorId cursor, ssize_t relativeOffs);

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
	 * Reads a single character from the ring buffer from the given cursor and
	 * moves to the next character.
	 *
	 * @param cursor specifies the cursor from which the data should be read.
	 * The cursor will be advanced by one byte.
	 * @param c is the character into which the data needs to be read.
	 * @return true if a character was read, false if the end of the stream has
	 * been reached.
	 */
	bool read(CursorId cursor, char &c);

	/**
	 * Returns a single character from the ring buffer from the current cursor
	 * position and stays at that position.
	 *
	 * @param cursor specifies the cursor from which the data should be read.
	 * The cursor will be advanced by one byte.
	 * @param c is the character into which the data needs to be read.
	 * @return true if a character could be fetched, false if the end of the
	 * stream has been reached.
	 */
	bool fetch(CursorId cursor, char &c);
};

// Forward declaration
class CharReaderFork;

/**
 * Used within parsers for convenient access to single characters in an input
 * stream or buffer. It allows reading and peeking single characters from a
 * buffer. Additionally it contains an internal state machine that handles the
 * detection of linebreaks and converts these to a single '\n'.
 */
class CharReader {
private:
	/**
	 * Substitutes "\r", "\n\r", "\r\n" with a single "\n".
	 *
	 * @param cursor is the cursor from which the character should be read.
	 * @param c a reference to the character that should be written.
	 * @return true if another character needs to be read.
	 */
	bool substituteLinebreaks(Buffer::CursorId &cursor, char &c);

	/**
	 * Reads a single character from the given cursor.
	 *
	 * @param cursor is the cursor from which the character should be read.
	 * @param c a reference to the character that should be written.
	 * @return true if a character was read, false if the end of the stream has
	 * been reached.
	 */
	bool readAtCursor(Buffer::CursorId &cursor, char &c);

protected:
	/**
	 * Reference pointing at the underlying buffer.
	 */
	std::shared_ptr<Buffer> buffer;

	/**
	 * Cursor used for reading.
	 */
	Buffer::CursorId readCursor;

	/**
	 * Cursor used for peeking.
	 */
	Buffer::CursorId peekCursor;

	/**
	 * Set to true as long the underlying Buffer cursor is at the same position
	 * for the read and the peek cursor. This is only used for optimization
	 * purposes and makes consecutive reads a bit faster.
	 */
	bool coherent;

	/**
	 * Id of the underlying source file.
	 */
	SourceId sourceId;

	/**
	 * Offset to be added to the underlying buffer byte positions.
	 */
	size_t offs;

	/**
	 * Protected constructor of the CharReader base class. Creates new read
	 * and peek cursors for the given buffer.
	 *
	 * @param buffer is a reference to the underlying Buffer class responsible
	 * for allowing to read from a single input stream from multiple locations.
	 * @param sourceId is the ID of the underlying source file.
	 * @param offs is the byte offset at which the char reader should start
	 * counting.
	 */
	CharReader(std::shared_ptr<Buffer> buffer, SourceId sourceId, size_t offs);

public:
	/**
	 * Creates a new CharReader instance from a string.
	 *
	 * @param str is a string containing the input data.
	 * @param sourceId is the ID of the underlying source file.
	 * @param offs is the byte offset at which the char reader should start
	 * counting.
	 */
	CharReader(const std::string &str, SourceId sourceId = InvalidSourceId,
	           size_t offs = 0);

	/**
	 * Creates a new CharReader instance for an input stream.
	 *
	 * @param istream is the input stream from which incomming data should be
	 * read.
	 * @param sourceId is the ID of the underlying source file.
	 * @param offs is the byte offset at which the char reader should start
	 * counting.
	 */
	CharReader(std::istream &istream, SourceId sourceId = InvalidSourceId,
	           size_t offs = 0);

	/**
	 * Deletes the used cursors from the underlying buffer instance.
	 */
	~CharReader();

	// No copy
	CharReader(const Buffer &) = delete;

	// No assign
	CharReader &operator=(const Buffer &) = delete;

	/**
	 * Peeks a single character. If called multiple times, returns the
	 * character after the previously peeked character.
	 *
	 * @param c is a reference to the character to which the result should be
	 * written.
	 * @return true if the character was successfully read, false if there are
	 * no more characters to be read in the buffer.
	 */
	bool peek(char &c);

	/**
	 * Reads a character from the input data. If "peek" was called
	 * beforehand resets the peek pointer.
	 *
	 * @param c is a reference to the character to which the result should be
	 * written.
	 * @return true if the character was successfully read, false if there are
	 * no more characters to be read in the buffer.
	 */
	bool read(char &c);

	/**
	 * Resets the peek pointer to the "read" pointer.
	 */
	void resetPeek();

	/**
	 * Advances the read pointer to the peek pointer -- so if the "peek"
	 * function was called, "read" will now return the character after
	 * the last peeked character.
	 */
	void consumePeek();

	/**
	 * Moves the read cursor to the next non-whitespace character. Returns
	 * false, if the end of the stream was reached.
	 *
	 * @return false if the end of the stream was reached, false othrwise.
	 */
	bool consumeWhitespace();

	/**
	 * Creates a new CharReader located at the same position as this CharReader
	 * instance, yet the new CharReader can be used independently of this
	 * CharReader. Use the "commit" function of the returned CharReader to
	 * copy the state of the forked CharReaderFork to this CharReader.
	 *
	 * @return a CharReaderFork instance positioned at the same location as this
	 * CharReader instance.
	 */
	CharReaderFork fork();

	/**
	 * Reads raw data from the CharReader without any processing. Data is always
	 * read from the read cursor.
	 *
	 * @param buf is the target memory buffer.
	 * @param size is the number of bytes to be read.
	 * @return the number of bytes read.
	 */
	size_t readRaw(char *buf, size_t size);

	/**
	 * Moves read and peek cursor to the requested offset. Returns the offset
	 * that was actually reached.
	 *
	 * @param requestedOffset is the requested offset. This offset may no longer
	 * be reachable by the CharReader.
	 * @return the actually reached offset. The operation was successful, if
	 * the requested and reached offset are equal.
	 */
	size_t seek(size_t requestedOffset);

	/**
	 * Returns true if there are no more characters as the stream was closed.
	 *
	 * @return true if there is no more data.
	 */
	bool atEnd() const;

	/**
	 * Returns the offset of the read cursor in bytes.
	 *
	 * @return the offset of the read cursor in bytes.
	 */
	size_t getOffset() const;

	/**
	 * Returns the offset of the peek cursor in bytes.
	 *
	 * @return the offset of the peek cursor in bytes.
	 */
	size_t getPeekOffset() const;

	/**
	 * Returns a SourceLocation object describing the exact position (including
	 * the source file) of the read cursor.
	 *
	 * @return a SourceLocation object at the position of the current read
	 * cursor.
	 */
	SourceLocation getLocation() const;

	/**
	 * Returns a SourceLocation object starting at the given start position and
	 * ending at the exact position (including the source file) of the read
	 * cursor.
	 *
	 * @return a SourceLocation object at the position of the current read
	 * cursor.
	 */
	SourceLocation getLocation(SourcePosition start) const;

	/**
	 * Returns the current SourceId which describes the Resource on which the
	 * CharReader is currently working.
	 *
	 * @return the current SourceId.
	 */
	SourceId getSourceId() const;
};

/**
 * A CharReaderFork is returned whenever the "fork" function of the CharReader
 * class is used. Its "commit" function can be used to move the underlying
 * CharReader instance to the location of the CharReaderFork instance. Otherwise
 * the read location of the underlying CharReader is left unchanged.
 */
class CharReaderFork : public CharReader {
private:
	friend CharReader;

	/**
	 * The reader cursor of the underlying CharReader instance.
	 */
	Buffer::CursorId parentReadCursor;

	/**
	 * The peek cursor of the underlying CharReader instance.
	 */
	Buffer::CursorId parentPeekCursor;

	/**
	 * Constructor of the CharReaderFork class.
	 *
	 * @param buffer is a reference at the parent Buffer instance.
	 * @param parentPeekCursor is a reference at the parent read cursor.
	 * @param parentPeekCursor is a reference at the parent peek cursor.
	 * @param location is the current location.
	 * @param coherent specifies whether the char reader cursors are initialized
	 * coherently.
	 */
	CharReaderFork(std::shared_ptr<Buffer> buffer,
	               Buffer::CursorId parentReadCursor,
	               Buffer::CursorId parentPeekCursor, SourceId sourceId,
	               size_t offs, bool coherent);

public:
	/**
	 * Moves the read and peek cursor of the parent CharReader to the location
	 * of the read and peek cursor in the fork.
	 */
	void commit();
};
}

#endif /* _OUSIA_CHAR_READER_HPP_ */

