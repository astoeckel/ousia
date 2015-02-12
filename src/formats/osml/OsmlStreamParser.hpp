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
 * @file OsmlStreamParser.hpp
 *
 * Provides classes for low-level classes for reading the TeX-esque osml
 * format. The class provided here does not build any model objects and does not
 * implement the Parser interface.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_OSML_STREAM_PARSER_HPP_
#define _OUSIA_OSML_STREAM_PARSER_HPP_

#include <stack>

#include <core/common/Variant.hpp>
#include <core/parser/utils/Tokenizer.hpp>

namespace ousia {

// Forward declarations
class CharReader;
class Logger;
class DataHandler;

/**
 * The OsmlStreamParser class provides a low-level reader for the TeX-esque osml
 * format. The parser is constructed around a "parse" function, which reads data
 * from the underlying CharReader until a new state is reached and indicates
 * this state in a return value. The calling code then has to pull corresponding
 * data from the stream reader. The reader makes sure the incommind file is
 * syntactically valid and tries to recorver from most errors. If an error is
 * irrecoverable (this is the case for errors with wrong nesting of commands or
 * fields, as this would lead to too many consecutive errors) a
 * LoggableException is thrown.
 */
class OsmlStreamParser {
public:
	/**
	 * Enum used to indicate which state the OsmlStreamParser class is in
	 * after calling the "parse" function.
	 */
	enum class State {
		/**
	     * State returned if a fully featured command has been read. A command
	     * consists of the command name and its arguments (which optionally
	     * includes the name).
	     */
		COMMAND,

		/**
	     * State returned if data is given. The reader must decide which field
	     * or command this should be routed to. Trailing or leading whitespace
	     * has been removed. Only called if the data is non-empty.
	     */
		DATA,

		/**
	     * A user-defined entity has been found. The entity sequence is stored
	     * in the command name.
	     */
		ENTITY,

		/**
	     * State returned if an annotation was started. An annotation consists
	     * of the command name and its arguments (which optionally include the
	     * name).
	     */
		ANNOTATION_START,

		/**
	     * State returned if an annotation ends. The reader indicates which
	     * annotation ends.
	     */
		ANNOTATION_END,

		/**
	     * State returned if a new field started. The reader assures that the
	     * current field ends before a new field is started and that the field
	     * is not started if data has been given outside of a field. The
	     * field number is set to the current field index.
	     */
		FIELD_START,

		/**
	     * State returned if the current field ends. The reader assures that a
	     * field was actually open.
	     */
		FIELD_END,

		/**
	     * The end of the stream has been reached.
	     */
		END,

		/**
	     * Returned from internal functions if nothing should be done.
	     */
		NONE,

		/**
	     * Returned from internal function to indicate irrecoverable errors.
	     */
		ERROR
	};

	/**
	 * Entry used for the command stack.
	 */
	struct Command {
		/**
		 * Name and location of the current command.
		 */
		Variant name;

		/**
		 * Arguments that were passed to the command.
		 */
		Variant arguments;

		/**
		 * Set to true if this is a command with clear begin and end.
		 */
		bool hasRange;

		/**
		 * Set to true if we are currently inside a field of this command.
		 */
		bool inField;

		/**
		 * Set to true if we are currently in the range field of the command
		 * (implies inField being set to true).
		 */
		bool inRangeField;

		/**
		 * Set to true if we are currently in a field that has been especially
		 * marked as default field (using the "|") syntax.
		 */
		bool inDefaultField;

		/**
		 * Default constructor.
		 */
		Command()
		    : hasRange(false),
		      inField(false),
		      inRangeField(false),
		      inDefaultField()
		{
		}

