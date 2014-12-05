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
	                                 const std::string &file, bool fatal,
	                                 int line, int column);

public:
	/**
	 * Message describing the error that occured.
	 */
	const std::string msg;

	/**
	 * Name of the file in which the error occured. May be empty.
	 */
	const std::string file;

	/**
	 * If set to true, the exception should not be handled as recoverable error
	 * but as "fatal" error.
	 */
	const bool fatal;

	/**
	 * Line at which the exception occured. Negative values are ignored.
	 */
	const int line;

	/**
	 * Column at which the exception occured. Negative values are ignored.
	 */
	const int column;

	/**
	 * Constructor of the LoggableException class.
	 *
	 * @param msg contains the error message.
	 * @param file provides the context the message refers to. May be empty.
	 * @param line is the line in the above file the message refers to.
	 * @param column is the column in the above file the message refers to.
	 * @param fatal shoudl be set to true if the error is non-recoverable.
	 */
	LoggableException(std::string msg, std::string file, bool fatal,
	                  int line = -1, int column = -1)
	    : OusiaException(formatMessage(msg, file, fatal, line, column)),
	      msg(std::move(msg)),
	      file(std::move(file)),
	      fatal(fatal),
	      line(line),
	      column(column)
	{
	}

	/**
	 * Constructor of the LoggableException class with empty file.
	 *
	 * @param msg contains the error message.
	 * @param fatal should be set to true if the error is non-recoverable.
	 * @param line is the line in the above file the message refers to.
	 * @param column is the column in the above file the message refers to.
	 */
	LoggableException(std::string msg, bool fatal, int line = -1, int column = -1)
	    : OusiaException(formatMessage(msg, "", fatal, line, column)),
	      msg(std::move(msg)),
	      fatal(fatal),
	      line(line),
	      column(column)
	{
	}

	/**
	 * Constructor of the LoggableException class with empty file.
	 *
	 * @param msg contains the error message.
	 * @param fatal should be set to true if the error is non-recoverable.
	 */
	LoggableException(std::string msg, bool fatal)
	    : OusiaException(formatMessage(msg, "", fatal, -1, -1)),
	      msg(std::move(msg)),
	      fatal(fatal),
	      line(-1),
	      column(-1)
	{
	}

	/**
	 * Constructor of the LoggableException class with empty file and an
	 * position object.
	 *
	 * @param msg is the actual log message.
	 * @param pos is a const reference to a variable which provides position
	 * information.
	 * @param fatal should be set to true if the error is non-recoverable.
	 */
	template <class PosType>
	LoggableException(std::string msg, bool fatal, const PosType &pos)
	    : OusiaException(
	          formatMessage(msg, "", fatal, pos.getLine(), pos.getColumn())),
	      msg(std::move(msg)),
	      fatal(fatal),
	      line(pos.getLine()),
	      column(pos.getColumn())
	{
	}
};
}

#endif /* _OUSIA_EXCEPTIONS_HPP_ */

