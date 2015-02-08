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

#ifndef _OUSIA_PLAIN_FORMAT_STREAM_READER_HPP_
#define _OUSIA_PLAIN_FORMAT_STREAM_READER_HPP_

/**
 * @file PlainFormatStreamReader.hpp
 *
 * Provides classes for low-level classes for reading the plain TeX-esque
 * format. The class provided here do not build any model objects and does not
 * implement the Parser interfaces.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#include <core/common/Variant.hpp>

#include "DynamicTokenizer.hpp"

namespace ousia {

// Forward declarations
class CharReader;
class Logger;

/**
 * The PlainFormatStreamReader class provides a low-level reader for the plain
 * TeX-esque format. The parser is constructed around a "parse" function, which
 * reads data from the underlying CharReader until a new state is reached and
 * indicates this state in a return value. The calling code then has to pull
 * corresponding data from the stream reader. The reader already handles some
 * invalid cases, but recovers from most errors and happily continues parsing.
 */
class PlainFormatStreamReader {
public:
	/**
	 * Enum used to indicate which state the PlainFormatStreamReader class is in
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
		 * State returned if a linebreak has been reached (outside of comments).
		 */
		LINEBREAK,

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
		END
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
	DynamicTokenizer tokenizer;

	/**
	 * Variant containing the current command name (always is a string variant,
	 * but additionally contains the correct locatino of the name).
	 */
	Variant commandName;

	/**
	 * Variant containing the command arguments (always is a map or array
	 * variant, but additionally contains the source location of the arguments).
	 */
	Variant commandArguments;

	/**
	 * Variant containing the data that has been read (always is a string,
	 * contains the exact location of the data in the source file).
	 */
	Variant data;

	/**
	 * Id of the backslash token.
	 */
	TokenTypeId tokenBackslash;

	/**
	 * Id of the linebreak token.
	 */
	TokenTypeId tokenLinebreak;

	/**
	 * Id of the line comment token.
	 */
	TokenTypeId tokenLineComment;

	/**
	 * Id of the block comment start token.
	 */
	TokenTypeId tokenBlockCommentStart;

	/**
	 * If of the block comment end token.
	 */
	TokenTypeId tokenBlockCommentEnd;

	/**
	 * Contains the field index of the current command.
	 */
	size_t fieldIdx;

	/**
	 * Function used internall to parse an identifier.
	 *
	 * @param start is the start byte offset of the identifier (including the
	 * backslash).
	 */
	Variant parseIdentifier(size_t start);

	/**
	 * Function used internally to parse a command.
	 *
	 * @param start is the start byte offset of the command (including the
	 * backslash).
	 */
	void parseCommand(size_t start);

	/**
	 * Function used internally to parse a block comment.
	 */
	void parseBlockComment();

	/**
	 * Function used internally to parse a generic comment.
	 */
	void parseLineComment();

public:
	/**
	 * Constructor of the PlainFormatStreamReader class. Attaches the new
	 * PlainFormatStreamReader to the given CharReader and Logger instances.
	 *
	 * @param reader is the reader instance from which incomming characters
	 * should be read.
	 * @param logger is the logger instance to which errors should be written.
	 */
	PlainFormatStreamReader(CharReader &reader, Logger &logger);

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
	const Variant& getData() {return data;}

	/**
	 * Returns a reference at the internally stored command name. Only valid if
	 * State::COMMAND was returned by the "parse" function.
	 *
	 * @return a reference at a variant containing name and location of the
	 * parsed command.
	 */
	const Variant& getCommandName() {return commandName;}

	/**
	 * Returns a reference at the internally stored command name. Only valid if
	 * State::COMMAND was returned by the "parse" function.
	 *
	 * @return a reference at a variant containing arguments given to the
	 * command.
	 */
	const Variant& getCommandArguments() {return commandArguments;}
};
}

#endif /* _OUSIA_PLAIN_FORMAT_STREAM_READER_HPP_ */

