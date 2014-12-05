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
 * @file BufferedCharReader.hpp
 *
 * Contains the BufferedCharReader class which is used for reading/peeking
 * single characters from an input stream or string.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_BUFFERED_CHAR_READER_H_
#define _OUSIA_BUFFERED_CHAR_READER_H_

#include <deque>
#include <string>
#include <istream>
#include <cstdint>

namespace ousia {

// TODO: Better split this class into multiple classes with base class
// BufferedCharReader where each sub class represents one method of supplying
// the input data (feeding, initial string, input stream).

/**
 * The BufferedCharReader class is used for storing incomming data that
 * is fed into the pipeline as well as reading/peeking single characters
 * from that buffer. Additionally it counts the current column/row
 * (with correct handling for UTF-8) and contains an internal state
 * machine that handles the detection of linebreaks and converts these to a
 * single '\n'.
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
		 * Specifies whether this is a destructive cursor (bytes are discarded
		 * once they were read from the buffer).
		 */
		const bool destructive;

		/**
		 * State variable used in the internal state machine of the
		 * line feed detection.
		 */
		uint8_t lbState;

		/**
		 * Constructor of the ReadCursor structure.
		 *
		 * @param line is the start line.
		 * @param column is the start column.
		 * @param destructive specifies whether the ReadCursor is destructive
		 * (consumes all read characters, as used in the "read cursor") or
		 * non-destructive (as used in the "peek cursor").
		 */
		ReadCursor(unsigned int line, unsigned int column, bool destructive);

		/**
		 * Copys the data from another ReadCursor without overriding the
		 * "destructive" flag.
		 *
		 * @param cursor is the cursor that should be copied.
		 */
		void assign(const ReadCursor &cursor);
	};

	/**
	 * Pointer at an (optional) input stream used for reading a chunk of data
	 * whenever the input buffer depletes.
	 */
	std::istream *inputStream;

	/**
	 * The read and the peek cursor.
	 */
	ReadCursor readCursor, peekCursor;

	/**
	 * Set to true if there is no more input data.
	 */
	bool depleted;

	/**
	 * Queue containing the data that has been fed into the char reader.
	 */
	std::deque<std::string> buffer;

	/**
	 * Substitute any combination of linebreaks in the incomming code with "\n".
	 * Returns true if the current character is meant as output, false
	 * otherwise.
	 */
	bool substituteLinebreaks(ReadCursor &cursor, char *c);

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
	bool readCharacterAtCursor(ReadCursor &cursor, char *c);

	/**
	 * Function that is called for each read character -- updates the row and
	 * column count.
	 */
	void updatePositionCounters(const char c);

public:

	/**
	 * Constructor of the buffered char reader class with empty buffer as input.
	 * This operates the BufferedCharReader in a mode where new data has to be
	 * fed using the "feed" function and explicitly closed using the "close"
	 * function.
	 *
	 * @param line is the start line.
	 * @param column is the start column.
	 */
	BufferedCharReader(int line = 1, int column = 1);


	/**
	 * Constructor of the buffered char reader class with a string as input.
	 *
	 * @param str is a string containing the input data.
	 * @param line is the start line.
	 * @param column is the start column.
	 */
	BufferedCharReader(const std::string &str, int line = 1, int column = 1);

	/**
	 * Constructor of the buffered char reader class with a string as input.
	 *
	 * @param inputStream is the input stream from which incomming data should
	 * be read.
	 * @param line is the start line.
	 * @param column is the start column.
	 */
	BufferedCharReader(std::istream &inputStream, int line = 1, int column = 1);

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
	 * Feeds new data into the internal buffer of the BufferedCharReader
	 * class. Only applicable if the buffered char reader was constructed
	 * without an input stream or string.
	 *
	 * @param data is a string containing the data that should be
	 * appended to the internal buffer.
	 */
	void feed(const std::string &data);

	/**
	 * Tells the buffered char reader that no more data will be fed.
	 * Only applicable if the buffered char reader was constructed without an
	 * input stream or string.
	 *
	 * @param data is a string containing the data that should be
	 * appended to the internal buffer.
	 */
	void close();

	/**
	 * Returns true if there are no more characters as the stream was
	 * closed.
	 *
	 * @return true if there is no more data.
	 */
	bool atEnd() const;

	/**
	 * Returns the current line (starting with one).
	 *
	 * @return the current line number.
	 */
	int getLine() const { return readCursor.line; }

	/**
	 * Returns the current column (starting with one).
	 *
	 * @return the current column number.
	 */
	int getColumn() const { return readCursor.column; }
};
}

#endif /* _OUSIA_BUFFERED_CHAR_READER_H_ */

