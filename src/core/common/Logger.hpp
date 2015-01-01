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

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

#include "Exceptions.hpp"
#include "Location.hpp"

namespace ousia {

/**
 * Enum containing the severities used for logging errors and debug messages.
 */
enum class Severity : uint8_t {
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

// Forward declaration
class LoggerFork;

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
	friend LoggerFork;

	/**
	 * Describes a file inclusion.
	 */
	struct File {
		/**
		 * Current filename.
		 */
		std::string file;

		/**
		 * Location at which the file was included.
		 */
		SourceLocation loc;

		/**
		 * Callback used to retrieve the context for a certain location
		 */
		SourceContextCallback ctxCallback;

		/**
		 * Data to be passed to the callback.
		 */
		void *ctxCallbackData;

		/**
		 * Constructor of the Scope struct.
		 *
		 * @param type is the type of
		 * @param file is the name of the current file.
		 * @param loc is the location at which the file was included.
		 * @param ctxCallback is the callback function that should be called
		 * for looking up the context belonging to a SourceLocation instance.
		 * @param ctxCallbackData is additional data that should be passed to
		 * the callback function.
		 */
		File(std::string file, SourceLocation loc,
		     SourceContextCallback ctxCallback, void *ctxCallbackData)
		    : file(std::move(file)),
		      loc(loc),
		      ctxCallback(ctxCallback),
		      ctxCallbackData(ctxCallbackData)
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
		 * Location passed along with the message.
		 */
		SourceLocation loc;

		/**
		 * Constructor of the Message struct.
		 *
		 * @param severity describes the message severity.
		 * @param msg contains the actual message.
		 */
		Message(Severity severity, std::string msg, const SourceLocation &loc)
		    : severity(severity), msg(std::move(msg)), loc(loc){};
	};

protected:
	/**
	 * Function to be overriden by child classes to actually display or store
	 * the messages. The default implementation just discards all incomming
	 * messages.
	 *
	 * @param msg is an instance of the Message struct containing the data that
	 * should be logged.
	 */
	virtual void processMessage(const Message &msg) {}

	/**
	 * Called right before the processMessage function is called. Allows any
	 * concrete implementation of the Logger class to discard certain messages.
	 *
	 * @param msg is the message that should be filtered.
	 * @return true if the message should be passed to the processMessage
	 * method, false otherwise.
	 */
	virtual bool filterMessage(const Message &msg) { return true; }

	/**
	 * Called whenever a new file is pushed onto the stack.
	 *
	 * @param file is the file structure that should be stored on the stack.
	 */
	virtual void processPushFile(const File &file) {}

	/**
	 * Called whenever a scope is popped from the stack.
	 */
	virtual void processPopFile() {}

	/**
	 * Called whenever the setDefaultLocation function is called.
	 *
	 * @param loc is the default location that should be set.
	 */
	virtual void processSetDefaultLocation(const SourceLocation &loc) {}

public:
	/**
	 * Virtual destructor.
	 */
	virtual ~Logger(){};

	// Default constructor
	Logger() {}

	// No copy
	Logger(const Logger &) = delete;

	// No assign
	Logger &operator=(const Logger &) = delete;

	/**
	 * Logs the given message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param severity is the severity of the log message.
	 * @param msg is the actual log message.
	 * @param loc is the location in the source file the message refers to.
	 */
	void log(Severity severity, const std::string &msg,
	         const SourceLocation &loc = SourceLocation{});

	/**
	 * Logs the given loggable exception.
	 *
	 * @param ex is the exception that should be logged.
	 */
	void log(const LoggableException &ex)
	{
		log(Severity::ERROR, ex.msg, ex.getLocation());
	}

	/**
	 * Logs the given message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param severity is the severity of the log message.
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides location
	 * information.
	 */
	template <class LocationType>
	void log(Severity severity, const std::string &msg, LocationType &loc)
	{
		log(severity, msg, loc.getLocation());
	}

	/**
	 * Logs a debug message. Debug messages will be discarded if the software
	 * is compiled in the release mode (with the NDEBUG flag).
	 *
	 * @param msg is the actual log message.
	 * @param loc is the location in the source file the message refers to.
	 */
	void debug(const std::string &msg,
	           const SourceLocation &loc = SourceLocation{})
	{
#ifndef NDEBUG
		log(Severity::DEBUG, msg, loc);
#endif
	}

