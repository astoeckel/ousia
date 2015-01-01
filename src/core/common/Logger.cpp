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

#include <iostream>
#include <sstream>

#include "Logger.hpp"

namespace ousia {

/* Class Logger */

void Logger::log(Severity severity, const std::string &msg,
                 const SourceLocation &loc)
{
	// Assemble the message and pass it through the filter, then process it
	Message message { severity, std::move(msg), loc };
	if (filterMessage(message)) {
		processMessage(message);
	}
}

LoggerFork Logger::fork() { return LoggerFork(this); }

/* Class LoggerFork */

void LoggerFork::processMessage(const Message &msg)
{
	calls.push_back(Call(CallType::MESSAGE, messages.size()));
	messages.push_back(msg);
}

void LoggerFork::processPushFile(const File &file)
{
	calls.push_back(Call(CallType::PUSH_FILE, files.size()));
	files.push_back(file);
}

void LoggerFork::processPopFile()
{
	calls.push_back(Call(CallType::POP_FILE, 0));
}

void LoggerFork::processSetDefaultLocation(const SourceLocation &loc)
{
	// Check whether setDefaultLocation was called immediately before, if yes,
	// simply override the data
	if (!calls.empty() && calls.back().type == CallType::SET_DEFAULT_LOCATION) {
		locations.back() = loc;
	} else {
		calls.push_back(Call(CallType::SET_DEFAULT_LOCATION, locations.size()));
		locations.push_back(loc);
	}
}

void LoggerFork::purge()
{
	calls.clear();
	messages.clear();
	files.clear();
	locations.clear();
}

void LoggerFork::commit()
{
	for (const Call &call : calls) {
		switch (call.type) {
			case CallType::MESSAGE: {
				if (parent->filterMessage(messages[call.dataIdx])) {
					parent->processMessage(messages[call.dataIdx]);
				}
				break;
			}
			case CallType::PUSH_FILE: {
				parent->processPushFile(files[call.dataIdx]);
				break;
			}
			case CallType::POP_FILE:
				parent->processPopFile();
				break;
			case CallType::SET_DEFAULT_LOCATION:
				parent->processSetDefaultLocation(locations[call.dataIdx]);
				break;
		}
	}
	purge();
}

/* Class ConcreteLogger */

static const Logger::File EMPTY_FILE{"", SourceLocation{}, nullptr, nullptr};

void ConcreteLogger::processPushFile(const File &file)
{
	files.push_back(file);
}

void ConcreteLogger::processPopFile() { files.pop_back(); }

bool ConcreteLogger::filterMessage(const Message &msg)
{
	// Increment the message count for this severity
	uint8_t sev = static_cast<uint8_t>(msg.severity);
	if (sev >= messageCounts.size()) {
		messageCounts.resize(sev + 1);
	}
	messageCounts[sev]++;

	// Filter messages with too small severity
	return sev >= static_cast<uint8_t>(minSeverity);
}

void ConcreteLogger::processSetDefaultLocation(const SourceLocation &loc)
{
	defaultLocation = loc;
}

const Logger::File &ConcreteLogger::currentFile() const
{
	if (!files.empty()) {
		return files.back();
	}
	return EMPTY_FILE;
}

const std::string &ConcreteLogger::currentFilename() const
{
	return currentFile().file;
}

const SourceLocation &ConcreteLogger::messageLocation(const Message &msg) const
{
	if (msg.loc.valid()) {
		return msg.loc;
	}
	return defaultLocation;
}

SourceContext ConcreteLogger::messageContext(const Message &msg) const
{
	const Logger::File &file = currentFile();
	const SourceLocation &loc = messageLocation(msg);
	if (file.ctxCallback && loc.valid()) {
		return file.ctxCallback(loc, file.ctxCallbackData);
	}
	return SourceContext{};
}

Severity ConcreteLogger::getMaxEncounteredSeverity()
{
	for (ssize_t i = messageCounts.size() - 1; i >= 0; i--) {
		if (messageCounts[i] > 0) {
			return static_cast<Severity>(i);
		}
	}
	return Severity::DEBUG;
}

size_t ConcreteLogger::getSeverityCount(Severity severity)
{
	uint8_t sev = static_cast<uint8_t>(severity);
	if (sev >= messageCounts.size()) {
		return 0;
	}
	return messageCounts[sev];
}

void ConcreteLogger::reset()
{
	files.clear();
	messageCounts.clear();
}

bool ConcreteLogger::hasError()
{
	return getSeverityCount(Severity::ERROR) > 0 ||
	       getSeverityCount(Severity::FATAL_ERROR) > 0;
}

/* Class Terminal */

/**
 * The Terminal class contains some helper functions used to interact with the
 * terminal as used for colorful output when logging error messages.
 *
 * TODO: Disable on Windows or use corresponding API-functions for setting the
 * color.
 */
class Terminal {
private:
	/**
	 * If set to false, no control codes are generated.
	 */
	bool active;

public:
	/**
	 * ANSI color code for black.
	 */
	static const int BLACK = 30;

