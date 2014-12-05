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

#include <array>

#include "Utils.hpp"

#include "BufferedCharReader.hpp"

namespace ousia {

// Constants used within the linebreak statemachine.
static const uint8_t LB_STATE_NONE = 0x00;
static const uint8_t LB_STATE_ONE = 0x01;
static const uint8_t LB_STATE_LF = 0x10;
static const uint8_t LB_STATE_CR = 0x20;
static const uint8_t LB_STATE_MASK_CNT = 0x0F;
static const uint8_t LB_STATE_MASK_TYPE = 0xF0;

/* Struct BufferedCharReader::ReadCursor */

BufferedCharReader::ReadCursor::ReadCursor(unsigned int line,
                                           unsigned int column,
                                           bool destructive)
    : line(line),
      column(column),
      bufferElem(0),
      bufferPos(0),
      destructive(destructive),
      lbState(LB_STATE_NONE)
{
}

void BufferedCharReader::ReadCursor::assign(const ReadCursor &cursor)
{
	this->line = cursor.line;
	this->column = cursor.column;
	this->bufferElem = cursor.bufferElem;
	this->bufferPos = cursor.bufferPos;
	this->lbState = cursor.lbState;
}

/* Class BufferedCharReader */

BufferedCharReader::BufferedCharReader(int line, int column)
    : inputStream(nullptr),
      readCursor(line, column, true),
      peekCursor(line, column, false),
      depleted(false)
{
}

BufferedCharReader::BufferedCharReader(const std::string &str, int line,
                                       int column)
    : inputStream(nullptr),
      readCursor(line, column, true),
      peekCursor(line, column, false),
      depleted(true)
{
	buffer.push_back(str);
}

BufferedCharReader::BufferedCharReader(const std::string &str)
    : inputStream(nullptr),
      readCursor(1, 1, true),
      peekCursor(1, 1, false),
      depleted(true)
{
	buffer.push_back(str);
}

BufferedCharReader::BufferedCharReader(std::istream &inputStream, int line,
                                       int column)
    : inputStream(&inputStream),
      readCursor(line, column, true),
      peekCursor(line, column, false),
      depleted(false)
{
}

void BufferedCharReader::feed(const std::string &data)
{
	if (!depleted && !inputStream) {
		buffer.push_back(data);
	}
}

void BufferedCharReader::close()
{
	if (!inputStream) {
		depleted = true;
	}
}

bool BufferedCharReader::substituteLinebreaks(ReadCursor &cursor, char *c)
{
	// Handle line breaks, inserts breakes after the following character
	// combinations: \n, \r, \n\r, \r\n TODO: Change behaviour to \n, \n\r, \r\n
	if ((*c == '\n') || (*c == '\r')) {
		// Determine the type of the current linebreak character
		const uint8_t type = (*c == '\n') ? LB_STATE_LF : LB_STATE_CR;

		// Read the last count and the last type from the state
		const uint8_t lastCount = cursor.lbState & LB_STATE_MASK_CNT;
		const uint8_t lastType = cursor.lbState & LB_STATE_MASK_TYPE;

		// Set the current linebreak type and counter in the state
		cursor.lbState = ((lastCount + 1) & 1) | type;

		// If either this is the first instance of this character or the same
		// return character is repeated
		if (!lastCount || (lastType == type)) {
			*c = '\n';
			return true;
		}
		return false;
	}

	// Find the state
	cursor.lbState = LB_STATE_NONE;
	return true;
}

bool BufferedCharReader::readCharacterAtCursor(ReadCursor &cursor, char *c)
{
	bool hasChar = false;
	while (!hasChar) {
		// Abort if the current buffer element does not point to a valid entry
		// in the buffer -- we must try to feed another data block into the
		// internal buffer
		if (cursor.bufferElem >= buffer.size()) {
			// Abort if there is no more data or no input stream is set
			if (depleted || !inputStream) {
				return false;
			}

			// Read a buffer of the specified size
			constexpr std::streamsize BUFFER_SIZE = 1024;
			std::array<char, BUFFER_SIZE> buf;
			const std::streamsize cnt =
			    (*inputStream).read(buf.data(), BUFFER_SIZE).gcount();

			// If data has been read, append it to the input buffer and try
			// again
			if (cnt > 0) {
				buffer.emplace_back(std::string(buf.data(), cnt));
				continue;
			}

			// End of file handling
			if (inputStream->fail() || inputStream->eof()) {
				depleted = true;
				return false;
			}
		}

		// Fetch the current element the peek pointer points to
		const std::string &data = buffer[cursor.bufferElem];

		// Handle the "no data" case -- either in a destructive or
		// non-destructive manner.
		if (cursor.bufferPos >= data.length()) {
			if (cursor.destructive) {
				buffer.pop_front();
			} else {
				cursor.bufferElem++;
			}
			cursor.bufferPos = 0;
			continue;
		}

		// Read the character, advance the buffer position
		*c = *(data.data() + cursor.bufferPos);
		cursor.bufferPos++;

		// Substitute linebreaks with a single LF (0x0A)
		hasChar = substituteLinebreaks(cursor, c);
	}

	// Update the position counter
	if (*c == '\n') {
		cursor.line++;
		cursor.column = 1;
	} else {
		// Ignore UTF-8 continuation bytes
		if (!((*c & 0x80) && !(*c & 0x40))) {
			cursor.column++;
		}
	}

	return true;
}

bool BufferedCharReader::peek(char *c)
{
	return readCharacterAtCursor(peekCursor, c);
}

bool BufferedCharReader::read(char *c)
{
	resetPeek();
	return readCharacterAtCursor(readCursor, c);
}

void BufferedCharReader::consumePeek()
{
	// Remove all no longer needed buffer elements
	for (unsigned int i = 0; i < peekCursor.bufferElem; i++) {
		buffer.pop_front();
	}
	peekCursor.bufferElem = 0;

	// Copy the peek cursor to the read cursor
	readCursor.assign(peekCursor);
}

bool BufferedCharReader::consumeWhitespace()
{
	char c;
	while (peek(&c)) {
		if (!Utils::isWhitespace(c)) {
			resetPeek();
			return true;
		}
		consumePeek();
	}
	return false;
}

void BufferedCharReader::resetPeek()
{
	// Reset the peek cursor to the read cursor
	peekCursor.assign(readCursor);
}

bool BufferedCharReader::atEnd() const
{
	if (depleted || !inputStream) {
		if (buffer.size() <= 0) {
			return true;
		} else if (buffer.size() == 1) {
			return buffer[0].size() == readCursor.bufferPos;
		}
	}
	return false;
}
}

