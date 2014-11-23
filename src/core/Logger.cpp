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
                 const std::string &file, int line, int column)
{
	// Copy the current severity level
	if (static_cast<int>(severity) > static_cast<int>(maxEncounteredSeverity)) {
		maxEncounteredSeverity = severity;
	}

	// Call the actual log message function if the severity is larger or equal
	// to the minimum severity
	if (static_cast<int>(severity) >= static_cast<int>(minSeverity)) {
		process(Message{severity, msg, file, line, column});
	}
}

unsigned int Logger::pushFilename(const std::string &name)
{
	filenameStack.push(name);
	return filenameStack.size();
}

unsigned int Logger::popFilename()
{
	filenameStack.pop();
	return filenameStack.size();
}

void Logger::unwindFilenameStack(unsigned int pos)
{
	while (filenameStack.size() > pos && !filenameStack.empty()) {
		filenameStack.pop();
	}
}

/* Class TerminalLogger */

/**
 * Small class used internally for formated terminal output using ANSI/VT100
 * escape codes on supported terminals.
 *
 * TODO: Deactivate if using windows or use the corresponding API function.
 */
class Terminal {
private:
	/**
	 * If set to false, no control codes are generated.
	 */
	bool active;

public:
	static const int BLACK = 30;
	static const int RED = 31;
	static const int GREEN = 32;
	static const int YELLOW = 33;
	static const int BLUE = 34;
	static const int MAGENTA = 35;
	static const int CYAN = 36;
	static const int WHITE = 37;

	Terminal(bool active) : active(active) {}

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

	std::string reset() const
	{
		if (!active) {
			return std::string{};
		}
		return "\x1b[0m";
	}
};

void TerminalLogger::process(const Message &msg)
{
	Terminal t(useColor);

	// Print the file name
	if (msg.hasFile()) {
		os << t.color(Terminal::WHITE, true) << msg.file << t.reset();
	}

	// Print line and column number
	if (msg.hasLine()) {
		if (msg.hasFile()) {
			os << ':';
		}
		os << t.color(Terminal::WHITE, true) << msg.line
		   << t.reset();
		if (msg.hasColumn()) {
			os << ':' << msg.column;
		}
	}

	// Print the optional seperator
	if (msg.hasFile() || msg.hasLine()) {
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
			os << t.color(Terminal::RED, true) << "error: ";
			break;
	}
	os << t.reset();

	// Print the actual message
	os << msg.msg << std::endl;
}
}

