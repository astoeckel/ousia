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

#include <cassert>
#include <stack>
#include <vector>

#include <core/common/CharReader.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Utils.hpp>
#include <core/common/Variant.hpp>
#include <core/common/VariantReader.hpp>

#include <core/parser/utils/Tokenizer.hpp>
#include <core/parser/utils/TokenizedData.hpp>

#include "OsmlStreamParser.hpp"

namespace ousia {

namespace {
/**
 * Osml format default tokenizer. Registers the primary tokens in its
 * constructor. A single, static instance of this class is created as
 * "OsmlTokens", which is copied to the Tokenizer instance of
 * OsmlStreamParserImpl.
 */
class OsmlFormatTokens : public Tokenizer {
public:
	TokenId Backslash;
	TokenId LineComment;
	TokenId BlockCommentStart;
	TokenId BlockCommentEnd;
	TokenId FieldStart;
	TokenId FieldEnd;
	TokenId DefaultFieldStart;
	TokenId AnnotationStart;
	TokenId AnnotationEnd;

	/**
	 * Registers the plain format tokens in the internal tokenizer.
	 */
	OsmlFormatTokens()
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

/**
 * Instance of OsmlFormatTokens used to initialize the internal tokenizer
 * instance of OsmlStreamParserImpl.
 */
static const OsmlFormatTokens OsmlTokens;

/**
 * Structure representing a field.
 */
struct Field {
	/**
	 * Specifies whether this field was marked as default field.
	 */
	bool defaultField;

	/**
	 * Location at which the field was started.
	 */
	SourceLocation location;

	/**
	 * Constructor of the Field structure, initializes all member variables with
	 * the given values.
	 *
	 * @param defaultField is a flag specifying whether this field is a default
	 * field.
	 * @param location specifies the location at which the field was started.
	 */
	Field(bool defaultField = false,
	      const SourceLocation &location = SourceLocation{})
	    : defaultField(defaultField), location(location)
	{
	}
};

/**
 * Entry used for the command stack.
 */
class Command {
private:
	/**
	 * Name and location of the current command.
	 */
	Variant name;

	/**
	 * Arguments that were passed to the command.
	 */
	Variant arguments;

	/**
	 * Vector used as stack for holding the number of opening/closing braces
	 * and the corresponding "isDefaultField" flag.
	 */
	std::vector<Field> fields;

	/**
	 * Set to true if this is a command with clear begin and end.
	 */
	bool hasRange;

public:
	/**
	 * Default constructor, marks this command as normal, non-range command.
	 */
	Command() : hasRange(false) {}

	/**
	 * Constructor of the Command class.
	 *
	 * @param name is a string variant with name and location of the
	 * command.
	 * @param arguments is a map variant with the arguments given to the
	 * command.
	 * @param hasRange should be set to true if this is a command with
	 * explicit range.
	 */
	Command(Variant name, Variant arguments, bool hasRange)
	    : name(std::move(name)),
	      arguments(std::move(arguments)),
	      hasRange(hasRange)
	{
	}

	/**
	 * Returns a reference at the variant representing name and location of the
	 * command.
	 *
	 * @return a variant containing name and location of the command.
	 */
	const Variant &getName() const { return name; }

	/**
	 * Returns a reference at the variant containing name, value and location of
	 * the arguments.
	 *
	 * @return the arguments stored for the command.
	 */
	const Variant &getArguments() const { return arguments; }

	/**
	 * Returns a reference at the internal field list. This list should be used
	 * for printing error messages when fields are still open although the outer
	 * range field closes.
	 *
	 * @return a const reference at the internal field vector.
	 */
	const std::vector<Field> &getFields() const { return fields; }

	/**
	 * Returns true if this command is currently in a default field.
	 *
	 * @return true if the current field on the field stack was explicitly
	 * marked as default field. If the field stack is empty, true is returned
	 * if this is a range command.
	 */
	bool inDefaultField() const
	{
		return (!fields.empty() && fields.back().defaultField) ||
		       (fields.empty() && hasRange);
	}