	/**
	 * Logs a debug message. Debug messages will be discarded if the software
	 * is compiled in the release mode.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void debug(const std::string &msg, LocationType &loc)
	{
#ifndef NDEBUG
		log(Severity::DEBUG, msg, loc);
#endif
	}

	/**
	 * Logs a note.
	 *
	 * @param msg is the actual log message.
	 * @param loc is the location in the source file the message refers to.
	 */
	void note(const std::string &msg,
	          const SourceLocation &loc = SourceLocation{})
	{
		log(Severity::NOTE, msg, loc);
	}

	/**
	 * Logs a note.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void note(const std::string &msg, LocationType &loc)
	{
		log(Severity::NOTE, msg, loc);
	}

	/**
	 * Logs a warning.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 */
	void warning(const std::string &msg,
	             const SourceLocation &loc = SourceLocation{})
	{
		log(Severity::WARNING, msg, loc);
	}

	/**
	 * Logs a warning.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void warning(const std::string &msg, LocationType &loc)
	{
		log(Severity::WARNING, msg, loc);
	}

	/**
	 * Logs an error message.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 */
	void error(const std::string &msg,
	           const SourceLocation &loc = SourceLocation{})
	{
		log(Severity::ERROR, msg, std::move(loc));
	}

	/**
	 * Logs an error message.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void error(const std::string &msg, LocationType &loc)
	{
		log(Severity::ERROR, msg, loc);
	}

	/**
	 * Logs a fatal error message.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 */
	void fatalError(const std::string &msg,
	                const SourceLocation &loc = SourceLocation{})
	{
		log(Severity::FATAL_ERROR, msg, loc);
	}

	/**
	 * Logs a fatal error message.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void fatalError(const std::string &msg, LocationType &loc)
	{
		log(Severity::FATAL_ERROR, msg, loc);
	}

	/**
	 * Pushes a new file name onto the internal filename stack.
	 *
	 * @param name is the name of the file to be added to the stack.
	 * @param loc is the position from which the new file is included.
	 */
	void pushFile(std::string name, SourceLocation loc = SourceLocation{},
	              SourceContextCallback contextCallback = nullptr,
	              void *contextCallbackData = nullptr)
	{
		processPushFile(
		    File(std::move(name), loc, contextCallback, contextCallbackData));
	}

	/**
	 * Pops the filename from the internal filename stack. Resets any location
	 * set by the setDefaultLocation() method.
	 */
	void popFile()
	{
		processPopFile();
		resetDefaultLocation();
	}

	/**
	 * Sets the default location. The default location is automatically reset
	 * once the popFile() method is called.
	 *
	 * @param loc is the location that should be used if no (valid) location is
	 * specified in the Logger.
	 */
	void setDefaultLocation(const SourceLocation &loc)
	{
		processSetDefaultLocation(loc);
	}

	/**
	 * Resets the default location, a previously set default location will be
	 * no longer used.
	 */
	void resetDefaultLocation() { processSetDefaultLocation(SourceLocation{}); }

	/**
	 * Returns a forked logger instance which can be used to collect log
	 * messages for which it is not sure whether they will be used.
	 *
	 * @return a LoggerFork instance which buffers all method calls and commits
	 * them once the "commit()" method is called.
	 */
	LoggerFork fork();
};

/**
 * Fork of the Logger -- stores all logged messages without actually pushing
 * them to the underlying logger instance. Maintains its own
 * maxEncounteredSeverity independently from the parent Logger instance.
 * Internally the LoggerFork class records all calls to the internal
 * processMessage, processPushScope and processPopFile calls and replays these
 * calls in the exact order on the parent Logger instance once the commit
 * function is called.
 */
class LoggerFork : public Logger {
private:
	friend Logger;

	/**
	 * Intanally used to store the incomming function calls.
	 */
	enum class CallType { MESSAGE, PUSH_FILE, POP_FILE, SET_DEFAULT_LOCATION };

	/**
	 * Datastructure used to represent a logger function call.
	 */
	struct Call {
		/**
		 * Type of the function call.
		 */
		CallType type;

		/**
		 * Index of the associated data in the type-specific vector.
		 */
		size_t dataIdx;

		/**
		 * Constructor of the Call structure.
		 *
		 * @param type is the type of the call.
		 * @param dataIdx is the index of the associated data in the type
		 * specific data vector.
		 */
		Call(CallType type, size_t dataIdx) : type(type), dataIdx(dataIdx) {}
	};

	/**
	 * Vector storing all incomming calls.
	 */
	std::vector<Call> calls;

	/**
	 * Vector storing all incomming messages.
	 */
	std::vector<Message> messages;

	/**
	 * Vector storing all incomming pushed Scope instances.
	 */
	std::vector<File> files;

	/**
	 * Vector storing all incomming location instances.
	 */
	std::vector<SourceLocation> locations;

	/**
	 * Parent logger instance.
	 */
	Logger *parent;