	/**
	 * ANSI color code for red.
	 */
	static const int RED = 31;

	/**
	 * ANSI color code for green.
	 */
	static const int GREEN = 32;

	/**
	 * ANSI color code for yellow.
	 */
	static const int YELLOW = 33;

	/**
	 * ANSI color code for blue.
	 */
	static const int BLUE = 34;

	/**
	 * ANSI color code for magenta.
	 */
	static const int MAGENTA = 35;

	/**
	 * ANSI color code for cyan.
	 */
	static const int CYAN = 36;

	/**
	 * ANSI color code for white.
	 */
	static const int WHITE = 37;

	/**
	 * Creates a new instance of the Terminal class.
	 *
	 * @param active specifies whether color codes should be generated.
	 */
	Terminal(bool active) : active(active) {}

	/**
	 * Returns a control string for switching to the given color.
	 *
	 * @param color is the color the terminal should switch to.
	 * @param bright specifies whether the terminal should switch to the bright
	 * mode.
	 * @return a control string to be included in the output stream.
	 */
	std::string color(int color, bool bright = true) const
	{
		if (!active) {
			return std::string{};
		}
		std::stringstream ss;
		ss << "\x1b[";
		if (bright) {
			ss << "1;";
		}
		ss << color << "m";
		return ss.str();
	}

	/**
	 * Returns a control string for switching to the bright mode.
	 *
	 * @return a control string to be included in the output stream.
	 */
	std::string bright() const
	{
		if (!active) {
			return std::string{};
		}
		std::stringstream ss;
		ss << "\x1b[1m";
		return ss.str();
	}

	/**
	 * Returns a control string for switching to the default mode.
	 *
	 * @return a control string to be included in the output stream.
	 */
	std::string reset() const
	{
		if (!active) {
			return std::string{};
		}
		return "\x1b[0m";
	}
};

/* Class TerminalLogger */

void TerminalLogger::processMessage(const Message &msg)
{
	Terminal t(useColor);

	// Fetch filename, position and context
	const std::string filename = currentFilename();
	const SourceLocation pos = messageLocation(msg);
	const SourceContext ctx = messageContext(msg);

	// Print the file name
	bool hasFile = !filename.empty();
	if (hasFile) {
		os << t.bright() << filename << t.reset();
	}

	// Print line and column number
	if (pos.hasLine()) {
		if (hasFile) {
			os << ':';
		}
		os << t.bright() << pos.line << t.reset();
		if (pos.hasColumn()) {
			os << ':' << pos.column;
		}
	}

	// Print the optional seperator
	if (hasFile || pos.hasLine()) {
		os << ": ";
	}

	// Print the severity
	switch (msg.severity) {
		case Severity::DEBUG:
			break;
		case Severity::NOTE:
			os << t.color(Terminal::CYAN, true) << "note: ";
			break;
		case Severity::WARNING:
			os << t.color(Terminal::MAGENTA, true) << "warning: ";
			break;
		case Severity::ERROR:
			os << t.color(Terminal::RED, true) << "error: ";
			break;
		case Severity::FATAL_ERROR:
			os << t.color(Terminal::RED, true) << "fatal error: ";
			break;
	}
	os << t.reset();

	// Print the actual message
	os << msg.msg << std::endl;

	// Print the error message context if available
	if (ctx.valid()) {
		size_t relPos = ctx.relPos;
		if (ctx.truncatedStart) {
			os << "[...] ";
		}
		os << ctx.text;
		if (ctx.truncatedEnd) {
			os << " [...]";
		}
		os << std::endl;

		if (ctx.truncatedStart) {
			os << "      ";
		}

		for (size_t i = 0; i < relPos; i++) {
			if (i < ctx.text.size() && ctx.text[i] == '\t') {
				os << '\t';
			} else {
				os << ' ';
			}
		}
		os << t.color(Terminal::GREEN) << '^' << t.reset() << std::endl;
	}
}
}

