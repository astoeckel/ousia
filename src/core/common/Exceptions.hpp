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

/**
 * @file Exceptions.hpp
 *
 * Describes basic exception classes which are used throughout Ousía.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_EXCEPTIONS_HPP_
#define _OUSIA_EXCEPTIONS_HPP_

#include "TextCursor.hpp"

namespace ousia {

/**
 * Base exception class all other Ousía exceptions should derive from.
 */
class OusiaException : public std::exception {
private:
	/**
	 * Error message which will be printed by the runtime environment if the
	 * exception is not caught and handled in the code.
	 */
	const std::string formatedMessage;

public:
	/**
	 * Constructor of the OusiaException class.
	 *
	 * @param formatedMessage is a formated message that should be printed by
	 * the runtime environment if the exception is not caught.
	 */
	OusiaException(std::string formatedMessage)
	    : formatedMessage(std::move(formatedMessage))
	{
	}

	/**
	 * Virtual destructor.
	 */
	virtual ~OusiaException() {}

	/**
	 * Implementation of the std::exception what function and used to retrieve
	 * the error message that should be printed by the runtime environment.
	 *
	 * @return a reference to the formated message string given in the
	 * constructor.
	 */
	const char *what() const noexcept override
	{
		return formatedMessage.c_str();
	}
};

/**
 * Exception class which can be directly passed to a Logger instance and thus
 * makes it simple to handle non-recoverable errors in the code.
 */
class LoggableException : public OusiaException {
private:
	/**
	 * Function used internally to build the formated message that should be
	 * reported to the runtime environment.
	 */
	static std::string formatMessage(const std::string &msg,
	                                 const TextCursor::Position &pos,
	                                 const TextCursor::Context &ctx);

public:
	/**
	 * Reported error message.
	 */
	const std::string msg;

	/**
	 * Position in the document at which the exception occurred.
	 */
	const TextCursor::Position pos;

	/**
	 * Context in the document text in which the exception occurred.
	 */
	const TextCursor::Context ctx;

	/**
	 * Constructor of the LoggableException class.
	 *
	 * @param msg contains the error message.
	 * @param pos is the position at which the error occured.
	 * @param ctx describes the context in which the error occured.
	 */
	LoggableException(std::string msg,
	                  TextCursor::Position pos = TextCursor::Position{},
	                  TextCursor::Context ctx = TextCursor::Context{})
	    : OusiaException(formatMessage(msg, pos, ctx)),
	      msg(std::move(msg)),
	      pos(std::move(pos)),
	      ctx(std::move(ctx))
	{
	}

	/**
	 * Constructor of the LoggableException class.
	 *
	 * @param msg contains the error message.
	 * @param line is the line in the above file the message refers to.
	 * @param column is the column in the above file the message refers to.
	 * @param offs is the byte offset.
	 */
	LoggableException(std::string msg, TextCursor::PosType line,
	                  TextCursor::PosType column, size_t offs)
	    : LoggableException(msg, TextCursor::Position(line, column, offs))
	{
	}

	/**
	 * Constructor of LoggableException for arbitrary position objects.
	 *
	 * @param msg is the actual log message.
	 * @param pos is a reference to a variable with position and context data.
	 */
	template <class PosType>
	LoggableException(std::string msg, PosType &pos)
	    : LoggableException(std::move(msg), pos.getPosition(), pos.getContext())
	{
	}

	/**
	 * Returns the position at which the exception occured in the text.
	 *
	 * @return the position descriptor.
	 */
	TextCursor::Position getPosition() const { return pos; }

	/**
	 * Returns the context in which the exception occured in the text.
	 *
	 * @return the context descriptor.
	 */
	TextCursor::Context getContext() const { return ctx; }
};
}

#endif /* _OUSIA_EXCEPTIONS_HPP_ */
