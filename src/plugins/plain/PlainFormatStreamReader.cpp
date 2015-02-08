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

#include <core/common/CharReader.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Utils.hpp>

#include "PlainFormatStreamReader.hpp"

namespace ousia {

namespace {

/**
 * Class used internally to collect data issued via "DATA" event.
 */
class DataHandler {
private:
	/**
	 * Internal character buffer.
	 */
	std::vector<char> buf;

	/**
	 * Start location of the character data.
	 */
	SourceOffset start;

	/**
	 * End location of the character data.
	 */
	SourceOffset end;

public:

	/**
	 * Default constructor, initializes start and end with zeros.
	 */
	DataHandler() : start(0), end(0) {}

	/**
	 * Returns true if the internal buffer is empty.
	 *
	 * @return true if no characters were added to the internal buffer, false
	 * otherwise.
	 */
	bool isEmpty() { return buf.empty(); }

	/**
	 * Appends a single character to the internal buffer.
	 *
	 * @param c is the character that should be added to the internal buffer.
	 * @param charStart is the start position of the character.
	 * @param charEnd is the end position of the character.
	 */
	void append(char c, SourceOffset charStart, SourceOffset charEnd)
	{
		if (isEmpty()) {
			start = charStart;
		}
		buf.push_back(c);
		end = charEnd;
	}

	/**
	 * Appends a string to the internal buffer.
	 *
	 * @param s is the string that should be added to the internal buffer.
	 * @param stringStart is the start position of the string.
	 * @param stringEnd is the end position of the string.
	 */
	void append(const std::string &s, SourceOffset stringStart,
	            SourceOffset stringEnd)
	{
		if (isEmpty()) {
			start = stringStart;
		}
		std::copy(s.c_str(), s.c_str() + s.size(), back_inserter(buf));
		end = stringEnd;
	}

	/**
	 * Converts the internal buffer to a variant with attached location
	 * information.
	 *
	 * @param sourceId is the source id which is needed for building the
	 * location information.
	 * @return a Variant with the internal buffer content as string and
	 * the correct start and end location.
	 */
	Variant toVariant(SourceId sourceId)
	{
		Variant res = Variant::fromString(std::string(buf.data(), buf.size()));
		res.setLocation({sourceId, start, end});
		return res;
	}
};
}

PlainFormatStreamReader::PlainFormatStreamReader(CharReader &reader,
                                                 Logger &logger)
    : reader(reader), logger(logger), fieldIdx(0)
{
	tokenBackslash = tokenizer.registerToken("\\");
	tokenLinebreak = tokenizer.registerToken("\n");
	tokenLineComment = tokenizer.registerToken("%");
	tokenBlockCommentStart = tokenizer.registerToken("%{");
	tokenBlockCommentEnd = tokenizer.registerToken("}%");
}

void PlainFormatStreamReader::parseBlockComment()
{
	DynamicToken token;
	size_t depth = 1;
	while (tokenizer.read(reader, token)) {
		if (token.type == tokenBlockCommentEnd) {
			depth--;
			if (depth == 0) {
				return;
			}
		}
		if (token.type == tokenBlockCommentStart) {
			depth++;
		}
	}

	// Issue an error if the file ends while we are in a block comment
	logger.error("File ended while being in a block comment", reader);
}

void PlainFormatStreamReader::parseLineComment()
{
	char c;
	reader.consumePeek();
	while (reader.read(c)) {
		if (c == '\n') {
			return;
		}
	}
}

PlainFormatStreamReader::State PlainFormatStreamReader::parse()
{
// Macro (sorry for that) used for checking whether there is data to issue, and
// if yes, aborting the loop, allowing for a reentry on a later parse call by
// resetting the peek cursor
#define CHECK_ISSUE_DATA()            \
	{                                 \
		if (!dataHandler.isEmpty()) { \
			reader.resetPeek();       \
			abort = true;             \
			break;                    \
		}                             \
	}

	// Handler for incomming data
	DataHandler dataHandler;

	// Variable set to true if the parser loop should be left
	bool abort = false;

	// Read tokens until the outer loop should be left
	DynamicToken token;
	while (!abort && tokenizer.peek(reader, token)) {
		// Check whether this backslash just escaped some special or
		// whitespace character or was the beginning of a command
		if (token.type == tokenBackslash) {
			// Check whether this character could be the start of a command
			char c;
			reader.consumePeek();
			reader.peek(c);
			if (Utils::isIdentifierStart(c)) {
				CHECK_ISSUE_DATA();
				// TODO: Parse a command
				return State::COMMAND;
			}

			// This was not a special character, just append the given character
			// to the data buffer, use the escape character start as start
			// location and the peek offset as end location
			dataHandler.append(c, token.location.getStart(),
			                   reader.getPeekOffset());
		} else if (token.type == tokenLineComment) {
			CHECK_ISSUE_DATA();
			reader.consumePeek();
			parseLineComment();
		} else if (token.type == tokenBlockCommentStart) {
			CHECK_ISSUE_DATA();
			reader.consumePeek();
			parseBlockComment();
		} else if (token.type == tokenLinebreak) {
			CHECK_ISSUE_DATA();
			reader.consumePeek();
			return State::LINEBREAK;
		} else if (token.type == TextToken) {
			dataHandler.append(token.content, token.location.getStart(),
			                   token.location.getEnd());
		}

		// Consume the peeked character if we did not abort, otherwise abort
		if (!abort) {
			reader.consumePeek();
		}
	}

	// Send out pending output data, otherwise we are at the end of the stream
	if (!dataHandler.isEmpty()) {
		data = dataHandler.toVariant(reader.getSourceId());
		return State::DATA;
	}
	return State::END;
#undef CHECK_ISSUE_DATA
}
}

