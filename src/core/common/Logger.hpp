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
 * Contains classes for logging messages in Ousía. Provides various base Logger
 * classes that allow to Fork logger instances or to create a GuardedLogger.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_LOGGER_HPP_
#define _OUSIA_LOGGER_HPP_

#include <cstdint>
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

/**
 * Enum signifying how the message should be displayed. MessageMode constants
 * can be combined using the bitwise or (|) operator.
 */
enum class MessageMode : uint8_t {
	/**
     * Default display mode.
     */
	DEFAULT = 0,

	/**
     * Do not display a context.
     */
	NO_CONTEXT = 1,

	/**
     * Do not display a file backtrace.
     */
	NO_TRACE = 2
};

/**
 * Bitwise or for the MessageMode class.
 *
 * @param a is the first MessageMode.
 * @param b is the second MessageMode.
 * @return the two message modes combined using bitwise or.
 */
inline MessageMode operator|(MessageMode a, MessageMode b)
{
	return static_cast<MessageMode>(static_cast<uint8_t>(a) |
	                                static_cast<uint8_t>(b));
}

/**
 * Checks whether the MessageMode given in "flag" is set in the MessageMode set
 * given in "mode".
 *
 * @param mode is the MessageMode set that should be checked for flag.
 * @param flag is the flag that should be checked in mode.
 * @return true if part is set in mode.
 */
inline bool messageModeSet(MessageMode mode, MessageMode flag)
{
	return static_cast<uint8_t>(mode) & static_cast<uint8_t>(flag);
}

// Forward declaration
class LoggerFork;
class GuardedLogger;

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
	friend GuardedLogger;

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
		 * Message mode.
		 */
		MessageMode mode;

		/**
		 * Actual log message.
		 */
		std::string msg;

		/**
		 * Location passed along with the message.
		 */
		SourceLocation loc;

		/**
		 * Default constructor of the Message struct.
		 */
		Message() : severity(Severity::DEBUG), mode(MessageMode::DEFAULT) {}

		/**
		 * Constructor of the Message struct.
		 *
		 * @param severity describes the message severity.
		 * @param mode is the mode in which the message should be displayed.
		 * @param msg contains the actual message.
		 * @param loc is the location at which the message should be displayed.
		 */
		Message(Severity severity, MessageMode mode, std::string msg,
		        const SourceLocation &loc)
		    : severity(severity), mode(mode), msg(std::move(msg)), loc(loc)
		{
		}
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
	 * Called whenever the pushDefaultLocation function is called.
	 *
	 * @param loc is the default location that should be pushed onto the stack.
	 */
	virtual void processPushDefaultLocation(const SourceLocation &loc) {}

	/**
	 * Called whenever the popDefaultLocation function is called.
	 *
	 * @param loc is the default location that should be popped from the stack.
	 */
	virtual void processPopDefaultLocation() {}

	/**
	 * Called whenever the setDefaultLocation function is called.
	 *
	 * @param loc is the default location that shuold replace the current one on
	 * the stack.
	 */
	virtual void processSetDefaultLocation(const SourceLocation &loc) {}

	/**
	 * Called whenever the setSourceContextCallback function is called.
	 *
	 * @param sourceContextCallback is the callback function that should be set.
	 */
	virtual void processSetSourceContextCallback(
	    SourceContextCallback sourceContextCallback)
	{
	}