	/**
	 * Returns true if this command currently is in any field.
	 *
	 * @return true if a field is on the stack or this is a range commands.
	 * Range commands always are in a field.
	 */
	bool inField() const { return !fields.empty() || hasRange; }

	/**
	 * Returns true if this command currently is in a range field.
	 *
	 * @return true if the command has a range and no other ranges are on the
	 * stack.
	 */
	bool inRangeField() const { return fields.empty() && hasRange; }

	/**
	 * Returns true if this command currently is in a non-range field.
	 *
	 * @return true if the command is in a field, but the field is not the field
	 * constructed by the "range"
	 */
	bool inNonRangeField() const { return !fields.empty(); }

	/**
	 * Pushes another field onto the field stack of this command.
	 *
	 * @param defaultField if true, explicitly marks this field as default
	 * field.
	 * @param location is the source location at which the field was started.
	 * Used for error messages in which the user is notified about an error with
	 * too few closing fields.
	 */
	void pushField(bool defaultField = false,
	               const SourceLocation &location = SourceLocation{})
	{
		fields.emplace_back(defaultField, location);
	}

	/**
	 * Removes another field from the field stack of this command, returns true
	 * if the operation was successful.
	 *
	 * @return true if there was a field to pop on the stack, false otherwise.
	 */
	bool popField()
	{
		if (!fields.empty()) {
			fields.pop_back();
			return true;
		}
		return false;
	}
};
}

/* Class OsmlStreamParserImpl */

/**
 * Internal implementation of OsmlStreamParser.
 */
class OsmlStreamParserImpl {
public:
	/**
	 * State enum compatible with OsmlStreamParserState but extended by two more
	 * entries (END and NONE).
	 */
	enum class State : uint8_t {
		COMMAND_START = 0,
		RANGE_END = 1,
		FIELD_START = 2,
		FIELD_END = 3,
		ANNOTATION_START = 4,
		ANNOTATION_END = 5,
		DATA = 6,
		END = 7,
		RECOVERABLE_ERROR = 8,
		IRRECOVERABLE_ERROR = 9
	};

private:
	/**
	 * Reference to the CharReader instance from which the incomming bytes are
	 * read.
	 */
	CharReader &reader;

	/**
	 * Reference at the logger instance to which all error messages are sent.
	 */
	Logger &logger;

	/**
	 * Tokenizer instance used to read individual tokens from the text.
	 */
	Tokenizer tokenizer;

	/**
	 * Stack containing the current commands.
	 */
	std::stack<Command> commands;

	/**
	 * Variant containing the tokenized data that was returned from the
	 * tokenizer as data.
	 */
	TokenizedData data;

	/**
	 * Variable containing the current location of the parser.
	 */
	SourceLocation location;

	/**
	 * Function used internally to parse an identifier.
	 *
	 * @param start is the start byte offset of the identifier (including the
	 * backslash).
	 * @param allowNSSep should be set to true if the namespace separator is
	 * allowed in the identifier name. Issues error if the namespace separator
	 * is placed incorrectly.
	 */
	Variant parseIdentifier(size_t start, bool allowNSSep = false);

	/**
	 * Function used internally to handle the special "\begin" command.
	 *
	 * @return an internal State specifying whether an error occured (return
	 * values State::REOVERABLE_ERROR or State::IRRECOVERABLE_ERROR) or a
	 * command was actually started (return value State::COMMAND_START).
	 */
	State parseBeginCommand();

	/**
	 * Function used internally to handle the special "\end" command.
	 *
	 * @return an internal State specifying whether an error occured (return
	 * values State::REOVERABLE_ERROR or State::IRRECOVERABLE_ERROR) or a
	 * command was actually ended (return value State::RANGE_END).
	 */
	State parseEndCommand();

	/**
	 * Parses the command arguments. Handles errors if the name of the command
	 * was given using the hash notation and as a name field.
	 *
	 * @param commandArgName is the name argument that was given using the hash
	 * notation.
	 * @return a map variant containing the arguments.
	 */
	Variant parseCommandArguments(Variant commandArgName);

