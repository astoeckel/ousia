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
#include "TextCursor.hpp"

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
	 * Describes an included file.
	 */
	struct File {
		/**
		 * Is the name of the file.
		 */
		std::string file;

		/**
		 * Position at which the file was included.
		 */
		TextCursor::Position pos;

		/**
		 * Context in which the file was included.
		 */
		TextCursor::Context ctx;

		/**
		 * Constructor of the File struct.
		 *
		 * @param file is the name of the included file.
		 * @param pos is the position in the parent file, at which this file
		 * was included.
		 * @param ctx is the context in which the feil was included.
		 */
		File(std::string file, TextCursor::Position pos,
		     TextCursor::Context ctx)
		    : file(file), pos(pos), ctx(ctx)
		{
		}
	};

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
		 * Position in the text the message refers to.
		 */
		TextCursor::Position pos;

		/**
		 * Context the message refers to.
		 */
		TextCursor::Context ctx;

		/**
		 * Constructor of the Message struct.
		 *
		 * @param severity describes the message severity.
		 * @param msg contains the actual message.
		 * @param line is the line in the above file the message refers to.
		 * @param column is the column in the above file the message refers to.
		 */
		Message(Severity severity, std::string msg, TextCursor::Position pos,
		        TextCursor::Context ctx)
		    : severity(severity),
		      msg(std::move(msg)),
		      pos(std::move(pos)),
		      ctx(std::move(ctx)){};
	};

private:
	/**
	 * Minimum severity a log message should have before it is discarded.
	 */
	const Severity minSeverity;

	/**
	 * Maximum encountered log message severity.
	 */
	Severity maxEncounteredSeverity;