public:
	/**
	 * Virtual destructor.
	 */
	virtual ~Logger(){};

	// Default constructor
	Logger() {}

	/**
	 * Logs the given message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param severity is the severity of the log message.
	 * @param msg is the actual log message.
	 * @param loc is the location in the source file the message refers to.
	 * @param mode specifies how the message should be displayed.
	 */
	void log(Severity severity, const std::string &msg,
	         const SourceLocation &loc = SourceLocation{},
	         MessageMode mode = MessageMode::DEFAULT);

	/**
	 * Logs the given loggable exception.
	 *
	 * @param ex is the exception that should be logged.
	 * @param loc is a location which is used in case the exception has no valid
	 * location attached.
	 * @param mode specifies how the message should be displayed.
	 */
	void log(const LoggableException &ex,
	         const SourceLocation &loc = SourceLocation{},
	         MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::ERROR, ex.msg, ex.getLocation().isValid() ? ex.getLocation() : loc);
	}

	/**
	 * Logs the given loggable exception.
	 *
	 * @param ex is the exception that should be logged.
	 * @param loc is a location which (if valid overrides the location given in
	 * the exception.
	 * @param mode specifies how the message should be displayed.
	 */
	template <class LocationType>
	void log(const LoggableException &ex, LocationType loc,
	         MessageMode mode = MessageMode::DEFAULT)
	{
		log(ex, SourceLocation::location(loc), mode);
	}

	/**
	 * Logs the given message. The file name is set to the topmost file name on
	 * the file name stack.
	 *
	 * @param severity is the severity of the log message.
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides location
	 * information.
	 * @param mode specifies how the message should be displayed.
	 */
	template <class LocationType>
	void log(Severity severity, const std::string &msg, LocationType loc,
	         MessageMode mode = MessageMode::DEFAULT)
	{
		log(severity, msg, SourceLocation::location(loc), mode);
	}

	/**
	 * Logs a debug message. Debug messages will be discarded if the software
	 * is compiled in the release mode (with the NDEBUG flag).
	 *
	 * @param msg is the actual log message.
	 * @param loc is the location in the source file the message refers to.
	 */
	void debug(const std::string &msg,
	           const SourceLocation &loc = SourceLocation{},
	           MessageMode mode = MessageMode::DEFAULT)
	{
#ifndef NDEBUG
		log(Severity::DEBUG, msg, loc, mode);
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
	void debug(const std::string &msg, LocationType loc,
	           MessageMode mode = MessageMode::DEFAULT)
	{
#ifndef NDEBUG
		log(Severity::DEBUG, msg, loc, mode);
#endif
	}

	/**
	 * Logs a note.
	 *
	 * @param msg is the actual log message.
	 * @param loc is the location in the source file the message refers to.
	 */
	void note(const std::string &msg,
	          const SourceLocation &loc = SourceLocation{},
	          MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::NOTE, msg, loc, mode);
	}

	/**
	 * Logs a note.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void note(const std::string &msg, LocationType loc,
	          MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::NOTE, msg, loc, mode);
	}

	/**
	 * Logs a warning.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 */
	void warning(const std::string &msg,
	             const SourceLocation &loc = SourceLocation{},
	             MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::WARNING, msg, loc, mode);
	}

	/**
	 * Logs a warning.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void warning(const std::string &msg, LocationType loc,
	             MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::WARNING, msg, SourceLocation::location(loc), mode);
	}

	/**
	 * Logs an error message.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 */
	void error(const std::string &msg,
	           const SourceLocation &loc = SourceLocation{},
	           MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::ERROR, msg, loc, mode);
	}

	/**
	 * Logs an error message.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void error(const std::string &msg, LocationType loc,
	           MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::ERROR, msg, SourceLocation::location(loc), mode);
	}

	/**
	 * Logs a fatal error message.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 */
	void fatalError(const std::string &msg,
	                const SourceLocation &loc = SourceLocation{},
	                MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::FATAL_ERROR, msg, loc, mode);
	}

	/**
	 * Logs a fatal error message.
	 *
	 * @param msg is the actual log message.
	 * @param loc is a reference to a variable which provides position
	 * information.
	 */
	template <class LocationType>
	void fatalError(const std::string &msg, LocationType loc,
	                MessageMode mode = MessageMode::DEFAULT)
	{
		log(Severity::FATAL_ERROR, msg, SourceLocation::location(loc), mode);
	}

	/**
	 * Sets the source context callback to be used to resolve SourceLocation
	 * instances to SourceContext instances. The sourceContextCallback should be
	 * set as early as possible when using the logger.
	 *
	 * @param sourceContextCallback is the new sourceContextCallback to be used.
	 */
	void setSourceContextCallback(SourceContextCallback sourceContextCallback)
	{
		processSetSourceContextCallback(sourceContextCallback);
	}

	/**
	 * Pushes a new default location onto the default location stack.
	 *
	 * @param loc is the location that should be used if no (valid) location is
	 * specified in the Logger.
	 */
	void pushDefaultLocation(const SourceLocation &loc)
	{
		processPushDefaultLocation(loc);
	}

	/**
	 * Pops the last default location from the default location stack.
	 */
	void popDefaultLocation() { processPopDefaultLocation(); }

	/**
	 * Replaces the topmost default location on the location stack with the
	 * given location. Creates a new entry in the location stack if the stack
	 * was empty.
	 *
	 * @param loc is the location that should be used if no (valid) location is
	 * specified in the Logger.
	 */
	void setDefaultLocation(const SourceLocation &loc)
	{
		processSetDefaultLocation(loc);
	}

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
	enum class CallType {
		MESSAGE,
		PUSH_LOCATION,
		POP_LOCATION,
		SET_LOCATION,
		SET_CONTEXT_CALLBACK
	};

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
	 * Vector storing all incomming location instances.
	 */
	std::vector<SourceLocation> locations;

	/**
	 * Vector storing all incomming source context callbacks.
	 */
	std::vector<SourceContextCallback> callbacks;

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
	void processPushDefaultLocation(const SourceLocation &loc) override;
	void processPopDefaultLocation() override;
	void processSetDefaultLocation(const SourceLocation &loc) override;
	void processSetSourceContextCallback(
	    SourceContextCallback sourceContextCallback) override;

public:
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

/**
 * The GuardedLogger class can be used to automatically pop any pushed file from
 * the File stack maintained by a Logger class (in a RAII manner). This
 * simplifies managing pushing and popping files in case there are multiple
 * return calls or exceptions thrown.
 */
