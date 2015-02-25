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

#include "OsmlStreamParser.hpp"

namespace ousia {

/**
 * Plain format default tokenizer.
 */
class PlainFormatTokens : public Tokenizer {
public:
	/**
	 * Id of the backslash token.
	 */
	TokenId Backslash;

	/**
	 * Id of the line comment token.
	 */
	TokenId LineComment;

	/**
	 * Id of the block comment start token.
	 */
	TokenId BlockCommentStart;

	/**
	 * Id of the block comment end token.
	 */
	TokenId BlockCommentEnd;

	/**
	 * Id of the field start token.
	 */
	TokenId FieldStart;

	/**
	 * Id of the field end token.
	 */
	TokenId FieldEnd;

	/**
	 * Id of the default field start token.
	 */
	TokenId DefaultFieldStart;

	/**
	 * Id of the annotation start token.
	 */
	TokenId AnnotationStart;

	/**
	 * Id of the annotation end token.
	 */
	TokenId AnnotationEnd;

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
		DefaultFieldStart = registerToken("{!");
		AnnotationStart = registerToken("<\\");
		AnnotationEnd = registerToken("\\>");
	}
};

static const PlainFormatTokens OsmlTokens;

OsmlStreamParser::OsmlStreamParser(CharReader &reader, Logger &logger)
    : reader(reader),
      logger(logger),
      tokenizer(OsmlTokens),
      data(reader.getSourceId())
{
	// Place an intial command representing the complete file on the stack
	commands.push(Command{"", Variant::mapType{}, true, true, true, false});
}

Variant OsmlStreamParser::parseIdentifier(size_t start, bool allowNSSep)
{
	bool first = true;
	bool hasCharSinceNSSep = false;
	std::vector<char> identifier;
	size_t end = reader.getPeekOffset();
	char c, c2;
	while (reader.peek(c)) {
		// Abort if this character is not a valid identifer character
		if ((first && Utils::isIdentifierStartCharacter(c)) ||
		    (!first && Utils::isIdentifierCharacter(c))) {
			identifier.push_back(c);
		} else if (c == ':' && hasCharSinceNSSep && reader.fetchPeek(c2) &&
		           Utils::isIdentifierStartCharacter(c2)) {
			identifier.push_back(c);
		} else {
			if (c == ':' && allowNSSep) {
				logger.error(
				    "Expected character before and after namespace separator "
				    "\":\"",
				    reader);
			}
			reader.resetPeek();
			break;
		}

		// This is no longer the first character
		first = false;

		// Advance the hasCharSinceNSSep flag
		hasCharSinceNSSep = allowNSSep && (c != ':');

		end = reader.getPeekOffset();
		reader.consumePeek();
	}

	// Return the identifier at its location
	Variant res =
	    Variant::fromString(std::string(identifier.data(), identifier.size()));
	res.setLocation({reader.getSourceId(), start, end});
	return res;
}