protected:
	/**
	 * Function to be overriden by child classes to actually display or store
	 * the messages. The default implementation just discards all incomming
	 * messages.
	 *
	 * @param msg is an instance of the Message struct containing the data that
	 * should be logged.
	 */
	virtual void processMessage(Message msg) {}

	/**
	 * Called whenever a new file is pushed onto the stack.
	 *
	 * @param file is the file that should be pushed onto the stack.
	 * @return the stack depth after the file has been pushed.
	 */
	virtual size_t processPushFile(File file) { return 0; }

	/**
	 * Called whenever a file is popped from the stack.
	 *
	 * @return the stack depth after the current file has been removed from the
	 * stack.
	 */
	virtual size_t processPopFile() { return 0; }

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
	 * Logs the given message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param severity is the severity of the log message.
	 * @param msg is the actual log message.
	 * @param pos is the position the log message refers to.
	 * @param ctx describes the context of the log message.
	 */
	void log(Severity severity, std::string msg,
	         TextCursor::Position pos = TextCursor::Position{},
	         TextCursor::Context ctx = TextCursor::Context{})
	{
		// Update the maximum encountered severity level
		if (static_cast<int>(severity) >
		    static_cast<int>(maxEncounteredSeverity)) {
			maxEncounteredSeverity = severity;
		}

		// Only process the message if its severity is larger than the
		// set minimum severity.
		if (static_cast<int>(severity) >= static_cast<int>(minSeverity)) {
			processMessage(Message{severity, std::move(msg), std::move(pos),
			                       std::move(ctx)});
		}
	}

	/**
	 * Logs the given loggable exception.
	 *
	 * @param ex is the exception that should be logged.
	 */
	void log(const LoggableException &ex)
	{
		log(Severity::ERROR, ex.msg, ex.getPosition(), ex.getContext());
	}

	/**
	 * Logs the given message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param severity is the severity of the log message.
	 * @param msg is the actual log message.
	 * @param pos is a reference to a variable which provides position and
	 * context information.
	 */
	template <class PosType>
	void logAt(Severity severity, std::string msg, PosType &pos)
	{
		log(severity, std::move(msg), pos.getPosition(), pos.getContext());
	}

	/**
	 * Logs a debug message. Debug messages will be discarded if the software
	 * is compiled in the release mode (with the NDEBUG flag).
	 *
	 * @param msg is the actual log message.
	 * @param pos describes the position of the debug message.
	 * @param ctx describes the context of the debug message.
	 */
	void debug(std::string msg,
	           TextCursor::Position pos = TextCursor::Position{},
	           TextCursor::Context ctx = TextCursor::Context{})
	{
#ifndef NDEBUG
		log(Severity::DEBUG, std::move(msg), std::move(pos), std::move(ctx));
#endif
	}

	/**
	 * Logs a debug message. Debug messages will be discarded if the software
	 * is compiled in the release mode.
	 *
	 * @param msg is the actual log message.
	 * @param pos is a reference to a variable which provides position and
	 * context information.
	 */
	template<class PosType>
	void debug(std::string msg, PosType &pos)
	{
#ifndef NDEBUG
		logAt(Severity::DEBUG, std::move(msg), pos);
#endif
	}

	/**
	 * Logs a note.
	 *
	 * @param msg is the actual log message.
	 * @param pos describes the position of the note.
	 * @param ctx describes the context of the note.
	 */
	void note(std::string msg,
	          TextCursor::Position pos = TextCursor::Position{},
	          TextCursor::Context ctx = TextCursor::Context{})
	{
		log(Severity::NOTE, std::move(msg), std::move(pos), std::move(ctx));
	}

	/**
	 * Logs a note.
	 *
	 * @param msg is the actual log message.
	 * @param pos is a reference to a variable which provides position and
	 * context information.
	 */
	template<class PosType>
	void note(std::string msg, PosType &pos)
	{
		logAt(Severity::NOTE, std::move(msg), pos);
	}

	/**
	 * Logs a warning.
	 *
	 * @param msg is the actual log message.
	 * @param pos describes the position of the warning.
	 * @param ctx describes the context of the warning.
	 */
	void warning(std::string msg,
	             TextCursor::Position pos = TextCursor::Position{},
	             TextCursor::Context ctx = TextCursor::Context{})
	{
		log(Severity::WARNING, std::move(msg), std::move(pos), std::move(ctx));
	}

	/**
	 * Logs a warning.
	 *
	 * @param msg is the actual log message.
	 * @param pos is a reference to a variable which provides position and
	 * context information.
	 */
	template<class PosType>
	void warning(std::string msg, PosType &pos)
	{
		logAt(Severity::WARNING, std::move(msg), pos);
	}

	/**
	 * Logs an error message.
	 *
	 * @param msg is the actual log message.
	 * @param pos is the position at which the error occured.
	 * @param ctx describes the context in which the error occured.
	 */
	void error(std::string msg,
	           TextCursor::Position pos = TextCursor::Position{},
	           TextCursor::Context ctx = TextCursor::Context{})
	{
		log(Severity::ERROR, std::move(msg), std::move(pos), std::move(ctx));
	}

	/**
	 * Logs an error message.
	 *
	 * @param msg is the actual log message.
	 * @param pos is a reference to a variable which provides position and
	 * context information.
	 */
	template<class PosType>
	void error(std::string msg, PosType &pos)
	{
		logAt(Severity::ERROR, std::move(msg), pos);
	}

	/**
	 * Logs a fatal error message.
	 *
	 * @param msg is the actual log message.
	 * @param pos is the position at which the error occured.
	 * @param ctx describes the context in which the error occured.
	 */
	void fatalError(std::string msg,
	                TextCursor::Position pos = TextCursor::Position{},
	                TextCursor::Context ctx = TextCursor::Context{})
	{
		log(Severity::FATAL_ERROR, std::move(msg), std::move(pos),
		    std::move(ctx));
	}


	/**
	 * Logs a fatal error message.
	 *
	 * @param msg is the actual log message.
	 * @param pos is a reference to a variable which provides position and
	 * context information.
	 */
	template<class PosType>
	void fatalError(std::string msg, PosType &pos)
	{
		logAt(Severity::FATAL_ERROR, std::move(msg), pos);
	}

	/**
	 * Pushes a new file name onto the internal filename stack.
	 *
	 * @param name is the name of the file to be added to the stack.
	 * @param pos is the position from which the new file is included.
	 * @param ctx is the context in which the new file is included.
	 */
	size_t pushFile(std::string name,
	                TextCursor::Position pos = TextCursor::Position{},
	                TextCursor::Context ctx = TextCursor::Context{})
	{
		return processPushFile(
		    File(std::move(name), std::move(pos), std::move(ctx)));
	}

	/**
	 * Pops the filename from the internal filename stack.
	 *
	 * @return the current size of the filename stack.
	 */
	size_t popFile() { return processPopFile(); }

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

	/**
	 * Stack used to keep the file references.
	 */
	std::stack<File> files;

	/**
	 * The size of the stack the last time a file backtrace was printed.
	 */
	size_t lastFilePrinted = 0;

protected:
	void processMessage(Message msg) override;
	size_t processPushFile(File file) override;
	size_t processPopFile() override;

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

	/**
	 * Returns the name of the topmost file.
	 */
	std::string currentFilename();

};
}

#endif /* _OUSIA_LOGGER_HPP_ */