class GuardedLogger : public Logger {
private:
	/**
	 * Reference to the parent logger instance.
	 */
	Logger &parent;

	/**
	 * Number of push calls.
	 */
	size_t depth;

protected:
	/**
	 * Relays the processMessage call to the parent logger.
	 *
	 * @param msg is the message to be relayed to the parent logger.
	 */
	void processMessage(const Message &msg) override;

	/**
	 * Relays the filterMessage call to the parent logger.
	 *
	 * @param msg is the message to be relayed to the parent logger.
	 */
	bool filterMessage(const Message &msg) override;

	/**
	 * Relays the processPushDefaultLocation call to the parent logger and
	 * increments the stack depth counter.
	 */
	void processPushDefaultLocation(const SourceLocation &loc) override;

	/**
	 * Relays the processPopDefaultLocation call to the parent logger and
	 * decrements the stack depth counter.
	 */
	void processPopDefaultLocation() override;

	/**
	 * Relays the processSetDefaultLocation call to the parent logger.
	 */
	void processSetDefaultLocation(const SourceLocation &loc) override;

	/**
	 * Relays the processSetSourceContextCallback call to the parent logger.
	 */
	void processSetSourceContextCallback(
	    SourceContextCallback sourceContextCallback) override;

public:
	/**
	 * Constructor of the GuardedLogger class, pushes a first file instance onto
	 * the file stack.
	 *
	 * @param parent is the parent logger instance to which all calls should
	 * be relayed.
	 * @param loc specifies the first source location.
	 */
	GuardedLogger(Logger &parent, SourceLocation loc = SourceLocation{});

	/**
	 * Constructor of the GuardedLogger class, pushes a first file instance onto
	 * the file stack.
	 *
	 * @tparam LocationType is the type of the object pointing at the location.
	 * @param parent is the parent logger instance to which all calls should
	 * be relayed.
	 * @param loc specifies the first source location.
	 */
	template <class LocationType>
	GuardedLogger(Logger &parent, LocationType loc)
	    : GuardedLogger(parent, SourceLocation::location(loc))
	{
	}

	/**
	 * Destructor of the GuardedLogger class, automatically unwinds the file
	 * stack.
	 */
	~GuardedLogger();
};

/**
 * Logger instance which throws each encountered error as LoggableException.
 */
class ExceptionLogger : public Logger {
protected:
	/**
	 * Throws errors and fatal errors as exception.
	 *
	 * @param msg is the message that should be thrown as exception.
	 */
	void processMessage(const Message &msg) override
	{
		if (msg.severity == Severity::ERROR ||
		    msg.severity == Severity::FATAL_ERROR) {
			throw LoggableException(msg.msg, msg.loc);
		}
	}
};

#ifdef NDEBUG
constexpr Severity DEFAULT_MIN_SEVERITY = Severity::NOTE;
#else
constexpr Severity DEFAULT_MIN_SEVERITY = Severity::DEBUG;
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
	 * Vector used to store the counts of each message type.
	 */
	std::vector<size_t> messageCounts;

	/**
	 * Vector used to store the current default locations.
	 */
	std::vector<SourceLocation> locations;

	/**
	 * Minimum severity to be used for filtering messages.
	 */
	Severity minSeverity;

	/**
	 * Current source context callback.
	 */
	SourceContextCallback sourceContextCallback;

protected:
	/**
	 * Filters the messages according to the given minimum severity.
	 *
	 * @param msg is the message that should be filtered.
	 * @return true if the message has a higher or equal severity compared to
	 * the minimum severity.
	 */
	bool filterMessage(const Message &msg) override;

	void processPushDefaultLocation(const SourceLocation &loc) override;
	void processPopDefaultLocation() override;
	void processSetDefaultLocation(const SourceLocation &loc) override;
	void processSetSourceContextCallback(
	    SourceContextCallback sourceContextCallback) override;

public:
	/**
	 * Creates a ConcreteLogger instance with the given minimum severity.
	 *
	 * @param minSeverity is the severity below which message should be
	 * discarded.
	 */
	ConcreteLogger(Severity minSeverity = DEFAULT_MIN_SEVERITY);

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
	 * @param severity is the log severity for which the message count
	 *should
	 * be returned.
	 * @return the number of messages for this severity. Returns zero for
	 * invalid arguments.
	 */
	size_t getSeverityCount(Severity severity);

	/**
	 * Resets the statistics gathered by the ConcreteLogger instance (the
	 * number
	 * of messages per log severity) and the internal file stack.
	 */
	void reset();

	/**
	 * Returns true if at least one message with either a fatal error or
	 * error severity was logged.
	 *
	 * @return true if an error or fatal error was logged.
	 */
	bool hasError();

	/**
	 * Returns true if at least one message with either a fatal error was
	 * logged.
	 *
	 * @return true if a fatal error was logged.
	 */
	bool hasFatalError();
};
}

#endif /* _OUSIA_LOGGER_HPP_ */

