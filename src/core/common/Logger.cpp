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

void Logger::log(Severity severity, std::string msg, TextCursor::Position pos,
                 TextCursor::Context ctx)
{
	// Update the maximum encountered severity level
	if (static_cast<int>(severity) > static_cast<int>(maxEncounteredSeverity)) {
		maxEncounteredSeverity = severity;
	}

	// Only process the message if its severity is larger than the
	// set minimum severity.
	if (static_cast<int>(severity) >= static_cast<int>(minSeverity)) {
		processMessage(
		    Message{severity, std::move(msg), std::move(pos), std::move(ctx)});
	}
}

LoggerFork Logger::fork() { return LoggerFork{this, minSeverity}; }

/* Class LoggerFork */

void LoggerFork::processMessage(Message msg)
{
	calls.push_back(Call(CallType::MESSAGE, messages.size()));
	messages.push_back(msg);
}

void LoggerFork::processPushFile(File file)
{
	calls.push_back(Call(CallType::PUSH_FILE, files.size()));
	files.push_back(file);
}

void LoggerFork::processPopFile()
{
	calls.push_back(Call(CallType::POP_FILE, 0));
}

void LoggerFork::commit()
{
	for (const Call &call : calls) {
		switch (call.type) {
			case CallType::MESSAGE: {
				const Message &msg = messages[call.dataIdx];
				parent->log(msg.severity, msg.msg, msg.pos, msg.ctx);
				break;
			}
			case CallType::PUSH_FILE: {
				const File &file = files[call.dataIdx];
				parent->pushFile(file.file, file.pos, file.ctx);
				break;
			}
			case CallType::POP_FILE:
				parent->popFile();
				break;
		}
	}
}

/* Class Terminal */

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

/* Class TerminalLogger */

/**
 * Small class used internally for formated terminal output using ANSI/VT100
 * escape codes on supported terminals.
 *
 * TODO: Deactivate if using windows or use the corresponding API function.
 */

std::string TerminalLogger::currentFilename()
{
	if (!files.empty()) {
		return files.top().file;
	}
	return std::string{};
}

void TerminalLogger::processMessage(Message msg)
{
	Terminal t(useColor);

	// Print the file name
	std::string filename = currentFilename();
	bool hasFile = !filename.empty();
	if (hasFile) {
		os << t.color(Terminal::WHITE, true) << filename << t.reset();
	}

	// Print line and column number
	if (msg.pos.hasLine()) {
		if (hasFile) {
			os << ':';
		}
		os << t.color(Terminal::WHITE, true) << msg.pos.line << t.reset();
		if (msg.pos.hasColumn()) {
			os << ':' << msg.pos.column;
		}
	}

	// Print the optional seperator
	if (hasFile || msg.pos.hasLine()) {
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
			os << t.color(Terminal::RED, true) << "fatal: ";
			break;
	}
	os << t.reset();

	// Print the actual message
	os << msg.msg << std::endl;

	// Print the error message context if available
	if (msg.ctx.valid()) {
		size_t relPos = msg.ctx.relPos;
		if (msg.ctx.truncatedStart) {
			os << "[...] ";
			relPos += 6;
		}
		os << msg.ctx.text;
		if (msg.ctx.truncatedEnd) {
			os << " [...]";
		}
		os << std::endl;
		for (size_t i = 0; i < relPos; i++) {
			os << ' ';
		}
		os << t.color(Terminal::GREEN) << '^' << t.reset() << std::endl;
	}
}

void TerminalLogger::processPushFile(File file) { files.push(file); }

void TerminalLogger::processPopFile() { files.pop(); }
}