		/**
		 * Constructor of the Command class.
		 *
		 * @param name is a string variant with name and location of the
		 * command.
		 * @param arguments is a map variant with the arguments given to the
		 * command.
		 * @param hasRange should be set to true if this is a command with
		 * explicit range.
		 * @param inField is set to true if we currently are inside a field
		 * of this command.
		 * @param inRangeField is set to true if we currently are inside the
		 * outer field of a ranged command.
		 * @param inDefaultField is set to true if we currently are in a
		 * specially marked default field.
		 */
		Command(Variant name, Variant arguments, bool hasRange,
		        bool inField, bool inRangeField, bool inDefaultField)
		    : name(std::move(name)),
		      arguments(std::move(arguments)),
		      hasRange(hasRange),
		      inField(inField),
		      inRangeField(inRangeField),
		      inDefaultField(inDefaultField)
		{
		}
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
	 * Variant containing the data that has been read (always is a string,
	 * contains the exact location of the data in the source file).
	 */
	Variant data;

	/**
	 * Contains the location of the last token.
	 */
	SourceLocation location;

	/**
	 * Contains the field index of the current command.
	 */
	size_t fieldIdx;

	/**
	 * Function used internall to parse an identifier.
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
	 */
	State parseBeginCommand();

	/**
	 * Function used internally to handle the special "\end" command.
	 */
	State parseEndCommand();

	/**
	 * Pushes the parsed command onto the command stack.
	 */
	void pushCommand(Variant commandName, Variant commandArguments,
	                 bool hasRange);

	/**
	 * Parses the command arguments.
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
	 * Checks whether there is any data pending to be issued, if yes, issues it.
	 *
	 * @param handler is the data handler that contains the data that may be
	 * returned to the user.
	 * @return true if there was any data and DATA should be returned by the
	 * parse function, false otherwise.
	 */
	bool checkIssueData(DataHandler &handler);

	/**
	 * Called before any data is appended to the internal data handler. Checks
	 * whether a new field should be started or implicitly ended.
	 *
	 * @return true if FIELD_START should be returned by the parse function.
	 */
	bool checkIssueFieldStart();

	/**
	 * Closes a currently open field. Note that the command will be removed from
	 * the internal command stack if the field that is being closed is a
	 * field marked as default field.
	 *
	 * @return true if the field could be closed, false if there was no field
	 * to close.
	 */
	bool closeField();

public:
	/**
	 * Constructor of the OsmlStreamParser class. Attaches the new
	 * OsmlStreamParser to the given CharReader and Logger instances.
	 *
	 * @param reader is the reader instance from which incomming characters
	 * should be read.
	 * @param logger is the logger instance to which errors should be written.
	 */
	OsmlStreamParser(CharReader &reader, Logger &logger);

	/**
	 * Continues parsing. Returns one of the states defined in the State enum.
	 * Callers should stop once the State::END state is reached. Use the getter
	 * functions to get more information about the current state, such as the
	 * command name or the data or the current field index.
	 *
	 * @return the new state the parser has reached.
	 */
	State parse();

	/**
	 * Returns a reference at the internally stored data. Only valid if
	 * State::DATA was returned by the "parse" function.
	 *
	 * @return a reference at a variant containing the data parsed by the
	 * "parse" function.
	 */
	const Variant &getData() const { return data; }

	/**
	 * Returns a reference at the internally stored command name. Only valid if
	 * State::COMMAND was returned by the "parse" function.
	 *
	 * @return a reference at a variant containing name and location of the
	 * parsed command.
	 */
	const Variant &getCommandName() const;

	/**
	 * Returns a reference at the internally stored command name. Only valid if
	 * State::COMMAND was returned by the "parse" function.
	 *
	 * @return a reference at a variant containing arguments given to the
	 * command.
	 */
	const Variant &getCommandArguments() const;

	/**
	 * Returns true if the current field is the "default" field. This is true if
	 * the parser either is in the outer range of a range command or inside a
	 * field that has been especially marked as "default" field (using the "|"
	 * syntax).
	 */
	bool inDefaultField() const;

	/**
	 * Returns a reference at the char reader.
	 *
	 * @return the last internal token location.
	 */
	const SourceLocation &getLocation() const { return location; }
};
}

#endif /* _OUSIA_OSML_STREAM_PARSER_HPP_ */

