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

#include <sstream>
#include <unordered_set>

#include <core/common/CharReader.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Utils.hpp>

#include "PlainFormatStreamReader.hpp"

namespace ousia {

/* Internally used types, protected from spilling the exports by a namespace */

namespace {
/**
 * Enum used to specify the state of the parseBlockComment state machine.
 */
enum class BlockCommentState { DEFAULT, HAS_CURLY_CLOSE, HAS_PERCENT };

/**
 * Class taking care of recording plain text data found withing the file.
 */
class DataHandler {
private:
	/**
	 * Const reference at the reader, used for reading the current location.
	 */
	const CharReader &reader;

	/**
	 * Flag defining whether whitespaces should be preserved or not.
	 */
	const bool preserveWhitespaces;

	/**
	 * Current source range of the data in the buffer.
	 */
	SourceLocation location;

	/**
	 * Current buffer containing all read characters.
	 */
	std::stringstream buffer;

	/**
	 * Set to false, once a non-whitespace character was reached.
	 */
	bool empty;

	/**
	 * Set to true if a whitespace was found -- these are normalized to a single
	 * space.
	 */
	bool hasWhitespace;

public:
	/**
	 * Constructor of the DataHandler class.
	 *
	 * @param reader is the CharReader that should be used for reading the data
	 * location.
	 * @param preserveWhitespaces should be set to true if all whitespaces
	 * should be preserved (for preformated environments).
	 */
	DataHandler(const CharReader &reader, bool preserveWhitespaces = false)
	    : reader(reader),
	      preserveWhitespaces(preserveWhitespaces),
	      location(reader.getSourceId()),
	      empty(true),
	      hasWhitespace(false)
	{
	}

	/**
	 * Appends the given character to the internal buffer.
	 *
	 * @param c is the character that should be appended.
	 * @param wasEscaped is set to true if the character was escaped (prepended
	 * with a backslash), this allows whitespace characters to be explicitly
	 * included.
	 */
	void append(char c, bool wasEscaped = false)
	{
		// Check whether the character is a whitespace
		const bool isWhitespace =
		    !wasEscaped && !preserveWhitespaces && Utils::isWhitespace(c);

		// Trim leading and trailing whitespaces
		if (isWhitespace) {
			if (!empty) {
				hasWhitespace = true;
			}
		} else {
			// Compress whitespaces to a single space
			if (hasWhitespace) {
				buffer << ' ';
				hasWhitespace = false;
			}

			// Append the character
			buffer << c;

			// Update the "empty" flag and set the start and end offset
			if (empty) {
				location.setStart(reader.getOffset());
				empty = false;
			}
			location.setEnd(reader.getPeekOffset());
		}
	}

	/**
	 * Returns true if no non-whitespace character has been found until now.
	 *
	 * @return true if the internal buffer is still empty.
	 */
	bool isEmpty() { return empty; }

	/**
	 * Returns a variant containg the read data and its location.
	 *
	 * @return a variant with a string value containing the read data and the
	 * location being set to
	 */
	Variant getData()
	{
		Variant res = Variant::fromString(buffer.str());
		res.setLocation(location);
		return res;
	}
};
}

PlainFormatStreamReader::PlainFormatStreamReader(CharReader &reader,
                                                 Logger &logger)
    : reader(reader), logger(logger), fieldIdx(0)
{
}

/* Comment handling */

void PlainFormatStreamReader::parseBlockComment()
{
	char c;
	BlockCommentState state = BlockCommentState::DEFAULT;
	while (reader.read(c)) {
		switch (state) {
			case BlockCommentState::DEFAULT:
				if (c == '%') {
					state = BlockCommentState::HAS_PERCENT;
				} else if (c == '}') {
					state = BlockCommentState::HAS_CURLY_CLOSE;
				}
				break;
			case BlockCommentState::HAS_PERCENT:
				if (c == '{') {
					parseBlockComment();
				}
				state = BlockCommentState::DEFAULT;
				break;
			case BlockCommentState::HAS_CURLY_CLOSE:
				if (c == '%') {
					return;
				}
				state = BlockCommentState::DEFAULT;
				break;
		}
	}

	// Issue an error if the file ends while we are in a block comment
	logger.error("File ended while being in a block comment", reader);
}

void PlainFormatStreamReader::parseComment()
{
	char c;
	bool first = true;
	reader.consumePeek();
	while (reader.read(c)) {
		// Continue parsing a block comment if a '{' is found
		if (c == '{' && first) {
			parseBlockComment();
			return;
		}
		if (c == '\n') {
			return;
		}
		first = false;
	}
}

/* Top level parse function */

static const std::unordered_set<char> EscapeableCharacters{'\\', '<', '>',
                                                    '{',  '}', '%'};

PlainFormatStreamReader::State PlainFormatStreamReader::parse()
{
// Macro (sorry for that) used for checking whether there is data to issue, and
// if yes, aborting the loop, allowing for a reentry on a later parse call by
// resetting the peek cursor
#define CHECK_ISSUE_DATA()      \
	{                           \
		if (!dataHandler.isEmpty()) {   \
			reader.resetPeek(); \
			abort = true;       \
			break;              \
		}                       \
	}

	// Data handler
	DataHandler dataHandler(reader);

	// Variable set to true if the parser loop should be left
	bool abort = false;

	// Happily add characters to the dataHandler and handle escaping until a
	// special character is reached. Then go to a specialiced parsing routine
	char c;
	while (!abort && reader.peek(c)) {
		switch (c) {
			case '\\':
				reader.peek(c);
				// Check whether this backslash just escaped some special or
				// whitespace character or was the beginning of a command
				if (EscapeableCharacters.count(c) == 0 &&
				    !Utils::isWhitespace(c)) {
					CHECK_ISSUE_DATA();
					// TODO: Parse command (starting from the backslash)
					return State::COMMAND;
				}
				// A character was escaped, add it to the buffer, with the
				// wasEscaped flag set to true
				dataHandler.append(c, true);
				break;
			case '<':
				// TODO: Annotations
				break;
			case '>':
				// TODO: Annotations
				break;
			case '{':
				// TODO: Issue start of field
				break;
			case '}':
			// TODO: Issue end of field
			case '%':
				CHECK_ISSUE_DATA();
				parseComment();
				break;
			case '\n':
				CHECK_ISSUE_DATA();
				reader.consumePeek();
				return State::LINEBREAK;
			default:
				dataHandler.append(c, false);
		}

		// Consume the peeked character if we did not abort, otherwise abort
		if (!abort) {
			reader.consumePeek();
		} else {
			break;
		}
	}

	// Send out pending output data, otherwise we are at the end of the stream
	if (!dataHandler.isEmpty()) {
		data = dataHandler.getData();
		return State::DATA;
	}
	return State::END;
#undef CHECK_ISSUE_DATA
}
}

