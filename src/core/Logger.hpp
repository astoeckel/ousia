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
 * @file Logger.hpp
 *
 * Contains classes for logging messages in Ousía. Provides a generic Logger
 * class, and TerminalLogger, an extension of Logger which logs do an output
 * stream.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_LOGGER_HPP_
#define _OUSIA_LOGGER_HPP_

#include <ostream>
#include <stack>
#include <string>
#include <vector>

#include "Exceptions.hpp"

namespace ousia {

/**
 * Enum containing the severities used for logging errors and debug messages.
 */
enum class Severity : int {
	/**
     * Indicates that this message was only printed for debugging. Note that
     * in release builds messages with this severity are discarded.
     */
	DEBUG = 0,

	/**
     * A message which might provide additional information to the user.
     */
	NOTE = 1,

	/**
     * A message which warns of possible mistakes by the user which might not be
     * actual errors but may lead to unintended behaviour.
     */
	WARNING = 2,

	/**
     * An error occurred while processing, however program execution continues,
     * trying to deal with the error situation (graceful degradation). However,
     * messages with this severity may be followed up by fatal errors.
     */
	ERROR = 3,

	/**
     * A fatal error occurred. Program execution cannot continue.
     */
	FATAL_ERROR = 4
};

#ifdef NDEBUG
static constexpr Severity DEFAULT_MIN_SEVERITY = Severity::NOTE;
#else
static constexpr Severity DEFAULT_MIN_SEVERITY = Severity::DEBUG;
#endif

/**
 * The Logger class is the base class the individual logging systems should
 * derive from. It provides a simple interface for logging errors, warnings and
 * notes and filters these according to the set minimum severity. Additionally
 * a stack of file names is maintained in order to allow simple descent into
 * included files. Note however, that this base Logger class simply discards the
 * incomming log messages. Use one of the derived classes to actually handle the
 * log messages.
 */
class Logger {
public:
	/**
	 * The message struct represents a single log message and all information
	 * attached to it.
	 */
	struct Message {
		/**
		 * Severity of the log message.
		 */
		Severity severity;

		/**
		 * Actual log message.
		 */
		std::string msg;

		/**
		 * Refers to the file which provides the context for this error message.
		 * May be empty.
		 */
		std::string file;

		/**
		 * Line in the above file the error message refers to. Ignored if
		 * smaller than zero.
		 */
		int line;

		/**
		 * Column in the above file the error message refers to. Ignored if
		 * smaller than zero.
		 */
		int column;

		/**
		 * Constructor of the Message struct.
		 *
		 * @param severity describes the message severity.
		 * @param msg contains the actual message.
		 * @param file provides the context the message refers to. May be empty.
		 * @param line is the line in the above file the message refers to.
		 * @param column is the column in the above file the message refers to.
		 */
		Message(Severity severity, std::string msg, std::string file, int line,
		        int column)
		    : severity(severity),
		      msg(std::move(msg)),
		      file(std::move(file)),
		      line(line),
		      column(column){};

		/**
		 * Returns true if the file string is set.
		 *
		 * @return true if the file string is set.
		 */
		bool hasFile() const { return !file.empty(); }

		/**
		 * Returns true if the line is set.
		 *
		 * @return true if the line number is a non-negative integer.
		 */
		bool hasLine() const { return line >= 0; }

		/**
		 * Returns true if column and line are set (since a column has no
		 * significance without a line number).
		 *
		 * @return true if line number and column number are non-negative
		 * integers.
		 */
		bool hasColumn() const { return hasLine() && column >= 0; }
	};

private:
	/**
	 * Minimum severity a log message should have before it is discarded.
	 */
	Severity minSeverity;

	/**
	 * Maximum encountered log message severity.
	 */
	Severity maxEncounteredSeverity;

	/**
	 * Stack containing the current file names that have been processed.
	 */
	std::stack<std::string> filenameStack;

protected:
	/**
	 * Function to be overriden by child classes to actually display or store
	 * the messages. The default implementation just discards all incomming
	 * messages.
	 *
	 * @param msg is an instance of the Message struct containing the data that
	 * should be logged.
	 */
	virtual void process(const Message &msg){};

public:
	/**
	 * Constructor of the Logger class.
	 *
	 * @param minSeverity is the minimum severity a log message should have.
	 * Messages below this severity are discarded.
	 */
	Logger(Severity minSeverity = DEFAULT_MIN_SEVERITY)
	    : minSeverity(minSeverity), maxEncounteredSeverity(Severity::DEBUG)
	{
	}

	Logger(const Logger &) = delete;

	/**
	 * Virtual destructor.
	 */
	virtual ~Logger(){};

	/**
	 * Logs the given message. Most generic log function.
	 *
	 * @param severity is the severity of the log message.
	 * @param msg is the actual log message.
	 * @param file is the name of the file the message refers to. May be empty.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void log(Severity severity, const std::string &msg, const std::string &file,
	         int line = -1, int column = -1);

	/**
	 * Logs the given message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param severity is the severity of the log message.
	 * @param msg is the actual log message.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void log(Severity severity, const std::string &msg, int line = -1,
	         int column = -1)
	{
		log(severity, msg, currentFilename(), line, column);
	}

	/**
	 * Logs the given loggable exception.
	 *
	 * @param ex is the exception that should be logged.
	 */
	void log(const LoggableException &ex)
	{
		log(ex.fatal ? Severity::FATAL_ERROR : Severity::ERROR, ex.msg,
		    ex.file.empty() ? currentFilename() : ex.file, ex.line, ex.column);
	}