OsmlStreamParser::State OsmlStreamParser::parseBeginCommand()
{
	// Expect a '{' after the command
	reader.consumeWhitespace();
	if (!reader.expect('{')) {
		logger.error("Expected \"{\" after \\begin", reader);
		return State::NONE;
	}

	// Parse the name of the command that should be opened
	Variant commandName = parseIdentifier(reader.getOffset(), true);
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

static bool checkStillInField(const OsmlStreamParser::Command &cmd,
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

OsmlStreamParser::State OsmlStreamParser::parseEndCommand()
{
	// Expect a '{' after the command
	if (!reader.expect('{')) {
		logger.error("Expected \"{\" after \\end", reader);
		return State::NONE;
	}

	// Fetch the name of the command that should be ended here
	Variant name = parseIdentifier(reader.getOffset(), true);

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
		                 std::string("\", but open command is \"") +
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

Variant OsmlStreamParser::parseCommandArguments(Variant commandArgName)
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

void OsmlStreamParser::pushCommand(Variant commandName,
                                   Variant commandArguments, bool hasRange)
{
	// Store the location on the stack
	location = commandName.getLocation();

	// Place the command on the command stack, remove the last commands if we're
	// not currently inside a field of these commands
	while (!commands.top().inField) {
		commands.pop();
	}
	commands.push(Command{std::move(commandName), std::move(commandArguments),
	                      hasRange, false, false, false});
}

OsmlStreamParser::State OsmlStreamParser::parseCommand(size_t start,
                                                       bool isAnnotation)
{
	// Parse the commandName as a first identifier
	Variant commandName = parseIdentifier(start, true);
	if (commandName.asString().empty()) {
		logger.error("Empty command name", reader);
		return State::NONE;
	}

	// Handle the special "begin" and "end" commands
	const auto commandNameComponents =
	    Utils::split(commandName.asString(), ':');
	const bool isBegin = commandNameComponents[0] == "begin";
	const bool isEnd = commandNameComponents[0] == "end";

	// Parse the begin or end command
	State res = State::COMMAND;
	if (isBegin || isEnd) {
		if (commandNameComponents.size() > 1) {
			logger.error(
			    "Special commands \"\\begin\" and \"\\end\" may not contain a "
			    "namespace separator \":\"",
			    commandName);
		}
		if (isBegin) {
			res = parseBeginCommand();
		} else if (isEnd) {
			res = parseEndCommand();
		}
	} else {
		// Check whether the next character is a '#', indicating the start of
		// the command name
		Variant commandArgName;
		start = reader.getOffset();
		if (reader.expect('#')) {
			commandArgName = parseIdentifier(start);
			if (commandArgName.asString().empty()) {
				logger.error("Expected identifier after \"#\"", commandArgName);
			}
		}

		// Parse the arugments
		Variant commandArguments =
		    parseCommandArguments(std::move(commandArgName));

		// Push the command onto the command stack
		pushCommand(std::move(commandName), std::move(commandArguments), false);
	}

	// Check whether a ">" character is the next character that is to be read.
	// In that case the current command could be an annotation end command!
	char c;
	if (reader.fetch(c) && c == '>') {
		// Ignore the character after a begin or end command
		if (isBegin || isEnd) {
			logger.warning(
			    "Ignoring annotation end character \">\" after special "
			    "commands \"begin\" or \"end\". Write \"\\>\" to end a "
			    "\"begin\"/\"end\" enclosed annotation.",
			    reader);
			return res;
		}

		// If this should be an annoation, ignore the character
		if (isAnnotation) {
			logger.warning(
			    "Ignoring annotation end character \">\" after annotation "
			    "start command. Write \"\\>\" to end the annotation.",
			    reader);
		} else {
			// Make sure no arguments apart from the "name" argument are given
			// to an annotation end
			Variant::mapType &map = commands.top().arguments.asMap();
			if (!map.empty()) {
				if (map.count("name") == 0 || map.size() > 1U) {
					logger.error(
					    "An annotation end command may not have any arguments "
					    "other than \"name\"");
					return res;
				}
			}

			// If we got here, this is a valid ANNOTATION_END command, issue it
			reader.peek(c);
			reader.consumePeek();
			return State::ANNOTATION_END;
		}
	}

	// If we're starting an annotation, return the command as annotation start
	// instead of command
	if (isAnnotation && res == State::COMMAND) {
		return State::ANNOTATION_START;
	}
	return res;
}

void OsmlStreamParser::parseBlockComment()
{
	Token token;
	size_t depth = 1;
	while (tokenizer.read(reader, token, data)) {
		// Throw the comment data away
		data.clear();

		if (token.id == OsmlTokens.BlockCommentEnd) {
			depth--;
			if (depth == 0) {
				return;
			}
		}
		if (token.id == OsmlTokens.BlockCommentStart) {
			depth++;
		}
	}

	// Issue an error if the file ends while we are in a block comment
	logger.error("File ended while being in a block comment", reader);
}

void OsmlStreamParser::parseLineComment()
{
	char c;
	while (reader.read(c)) {
		if (c == '\n') {
			return;
		}
	}
}

bool OsmlStreamParser::checkIssueData()
{
	if (!data.empty()) {
		location = data.getLocation();
		reader.resetPeek();
		return true;
	}
	return false;
}

bool OsmlStreamParser::checkIssueFieldStart()
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

bool OsmlStreamParser::closeField()
{
	// Try to end an open field of the current command -- if the current command
	// is not inside an open field, end this command and try to close the next
	// one
	for (int i = 0; i < 2 && commands.size() > 1; i++) {
		Command &cmd = commands.top();
		if (!cmd.inRangeField) {
			if (cmd.inField) {
				cmd.inField = false;
				if (cmd.inDefaultField) {
					commands.pop();
				}
				return true;
			}
			commands.pop();
		} else {
			return false;
		}
	}
	return false;
}

OsmlStreamParser::State OsmlStreamParser::parse()
{
	// Reset the data handler
	data.clear();

	// Read tokens until the outer loop should be left
	Token token;
	while (tokenizer.peek(reader, token, data)) {
		const TokenId type = token.id;

		// Special handling for Backslash and Text
		if (type == OsmlTokens.Backslash ||
		    type == OsmlTokens.AnnotationStart) {
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
				if (checkIssueData()) {
					return State::DATA;
				}

				// Parse the actual command
				State res = parseCommand(token.location.getStart(),
				                         type == OsmlTokens.AnnotationStart);
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

			// If this was an annotation start token, add the parsed < to the
			// output
			if (type == OsmlTokens.AnnotationStart) {
				data.append('<', token.location.getStart(),
				            token.location.getStart() + 1);
			}

			data.append(c, token.location.getStart(), reader.getPeekOffset());
			reader.consumePeek();
			continue;
		} else if (type == Tokens::Data) {
			// Check whether FIELD_START has to be issued before appending text
			if (checkIssueFieldStart()) {
				location = token.location;
				return State::FIELD_START;
			}
			reader.consumePeek();
			continue;
		}

		// A non-text token was reached, make sure all pending data commands
		// have been issued
		if (checkIssueData()) {
			return State::DATA;
		}

		// We will handle the token now, consume the peeked characters
		reader.consumePeek();

		// Update the location to the current token location
		location = token.location;

		if (token.id == OsmlTokens.LineComment) {
			parseLineComment();
		} else if (token.id == OsmlTokens.BlockCommentStart) {
			parseBlockComment();
		} else if (token.id == OsmlTokens.FieldStart) {
			Command &cmd = commands.top();
			if (!cmd.inField) {
				cmd.inField = true;
			}
			return State::FIELD_START;
/*			logger.error(
			    "Got field start token \"{\", but no command for which to "
			    "start the field. Write \"\\{\" to insert this sequence as "
			    "text.",
			    token);*/
		} else if (token.id == OsmlTokens.FieldEnd) {
			closeField();
			return State::FIELD_END;
/*			if (closeField()) {
				return State::FIELD_END;
			}
			logger.error(
			    "Got field end token \"}\", but there is no field to end. "
			    "Write \"\\}\" to insert this sequence as text.",
			    token);*/
		} else if (token.id == OsmlTokens.DefaultFieldStart) {
			// Try to start a default field the first time the token is reached
			Command &topCmd = commands.top();
			if (!topCmd.inField) {
				topCmd.inField = true;
				topCmd.inDefaultField = true;
			}
			return State::FIELD_START;
/*			logger.error(
			    "Got default field start token \"{!\", but no command for "
			    "which to start the field. Write \"\\{!\" to insert this "
			    "sequence as text",
			    token);*/
		} else if (token.id == OsmlTokens.AnnotationEnd) {
			// We got a single annotation end token "\>" -- simply issue the
			// ANNOTATION_END event
			Variant annotationName = Variant::fromString("");
			annotationName.setLocation(token.location);
			pushCommand(annotationName, Variant::mapType{}, false);
			return State::ANNOTATION_END;
		} else {
			logger.error("Unexpected token \"" + token.content + "\"", token);
		}
	}

	// Issue available data
	if (checkIssueData()) {
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

Variant OsmlStreamParser::getText(WhitespaceMode mode)
{
	TokenizedData dataFork = data;
	Variant text = dataFork.text(mode);
	location = text.getLocation();
	return text;
}

const Variant &OsmlStreamParser::getCommandName() const
{
	return commands.top().name;
}

const Variant &OsmlStreamParser::getCommandArguments() const
{
	return commands.top().arguments;
}

bool OsmlStreamParser::inDefaultField() const
{
	return commands.top().inRangeField || commands.top().inDefaultField;
}
}