	/**
	 * Function used internally to parse a command.
	 *
	 * @param start is the start byte offset of the command (including the
	 * backslash)
	 * @param isAnnotation if true, the command is not returned as command, but
	 * as annotation start.
	 * @return true if a command was actuall parsed, false otherwise.
	 */
	State parseCommand(size_t start, bool isAnnotation);

	/**
	 * Function used internally to parse a block comment.
	 */
	void parseBlockComment();

	/**
	 * Function used internally to parse a generic comment.
	 */
	void parseLineComment();

	/**
	 * Pushes the parsed command onto the command stack.
	 */
	void pushCommand(Variant commandName, Variant commandArguments,
	                 bool hasRange);

	/**
	 * Checks whether there is any data pending to be issued, if yes, resets the
	 * currently peeked characters and returns true.
	 *
	 * @return true if there was any data and DATA should be returned by the
	 * parse function, false otherwise.
	 */
	bool checkIssueData();

	/**
	 * Returns a reference at the current command at the top of the command
	 * stack.
	 *
	 * @return a reference at the top command in the command stack.
	 */
	Command &cmd() { return commands.top(); }

	/**
	 * Returns a reference at the current command at the top of the command
	 * stack.
	 *
	 * @return a reference at the top command in the command stack.
	 */
	const Command &cmd() const { return commands.top(); }

public:
	/**
	 * Constructor of the OsmlStreamParserImpl class. Attaches the new
	 * OsmlStreamParserImpl to the given CharReader and Logger instances.
	 *
	 * @param reader is the reader instance from which incomming characters
	 * should be read.
	 * @param logger is the logger instance to which errors should be written.
	 */
	OsmlStreamParserImpl(CharReader &reader, Logger &logger);

	State parse();

	TokenId registerToken(const std::string &token);
	void unregisterToken(TokenId token);

