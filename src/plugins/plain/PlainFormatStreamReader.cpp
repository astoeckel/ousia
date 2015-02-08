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
#include <core/common/VariantReader.hpp>

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

Variant PlainFormatStreamReader::parseIdentifier(size_t start)
{
	bool first = true;
	std::vector<char> identifier;
	size_t end = reader.getPeekOffset();
	char c;
	while (reader.peek(c)) {
		// Abort if this character is not a valid identifer character
		if ((first && Utils::isIdentifierStartCharacter(c)) ||
		    (!first && Utils::isIdentifierCharacter(c))) {
			identifier.push_back(c);
		} else {
			reader.resetPeek();
			break;
		}

		// This is no longer the first character
		first = false;
		end = reader.getPeekOffset();
		reader.consumePeek();
	}

	// Return the identifier at its location
	Variant res =
	    Variant::fromString(std::string(identifier.data(), identifier.size()));
	res.setLocation({reader.getSourceId(), start, end});
	return res;
}

void PlainFormatStreamReader::parseCommand(size_t start)
{
	// Parse the commandName as a first identifier
	commandName = parseIdentifier(start);

	// Check whether the next character is a '#', indicating the start of the
	// command name
	Variant commandArgName;
	start = reader.getOffset();
	if (reader.expect('#')) {
		commandArgName = parseIdentifier(start);
		if (commandArgName.asString().empty()) {
			logger.error("Expected identifier after '#'", commandArgName);
		}
	}

	// Read the arguments (if they are available), otherwise reset them
	if (reader.expect('[')) {
		auto res = VariantReader::parseObject(reader, logger, ']');
		commandArguments = res.second;
	} else {
		commandArguments = Variant::mapType{};
	}

	// Insert the parsed name, make sure "name" was not specified in the
	// arguments
	if (commandArgName.isString()) {
		auto res = commandArguments.asMap().emplace("name", commandArgName);
		if (!res.second) {
			logger.error("Name argument specified multiple times",
			             SourceLocation{}, MessageMode::NO_CONTEXT);
			logger.note("First occurance is here: ", commandArgName);
			logger.note("Second occurance is here: ", res.first->second);
		}
	}
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
			if (Utils::isIdentifierStartCharacter(c)) {
				CHECK_ISSUE_DATA();
				reader.resetPeek();
				parseCommand(token.location.getStart());
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

