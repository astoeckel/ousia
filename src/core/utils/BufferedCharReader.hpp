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

#ifndef _OUSIA_UTILS_BUFFERED_CHAR_READER_H_
#define _OUSIA_UTILS_BUFFERED_CHAR_READER_H_

#include <deque>
#include <string>
#include <cstdint>

namespace ousia {
namespace utils {

/**
 * The BufferedCharReader class is used for storing incomming data that
 * is fed into the pipeline as well as reading/peeking single characters
 * from that buffer. Additionally it counts the current column/row
 * (with correct handling for UTF-8) and contains an internal state
 * machine that handles the detection of linebreaks.
 *
 * Additionally the BufferedCharReader performs the following tasks:
 * 1. Convert the incomming character encoding to UTF-8 (TODO: implement)
 * 2. Convert arbitrary linebreaks to a single "\n"
 */
class BufferedCharReader {

private:

	/**
	 * The ReadCursor structure is responsible for representing the read
	 * position within the text an all state machine states belonging to the
	 * cursor. There are two types of read cursors: destructive and
	 * non-destructive read cursors.
	 */
	struct ReadCursor {
		/**
		 * Specifies whether this is a destructive cursor (bytes are discarded
		 * once they were read from the buffer).
		 */
		const bool destructive;

		/**
		 * The line the cursor currently points to.
		 */
		unsigned int line;

		/**
		 * The column the cursor currently points to.
		 */
		unsigned int column;

		/**
		 * The index of the element in the data buffer we're currently reading
		 * from.
		 */
		unsigned int bufferElem;

		/**
		 * The byte position within this data buffer.
		 */
		unsigned int bufferPos;

		/**
		 * State variable used in the internal state machine of the
		 * line feed detection.
		 */
		uint8_t lbState;

		/**
		 * Constructor of the ReadCursor structure.
		 *
		 * @param destructive specifies whether the ReadCursor is destructive
		 * (consumes all read characters, as used in the "read cursor") or
		 * non-destructive (as used in the "peek cursor").
		 */
		ReadCursor(const bool destructive);

		/**
		 * Copys the data from another ReadCursor without overriding the
		 * "destructive" flag.
		 */
		void assign(const ReadCursor &cursor);

		/**
		 * Resets the cursor without changing the "destructive" flag.
		 */
		void reset();
	};

	/**
	 * Queue containing the data that has been fed into the char reader.
	 */
	std::deque<std::string> buffer;

	/**
	 * The read and the peek cursor. 
	 */
	ReadCursor readCursor, peekCursor;

	/**
	 * Determines whether the reader has been closed.
	 */
	bool closed;

	/**
	 * Substitute any combination of linebreaks in the incomming code with "\n".
	 * Returns true if the current character is meant as output, false
	 * otherwise.
	 */
	bool substituteLinebreaks(ReadCursor *cursor, char *c);

	/**
	 * Reads a character from the input buffer and advances the given read
	 * cursor.
	 *
	 * @param cursor is a reference to the read cursor that should be used
	 * for reading.
	 * @param hasChar is set to true, if a character is available, false if
	 * no character is available (e.g. because line breaks are substituted or
	 * the end of a buffer boundary is reached -- in this case this function
	 * should be called again with the same parameters.)
	 * @param c is a output parameter, which will be set to the read character.
	 * @param returns true if there was enough data in the buffer, false
	 * otherwise.
	 */
	bool readCharacterAtCursor(ReadCursor *cursor, char *c);

	/**
	 * Function that is called for each read character -- updates the row and
	 * column count.
	 */
	void updatePositionCounters(const char c);

public:

	/**
	 * Constructor of the buffered char reader class.
	 */
	BufferedCharReader();

	/**
	 * Resets the reader to its initial state.
	 */
	void reset();

	/**
	 * Feeds new data into the internal buffer of the BufferedCharReader
	 * class.
	 *
	 * @param data is a string containing the data that should be
	 * appended to the internal buffer.
	 * @return true if the operation was successful, false otherwise (e.g.
	 * because the reader is closed).
	 */
	bool feed(const std::string &data);

	/**
	 * Marks the end of the input, allowing successors in the pipeline
	 * to react properly (e.g. creating the end of stream token).
	 */
	void close();

	/**
	 * Peeks a single character. If called multiple times, returns the
	 * character after the previously peeked character.
	 *
	 * @param c is a reference to the character to which the result should be
	 * writtern.
	 * @return true if the character was successfully read, false if there are
	 * no more characters to be read in the buffer.
	 */
	bool peek(char *c);

	/**
	 * Reads a character from the input data. If "peek" was called
	 * beforehand resets the peek pointer.
	 *
	 * @param c is a reference to the character to which the result should be
	 * writtern.
	 * @return true if the character was successfully read, false if there are
	 * no more characters to be read in the buffer.
	 */
	bool read(char *c);

	/**
	 * Advances the read pointer to the peek pointer -- so if the "peek"
	 * function was called, "read" will now return the character after
	 * the last peeked character.
	 */
	void consumePeek();

	/**
	 * Resets the peek pointer to the "read" pointer.
	 */
	void resetPeek();

	/**
	 * Returns true if there are no more characters as the stream was
	 * closed.
	 */
	bool atEnd();

	/**
	 * Returns the current line (starting with one).
	 */
	inline int getLine()
	{
		return readCursor.line;
	}

	/**
	 * Returns the current column (starting with one).
	 */
	inline int getColumn()
	{
		return readCursor.column;
	}

};

}
}

#endif /* _OUSIA_UTILS_BUFFERED_CHAR_READER_H_ */