	const TokenizedData &getData() const { return data; }
	const Variant &getCommandName() const { return cmd().getName(); }
	const Variant &getCommandArguments() const { return cmd().getArguments(); }
	const SourceLocation &getLocation() const { return location; }
	bool inRangeCommand() const { return cmd().inRangeField(); };
	bool inDefaultField() const { return cmd().inDefaultField(); }
};

/* Class OsmlStreamParserImpl */

OsmlStreamParserImpl::OsmlStreamParserImpl(CharReader &reader, Logger &logger)
    : reader(reader), logger(logger), tokenizer(OsmlTokens)
{
	commands.emplace("", Variant::mapType{}, true);
}

Variant OsmlStreamParserImpl::parseIdentifier(size_t start, bool allowNSSep)
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

OsmlStreamParserImpl::State OsmlStreamParserImpl::parseBeginCommand()
{
	// Expect a '{' after the command
	reader.consumeWhitespace();
	if (!reader.expect('{')) {
		logger.error("Expected \"{\" after \\begin", reader);
		return State::RECOVERABLE_ERROR;
	}

	// Parse the name of the command that should be opened
	Variant commandName = parseIdentifier(reader.getOffset(), true);
	if (commandName.asString().empty()) {
		logger.error("Expected identifier", commandName);
		return State::IRRECOVERABLE_ERROR;
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
		return State::IRRECOVERABLE_ERROR;
	}

	// Parse the arguments
	Variant commandArguments = parseCommandArguments(std::move(commandArgName));

	// Push the command onto the command stack
	pushCommand(std::move(commandName), std::move(commandArguments), true);

	return State::COMMAND_START;
}

OsmlStreamParserImpl::State OsmlStreamParserImpl::parseEndCommand()
{
	// Expect a '{' after the command
	if (!reader.expect('{')) {
		logger.error("Expected \"{\" after \\end", reader);
		return State::RECOVERABLE_ERROR;
	}

	// Fetch the name of the command that should be ended here
	Variant name = parseIdentifier(reader.getOffset(), true);

	// Make sure the given command name is not empty
	if (name.asString().empty()) {
		logger.error("Expected identifier", name);
		return State::IRRECOVERABLE_ERROR;
	}

	// Make sure the command name is terminated with a '}'
	if (!reader.expect('}')) {
		logger.error("Expected \"}\"", reader);
		return State::IRRECOVERABLE_ERROR;
	}

	// Unroll the command stack up to the last range command, make sure we do
	// not intersect with any open field
	while (!cmd().inRangeField()) {
		if (cmd().inField()) {
			logger.error(std::string("\\end in open field of command \"") +
			                 cmd().getName().asString() + std::string("\""),
			             name);
			const std::vector<Field> &fields = cmd().getFields();
			for (const Field &field : fields) {
				logger.note(std::string("Still open field started here: "),
				            field.location);
			}
			return State::IRRECOVERABLE_ERROR;
		}
		commands.pop();
	}

	// Special error message if the top-level command is reached
	if (commands.size() == 1) {
		logger.error(std::string("Cannot end command \"") + name.asString() +
		                 std::string("\" here, no command open"),
		             name);
		return State::IRRECOVERABLE_ERROR;
	}

	// Inform the user about command mismatches, copy the current command
	// descriptor before popping it from the stack
	if (getCommandName().asString() != name.asString()) {
		logger.error(std::string("Trying to end command \"") + name.asString() +
		                 std::string("\", but open command is \"") +
		                 getCommandName().asString() + std::string("\""),
		             name);
		logger.note("Open command started here:", getCommandName());
		return State::IRRECOVERABLE_ERROR;
	}

	// End the current command
	location = name.getLocation();
	commands.pop();
	return State::RANGE_END;
}

Variant OsmlStreamParserImpl::parseCommandArguments(Variant commandArgName)
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

OsmlStreamParserImpl::State OsmlStreamParserImpl::parseCommand(
    size_t start, bool isAnnotation)
{
	// Parse the commandName as a first identifier
	Variant commandName = parseIdentifier(start, true);
	if (commandName.asString().empty()) {
		logger.error("Empty command name", reader);
		return State::RECOVERABLE_ERROR;
	}

	// Handle the special "begin" and "end" commands
	const auto commandNameComponents =
	    Utils::split(commandName.asString(), ':');
	const bool isBegin = commandNameComponents[0] == "begin";
	const bool isEnd = commandNameComponents[0] == "end";

	// Parse the begin or end command
	State res = State::COMMAND_START;
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
			const Variant::mapType &map = getCommandArguments().asMap();
			if (!map.empty()) {
				if (map.count("name") == 0 || map.size() > 1U) {
					logger.error(
					    "An annotation end command may not have any arguments "
					    "other than \"name\"",
					    reader);
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
	if (isAnnotation && res == State::COMMAND_START) {
		return State::ANNOTATION_START;
	}
	return res;
}

void OsmlStreamParserImpl::parseBlockComment()
{
	Token token;
	TokenizedData commentData;
	size_t depth = 1;
	while (tokenizer.read(reader, token, commentData)) {
		// Throw the comment data away
		commentData.clear();

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

void OsmlStreamParserImpl::parseLineComment()
{
	char c;
	while (reader.read(c)) {
		if (c == '\n') {
			return;
		}
	}
}

void OsmlStreamParserImpl::pushCommand(Variant commandName,
                                       Variant commandArguments, bool hasRange)
{
	// Store the location of the command
	location = commandName.getLocation();

	// Place the command on the command stack, remove the last commands if we're
	// not currently inside a field of these commands
	while (!cmd().inField()) {
		commands.pop();
	}

	// Push the new command onto the command stack
	commands.emplace(std::move(commandName), std::move(commandArguments),
	                 hasRange);
}

bool OsmlStreamParserImpl::checkIssueData()
{
	if (!data.empty()) {
		location = data.getLocation();
		reader.resetPeek();
		return true;
	}
	return false;
}

OsmlStreamParserImpl::State OsmlStreamParserImpl::parse()
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
					case State::IRRECOVERABLE_ERROR:
						throw LoggableException(
						    "Last error was irrecoverable, ending parsing "
						    "process");
					case State::RECOVERABLE_ERROR:
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
			SourceOffset charStart = token.location.getStart();
			SourceOffset charEnd = reader.getPeekOffset();
			if (type == OsmlTokens.AnnotationStart) {
				data.append('<', charStart, charStart + 1);
				charStart = charStart + 1;
			}

			// Append the character to the output data, mark it as protected
			data.append(c, charStart, charEnd, true);
			reader.consumePeek();
			continue;
		} else if (type == Tokens::Data) {
			reader.consumePeek();
			continue;
		} else if (type == OsmlTokens.LineComment) {
			reader.consumePeek();
			parseLineComment();
			continue;
		} else if (type == OsmlTokens.BlockCommentStart) {
			reader.consumePeek();
			parseBlockComment();
			continue;
		}

		// A non-text token was reached, make sure all pending data commands
		// have been issued
		if (checkIssueData()) {
			return State::DATA;
		}

		// We will handle the token now, consume the peeked characters
		reader.consumePeek();

		// Synchronize the location with the current token location
		location = token.location;

		if (token.id == OsmlTokens.FieldStart) {
			cmd().pushField(false, token.location);
			return State::FIELD_START;
		} else if (token.id == OsmlTokens.FieldEnd) {
			// Remove all commands from the list that currently are not in any
			// field
			while (!cmd().inField()) {
				commands.pop();
			}

			// If the remaining command is not in a range field, remove this
			// command
			if (cmd().inNonRangeField()) {
				cmd().popField();
				return State::FIELD_END;
			}
			logger.error(
			    "Got field end token \"}\", but there is no field to end.",
			    token);
		} else if (token.id == OsmlTokens.DefaultFieldStart) {
			cmd().pushField(true, token.location);
			return State::FIELD_START;
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
	while (true) {
		bool topLevelCommand = commands.size() == 1U;
		if (cmd().inField()) {
			// If the stream ended with an open range field, issue information
			// about the range field
			if (cmd().inRangeField() && !topLevelCommand) {
				// Inform about the still open command itself
				logger.error("Reached end of stream, but command \"" +
				                 getCommandName().asString() +
				                 "\" has not been ended",
				             getCommandName());
			} else {
				// Issue information about still open fields
				const std::vector<Field> &fields = cmd().getFields();
				if (!fields.empty()) {
					logger.error(
					    std::string(
					        "Reached end of stream, but field is still open."),
					    fields.back().location);
				}
			}
		}
		if (!topLevelCommand) {
			commands.pop();
		} else {
			break;
		}
	}

	location = SourceLocation{reader.getSourceId(), reader.getOffset()};
	return State::END;
}

TokenId OsmlStreamParserImpl::registerToken(const std::string &token)
{
	return tokenizer.registerToken(token, false);
}

void OsmlStreamParserImpl::unregisterToken(TokenId token)
{
	assert(tokenizer.unregisterToken(token));
}

/* Class OsmlStreamParser */

OsmlStreamParser::OsmlStreamParser(CharReader &reader, Logger &logger)
    : impl(new OsmlStreamParserImpl(reader, logger))
{
}

OsmlStreamParser::~OsmlStreamParser()
{
	// Stub needed because OsmlStreamParserImpl is incomplete in header
}

OsmlStreamParser::State OsmlStreamParser::parse()
{
	return static_cast<State>(impl->parse());
}

const TokenizedData &OsmlStreamParser::getData() const
{
	return impl->getData();
}

const Variant &OsmlStreamParser::getCommandName() const
{
	return impl->getCommandName();
}

const Variant &OsmlStreamParser::getCommandArguments() const
{
	return impl->getCommandArguments();
}

const SourceLocation &OsmlStreamParser::getLocation() const
{
	return impl->getLocation();
}

bool OsmlStreamParser::inDefaultField() const { return impl->inDefaultField(); }

bool OsmlStreamParser::inRangeCommand() const { return impl->inRangeCommand(); }

TokenId OsmlStreamParser::registerToken(const std::string &token)
{
	return impl->registerToken(token);
}

void OsmlStreamParser::unregisterToken(TokenId token)
{
	impl->unregisterToken(token);
}
}