	/**
	 * Constructor of the LoggerFork class.
	 *
	 * @param parent is the parent logger instance.
	 */
	LoggerFork(Logger *parent) : parent(parent) {}

protected:
	void processMessage(const Message &msg) override;
	void processPushFile(const File &file) override;
	void processPopFile() override;
	void processSetDefaultLocation(const SourceLocation &loc) override;

public:
	// Default move constructor
	LoggerFork(LoggerFork &&l)
	    : calls(std::move(l.calls)),
	      messages(std::move(l.messages)),
	      files(std::move(l.files)),
	      locations(std::move(l.locations)),
	      parent(std::move(l.parent)){};

	/**
	 * Commits all collected messages to the parent Logger instance.
	 */
	void commit();

	/**
	 * Purges all collected messages. Resets the LoggerFork to its initial
	 * state (except for the maximum encountered severity).
	 */
	void purge();
};

#ifdef NDEBUG
static constexpr Severity DEFAULT_MIN_SEVERITY = Severity::NOTE;
#else
static constexpr Severity DEFAULT_MIN_SEVERITY = Severity::DEBUG;
#endif

/**
 * The ConcreteLogger class contains data fields used to specify the minimum
 * severity and to gather statistics about encountered log messages.
 * Additionally it provides the File stack and helper functions that can be
 * used to access location and context of a given message.
 */
class ConcreteLogger : public Logger {
private:
	/**
	 * Stack containing the current file instance.
	 */
	std::vector<File> files;

	/**
	 * Vector used to store the counts of each message type.
	 */
	std::vector<size_t> messageCounts;

	/**
	 * Minimum severity to be used for filtering messages.
	 */
	Severity minSeverity;

	/**
	 * Current default location.
	 */
	SourceLocation defaultLocation;

protected:
	/**
	 * Filters the messages according to the given minimum severity.
	 *
	 * @param msg is the message that should be filtered.
	 * @return true if the message has a higher or equal severity compared to
	 * the minimum severity.
	 */
	bool filterMessage(const Message &msg) override;

	/**
	 * Pushes the given file descriptor onto the internal file stack.
	 *
	 * @param file is the File descriptor to be pushed onto the internal file
	 * stack.
	 */
	void processPushFile(const File &file) override;

	/**
	 * Pops the given file descriptor from the internal file stack.
	 */
	void processPopFile() override;

	/**
	 * Sets the default location.
	 *
	 * @param loc is the new default location.
	 */
	void processSetDefaultLocation(const SourceLocation &loc) override;

public:
	/**
	 * Creates a ConcreteLogger instance with the given minimum severity.
	 *
	 * @param minSeverity is the severity below which message should be
	 * discarded.
	 */
	ConcreteLogger(Severity minSeverity = DEFAULT_MIN_SEVERITY)
	    : minSeverity(minSeverity)
	{
	}

	/**
	 * Returns the name of the current file or an empty instance of the File
	 * instance if no current file is available.
	 *
	 * @return the name of the current file.
	 */
	const File &currentFile() const;

	/**
	 * Returns the current filename or an empty string if no surch file is
	 * available.
	 */
	const std::string &currentFilename() const;

	/**
	 * Returns the current cursor location.
	 *
	 * @return the current cursor location.
	 */
	const SourceLocation &messageLocation(const Message &msg) const;

	/**
	 * Returns the current cursor context.
	 *
	 * @return the current cursor context.
	 */
	SourceContext messageContext(const Message &msg) const;

	/**
	 * Returns the maximum encountered severity.
	 *
	 * @return the maximum encountered severity.
	 */
	Severity getMaxEncounteredSeverity();

	/**
	 * Returns the number of messages for the given severity.
	 *
	 * @param severity is the log severity for which the message count should
	 * be returned.
	 * @return the number of messages for this severity. Returns zero for
	 * invalid arguments.
	 */
	size_t getSeverityCount(Severity severity);

	/**
	 * Resets the statistics gathered by the ConcreteLogger instance (the number
	 * of messages per log severity) and the internal file stack.
	 */
	void reset();

	/**
	 * Returns true if at least one message with either a fatal error or error
	 * severity was logged.
	 *
	 * @return true if an error or fatal error was logged.
	 */
	bool hasError();
};

/**
 * Class extending the Logger class and printing the log messages to the given
 * stream.
 */
class TerminalLogger : public ConcreteLogger {
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
	void processMessage(const Message &msg) override;

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
	    : ConcreteLogger(minSeverity), os(os), useColor(useColor)
	{
	}
};
}

#endif /* _OUSIA_LOGGER_HPP_ */

