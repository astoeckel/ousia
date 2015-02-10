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

/**
 * Plain format default tokenizer.
 */
class PlainFormatTokens : public DynamicTokenizer {
public:
	/**
	 * Id of the backslash token.
	 */
	TokenTypeId Backslash;

	/**
	 * Id of the line comment token.
	 */
	TokenTypeId LineComment;

	/**
	 * Id of the block comment start token.
	 */
	TokenTypeId BlockCommentStart;

	/**
	 * Id of the block comment end token.
	 */
	TokenTypeId BlockCommentEnd;

	/**
	 * Id of the field start token.
	 */
	TokenTypeId FieldStart;

	/**
	 * Id of the field end token.
	 */
	TokenTypeId FieldEnd;

	/**
	 * Registers the plain format tokens in the internal tokenizer.
	 */
	PlainFormatTokens()
	{
		Backslash = registerToken("\\");
		LineComment = registerToken("%");
		BlockCommentStart = registerToken("%{");
		BlockCommentEnd = registerToken("}%");
		FieldStart = registerToken("{");
		FieldEnd = registerToken("}");
	}
};

static const PlainFormatTokens Tokens;

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

PlainFormatStreamReader::PlainFormatStreamReader(CharReader &reader,
                                                 Logger &logger)
    : reader(reader), logger(logger), tokenizer(Tokens)
{
	// Place an intial command representing the complete file on the stack
	commands.push(Command{"", Variant::mapType{}, true, true, true});
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

PlainFormatStreamReader::State PlainFormatStreamReader::parseBeginCommand()
{
	// Expect a '{' after the command
	reader.consumeWhitespace();
	if (!reader.expect('{')) {
		logger.error("Expected \"{\" after \\begin", reader);
		return State::NONE;
	}

	// Parse the name of the command that should be opened
	Variant commandName = parseIdentifier(reader.getOffset());
	if (commandName.asString().empty()) {
		logger.error("Expected identifier", commandName);
		return State::ERROR;
	}

	// Check whether the next character is a '#', indicating the start of the
	// command name
	Variant commandArgName;
	SourceOffset start = reader.getOffset();
	if (reader.expect('#')) {
		commandArgName = parseIdentifier(start);
		if (commandArgName.asString().empty()) {
			logger.error("Expected identifier after \"#\"", commandArgName);
		}
	}

	if (!reader.expect('}')) {
		logger.error("Expected \"}\"", reader);
		return State::ERROR;
	}

	// Parse the arguments
	Variant commandArguments = parseCommandArguments(std::move(commandArgName));

	// Push the command onto the command stack
	pushCommand(std::move(commandName), std::move(commandArguments), true);

	return State::COMMAND;
}

static bool checkStillInField(const PlainFormatStreamReader::Command &cmd,
                              const Variant &endName, Logger &logger)
{
	if (cmd.inField && !cmd.inRangeField) {
		logger.error(std::string("\\end in open field of command \"") +
		                 cmd.name.asString() + std::string("\""),
		             endName);
		logger.note(std::string("Open command started here:"), cmd.name);
		return true;
	}
	return false;
}

PlainFormatStreamReader::State PlainFormatStreamReader::parseEndCommand()
{
	// Expect a '{' after the command
	if (!reader.expect('{')) {
		logger.error("Expected \"{\" after \\end", reader);
		return State::NONE;
	}

	// Fetch the name of the command that should be ended here
	Variant name = parseIdentifier(reader.getOffset());

	// Make sure the given command name is not empty
	if (name.asString().empty()) {
		logger.error("Expected identifier", name);
		return State::ERROR;
	}

	// Make sure the command name is terminated with a '}'
	if (!reader.expect('}')) {
		logger.error("Expected \"}\"", reader);
		return State::ERROR;
	}

	// Unroll the command stack up to the last range command
	while (!commands.top().hasRange) {
		if (checkStillInField(commands.top(), name, logger)) {
			return State::ERROR;
		}
		commands.pop();
	}

	// Make sure we're not in an open field of this command
	if (checkStillInField(commands.top(), name, logger)) {
		return State::ERROR;
	}

	// Special error message if the top-level command is reached
	if (commands.size() == 1) {
		logger.error(std::string("Cannot end command \"") + name.asString() +
		                 std::string("\" here, no command open"),
		             name);
		return State::ERROR;
	}

	// Inform the about command mismatches
	const Command &cmd = commands.top();
	if (commands.top().name.asString() != name.asString()) {
		logger.error(std::string("Trying to end command \"") +
		                 cmd.name.asString() +
		                 std::string(", but open command is \"") +
		                 name.asString() + std::string("\""),
		             name);
		logger.note("Last command was opened here:", cmd.name);
		return State::ERROR;
	}

	// Set the location to the location of the command that was ended, then end
	// the current command
	location = name.getLocation();
	commands.pop();
	return cmd.inRangeField ? State::FIELD_END : State::NONE;
}

Variant PlainFormatStreamReader::parseCommandArguments(Variant commandArgName)
{
	// Parse the arguments using the universal VariantReader
	Variant commandArguments;
	if (reader.expect('[')) {
		auto res = VariantReader::parseObject(reader, logger, ']');
		commandArguments = res.second;
	} else {
		commandArguments = Variant::mapType{};
	}

	// Insert the parsed name, make sure "name" was not specified in the
	// arguments
	if (commandArgName.isString()) {
		auto res =
		    commandArguments.asMap().emplace("name", std::move(commandArgName));
		if (!res.second) {
			logger.error("Name argument specified multiple times",
			             SourceLocation{}, MessageMode::NO_CONTEXT);
			logger.note("First occurance is here: ", commandArgName);
			logger.note("Second occurance is here: ", res.first->second);
		}
	}
	return commandArguments;
}

void PlainFormatStreamReader::pushCommand(Variant commandName,
                                          Variant commandArguments,
                                          bool hasRange)
{
	// Store the location on the stack
	location = commandName.getLocation();

	// Place the command on the command stack, remove the last commands if we're
	// not currently inside a field of these commands
	while (!commands.top().inField) {
		commands.pop();
	}
	commands.push(Command{std::move(commandName), std::move(commandArguments),
	                      hasRange, false, false});
}

PlainFormatStreamReader::State PlainFormatStreamReader::parseCommand(
    size_t start)
{
	// Parse the commandName as a first identifier
	Variant commandName = parseIdentifier(start);

	// Handle the special "begin" and "end" commands
	if (commandName.asString() == "begin") {
		return parseBeginCommand();
	} else if (commandName.asString() == "end") {
		return parseEndCommand();
	}

	// Check whether the next character is a '#', indicating the start of the
	// command name
	Variant commandArgName;
	start = reader.getOffset();
	if (reader.expect('#')) {
		commandArgName = parseIdentifier(start);
		if (commandArgName.asString().empty()) {
			logger.error("Expected identifier after \"#\"", commandArgName);
		}
	}

	// Parse the arugments
	Variant commandArguments = parseCommandArguments(std::move(commandArgName));

	// Push the command onto the command stack
	pushCommand(std::move(commandName), std::move(commandArguments), false);

	return State::COMMAND;
}

void PlainFormatStreamReader::parseBlockComment()
{
	DynamicToken token;
	size_t depth = 1;
	while (tokenizer.read(reader, token)) {
		if (token.type == Tokens.BlockCommentEnd) {
			depth--;
			if (depth == 0) {
				return;
			}
		}
		if (token.type == Tokens.BlockCommentStart) {
			depth++;
		}
	}

	// Issue an error if the file ends while we are in a block comment
	logger.error("File ended while being in a block comment", reader);
}

void PlainFormatStreamReader::parseLineComment()
{
	char c;
	while (reader.read(c)) {
		if (c == '\n') {
			return;
		}
	}
}

bool PlainFormatStreamReader::checkIssueData(DataHandler &handler)
{
	if (!handler.isEmpty()) {
		data = handler.toVariant(reader.getSourceId());
		location = data.getLocation();
		reader.resetPeek();
		return true;
	}
	return false;
}

bool PlainFormatStreamReader::checkIssueFieldStart()
{
	// Fetch the current command, and check whether we're currently inside a
	// field of this command
	Command &cmd = commands.top();
	if (!cmd.inField) {
		// If this is a range command, we're now implicitly inside the field of
		// this command -- we'll have to issue a field start command!
		if (cmd.hasRange) {
			cmd.inField = true;
			cmd.inRangeField = true;
			reader.resetPeek();
			return true;
		}

		// This was not a range command, so obviously we're now inside within
		// a field of some command -- so unroll the commands stack until a
		// command with open field is reached
		while (!commands.top().inField) {
			commands.pop();
		}
	}
	return false;
}

PlainFormatStreamReader::State PlainFormatStreamReader::parse()
{
	// Handler for incomming data
	DataHandler handler;

	// Read tokens until the outer loop should be left
	DynamicToken token;
	while (tokenizer.peek(reader, token)) {
		const TokenTypeId type = token.type;

		// Special handling for Backslash and Text
		if (type == Tokens.Backslash) {
			// Before appending anything to the output data or starting a new
			// command, check whether FIELD_START has to be issued, as the
			// current command is a command with range
			if (checkIssueFieldStart()) {
				location = token.location;
				return State::FIELD_START;
			}

			// Check whether a command starts now, without advancing the peek
			// cursor
			char c;
			if (!reader.fetchPeek(c)) {
				logger.error("Trailing backslash at the end of the file.",
				             token);
				return State::END;
			}

			// Try to parse a command
			if (Utils::isIdentifierStartCharacter(c)) {
				// Make sure to issue any data before it is to late
				if (checkIssueData(handler)) {
					return State::DATA;
				}

				// Parse the actual command
				State res = parseCommand(token.location.getStart());
				switch (res) {
					case State::ERROR:
						throw LoggableException(
						    "Last error was irrecoverable, ending parsing "
						    "process");
					case State::NONE:
						continue;
					default:
						return res;
				}
			}

			// This was not a special character, just append the given character
			// to the data buffer, use the escape character start as start
			// location and the peek offset as end location
			reader.peek(c);  // Peek the previously fetched character
			handler.append(c, token.location.getStart(),
			               reader.getPeekOffset());
			reader.consumePeek();
			continue;
		} else if (type == TextToken) {
			// Check whether FIELD_START has to be issued before appending text
			if (checkIssueFieldStart()) {
				location = token.location;
				return State::FIELD_START;
			}

			// Append the text to the data handler
			handler.append(token.content, token.location.getStart(),
			               token.location.getEnd());

			reader.consumePeek();
			continue;
		}

		// A non-text token was reached, make sure all pending data commands
		// have been issued
		if (checkIssueData(handler)) {
			return State::DATA;
		}

		// We will handle the token now, consume the peeked characters
		reader.consumePeek();

		// Update the location to the current token location
		location = token.location;

		if (token.type == Tokens.LineComment) {
			parseLineComment();
		} else if (token.type == Tokens.BlockCommentStart) {
			parseBlockComment();
		} else if (token.type == Tokens.FieldStart) {
			Command &cmd = commands.top();
			if (!cmd.inField) {
				cmd.inField = true;
				return State::FIELD_START;
			}
			logger.error(
			    "Got field start token \"{\", but no command for which to "
			    "start the field. Did you mean \"\\{\"?",
			    token);
		} else if (token.type == Tokens.FieldEnd) {
			// Try to end an open field of the current command -- if the current
			// command is not inside an open field, end this command and try to
			// close the next one
			for (int i = 0; i < 2 && commands.size() > 1; i++) {
				Command &cmd = commands.top();
				if (!cmd.inRangeField) {
					if (cmd.inField) {
						cmd.inField = false;
						return State::FIELD_END;
					}
					commands.pop();
				} else {
					break;
				}
			}
			logger.error(
			    "Got field end token \"}\" but there is no field to end. Did "
			    "you mean \"\\}\"?",
			    token);
		} else {
			logger.error("Unexpected token \"" + token.content + "\"", token);
		}
	}

	// Issue available data
	if (checkIssueData(handler)) {
		return State::DATA;
	}

	// Make sure all open commands and fields have been ended at the end of the
	// stream
	while (commands.size() > 1) {
		Command &cmd = commands.top();
		if (cmd.inField || cmd.hasRange) {
			logger.error("Reached end of stream, but command \"" +
			                 cmd.name.asString() + "\" has not been ended",
			             cmd.name);
		}
		commands.pop();
	}

	location = SourceLocation{reader.getSourceId(), reader.getOffset()};
	return State::END;
}

const Variant &PlainFormatStreamReader::getCommandName()
{
	return commands.top().name;
}

const Variant &PlainFormatStreamReader::getCommandArguments()
{
	return commands.top().arguments;
}
}

