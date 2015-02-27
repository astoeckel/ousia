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

#include <cstdint>
#include <memory>

namespace ousia {

// Forward declarations
class CharReader;
class Logger;
class OsmlStreamParserImpl;
class TokenizedData;
class Variant;

/**
 * The OsmlStreamParser class provides a low-level reader for the TeX-esque osml
 * format. The parser is constructed around a "parse" function, which reads data
 * from the underlying CharReader until a new state is reached and indicates
 * this state in a return value. The calling code then has to pull corresponding
 * data from the stream reader. The reader makes sure the incomming stream is
 * syntactically valid and tries to recorver from most errors. If an error is
 * irrecoverable (this is the case for errors with wrong nesting of commands or
 * fields, as this would lead to too many consecutive errors) a
 * LoggableException is thrown. The OsmlStreamParser can be compared to a SAX
 * parser for XML.
 */
class OsmlStreamParser {
public:
	/**
	 * Enum used to indicate which state the OsmlStreamParser class is in
	 * after calling the "parse" function.
	 */
	enum class State : uint8_t {
		/**
	     * State returned if the start of a command has been read. Use the
	     * getCommandName(), getCommandArguments() and inRangeCommand()
	     * functions the retrieve more information about the command that was
	     * just started.
	     */
		COMMAND_START = 0,

		/**
	     * State returned if a range command has just ended. This state is not
	     * returned for non-range commands (as the actual end of a command is
	     * context dependant).
	     */
		COMMAND_END = 1,

		/**
	     * State returned if a new field started. The reader assures that the
	     * current field ends before a new field is started and that the field
	     * is not started if data has been given outside of a field. The
	     * field number is set to the current field index.
	     */
		FIELD_START = 2,

		/**
	     * State returned if the current field ends. The reader assures that a
	     * field was actually open.
	     */
		FIELD_END = 3,

		/**
	     * State returned if an annotation was started. An annotation consists
	     * of the command name and its arguments (which optionally include the
	     * name).
	     */
		ANNOTATION_START = 4,

		/**
	     * State returned if an annotation ends. The reader indicates which
	     * annotation ends.
	     */
		ANNOTATION_END = 5,

		/**
	     * State returned if data is given. The reader must decide which field
	     * or command this should be routed to. Trailing or leading whitespace
	     * has been removed. Only called if the data is non-empty.
	     */
		DATA = 6,

		/**
	     * The end of the stream has been reached.
	     */
		END = 7
	};

private:
	/**
	 * Pointer at the class containing the internal implementation (according
	 * to the PIMPL idiom).
	 */
	std::unique_ptr<OsmlStreamParserImpl> impl;

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
	 * Destructor of the OsmlStreamParser, needed to destroy the incomplete
	 * OsmlStreamParserImpl.
	 */
	~OsmlStreamParser();

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
	 * Returns a reference at the internally stored command name. Only valid if
	 * State::COMMAND_START, State::ANNOTATION_START or State::ANNOTATION_END
	 * was returned by the "parse" function.
	 *
	 * @return a reference at a variant containing name and location of the
	 * parsed command.
	 */
	const Variant &getCommandName() const;

	/**
	 * Returns a reference at the internally stored command name. Only valid if
	 * State::COMMAND_START, State::ANNOTATION_START or State::ANNOTATION_END
	 * was returned by the "parse" function.
	 *
	 * @return a reference at a variant containing arguments given to the
	 * command.
	 */
	const Variant &getCommandArguments() const;

	/**
	 * Returns a reference at the internally stored data. Only valid if
	 * State::DATA was returned by the "parse" function.
	 *
	 * @return a reference at a variant containing the data parsed by the
	 * "parse" function.
	 */
	const TokenizedData &getData() const;

	/**
	 * Returns the location of the current token.
	 */
	const SourceLocation &getLocation() const;

	/**
	 * Returns true if the currently started command is a range command, only
	 * valid if State::COMMAND_START was returned by the "parse" function.
	 *
	 * @return true if the command is started is a range command, false
	 * otherwise.
	 */
	bool inRangeCommand() const;

	/**
	 * Returns true if the current field is the "default" field. This is true if
	 * the parser either is in the outer range of a range command or inside a
	 * field that has been especially marked as "default" field (using the "{!"
	 * syntax). Only valid if State::FIELD_START was returned by the "parse"
	 * function.
	 *
	 * @return true if the current field was marked as default field (using the
	 * "{!" syntax).
	 */
	bool inDefaultField() const;
};
}

#endif /* _OUSIA_OSML_STREAM_PARSER_HPP_ */