	/**
	 * Logs a debug message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param file is the name of the file the message refers to. May be empty.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void debug(const std::string &msg, const std::string &file, int line = -1, int column = -1)
	{
		log(Severity::DEBUG, msg, file, line, column);
	}

	/**
	 * Logs a debug message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void debug(const std::string &msg, int line = -1, int column = -1)
	{
		debug(msg, currentFilename(), line, column);
	}

	/**
	 * Logs a note. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param file is the name of the file the message refers to. May be empty.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void note(const std::string &msg, const std::string &file, int line = -1, int column = -1)
	{
		log(Severity::NOTE, msg, file, line, column);
	}

	/**
	 * Logs a note. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void note(const std::string &msg, int line = -1, int column = -1)
	{
		note(msg, currentFilename(), line, column);
	}

	/**
	 * Logs a warning. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param file is the name of the file the message refers to. May be empty.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void warning(const std::string &msg, const std::string &file, int line = -1, int column = -1)
	{
		log(Severity::WARNING, msg, file, line, column);
	}

	/**
	 * Logs a warning. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void warning(const std::string &msg, int line = -1, int column = -1)
	{
		warning(msg, currentFilename(), line, column);
	}

	/**
	 * Logs an error message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param file is the name of the file the message refers to. May be empty.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void error(const std::string &msg, const std::string &file, int line = -1, int column = -1)
	{
		log(Severity::ERROR, msg, file, line, column);
	}

	/**
	 * Logs an error message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void error(const std::string &msg, int line = -1, int column = -1)
	{
		error(msg, currentFilename(), line, column);
	}

	/**
	 * Logs a fatal error. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param file is the name of the file the message refers to. May be empty.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void fatalError(const std::string &msg, const std::string &file, int line = -1, int column = -1)
	{
		log(Severity::FATAL_ERROR, msg, file, line, column);
	}

	/**
	 * Logs a fatal error. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param msg is the actual log message.
	 * @param line is the line in the above file at which the error occured.
	 * Ignored if negative.
	 * @param column is the column in the above file at which the error occured.
	 * Ignored if negative.
	 */
	void fatalError(const std::string &msg, int line = -1, int column = -1)
	{
		fatalError(msg, currentFilename(), line, column);
	}

	/**
	 * Pushes a new file name onto the internal filename stack.
	 *
	 * @param name is the name of the file that should be added to the filename
	 * stack.
	 * @return the size of the filename stack. This number can be passed to the
	 * "unwindFilenameStack" method in order to return the stack to state it was
	 * in after this function has been called.
	 */
	unsigned int pushFilename(const std::string &name);

	/**
	 * Pops the filename from the internal filename stack.
	 *
	 * @return the current size of the filename stack.
	 */
	unsigned int popFilename();

	/**
	 * Pops elements from the filename stack while it has more elements than
	 * the given number and the stack is non-empty.
	 *
	 * @param pos is the position the filename stack should be unwound to. Use
	 * a number returned by pushFilename.
	 */
	void unwindFilenameStack(unsigned int pos);

	/**
	 * Returns the topmost filename from the internal filename stack.
	 *
	 * @return the topmost filename from the filename stack or an empty string
	 * if the filename stack is empty.
	 */
	std::string currentFilename()
	{
		return filenameStack.empty() ? std::string{} : filenameStack.top();
	}

	/**
	 * Returns the maximum severity that was encountered by the Logger but at
	 * least Severity::DEBUG.
	 *
	 * @return the severity of the most severe log message but at least
	 * Severity::DEBUG.
	 */
	Severity getMaxEncounteredSeverity() { return maxEncounteredSeverity; }

	/**
	 * Returns the minimum severity. Messages with a smaller severity are
	 * discarded.
	 *
	 * @return the minimum severity.
	 */
	Severity getMinSeverity() { return minSeverity; }

	/**
	 * Sets the minimum severity. Messages with a smaller severity will be
	 * discarded. Only new messages will be filtered according to the new value.
	 *
	 * @param severity is the minimum severity for new log messages.
	 */
	void setMinSeverity(Severity severity) { minSeverity = severity; }
};

/**
 * Class extending the Logger class and printing the log messages to the given
 * stream.
 */
class TerminalLogger : public Logger {
private:
	/**
	 * Reference to the target output stream.
	 */
	std::ostream &os;

	/**
	 * If true, the TerminalLogger will use colors to make the log messages
	 * prettier.
	 */
	bool useColor;

protected:
	/**
	 * Implements the process function and logs the messages to the output.
	 */
	void process(const Message &msg) override;

public:
	/**
	 * Constructor of the TerminalLogger class.
	 *
	 * @param os is the output stream the log messages should be logged to.
	 * Should be set to std::cerr in most cases.
	 * @param useColor if true, the TerminalLogger class will do its best to
	 * use ANSI/VT100 control sequences for colored log messages.
	 * @param minSeverity is the minimum severity below which log messages are
	 * discarded.
	 */
	TerminalLogger(std::ostream &os, bool useColor = false,
	               Severity minSeverity = DEFAULT_MIN_SEVERITY)
	    : Logger(minSeverity), os(os), useColor(useColor)
	{
	}
};
}

#endif /* _OUSIA_LOGGER_HPP_ */

