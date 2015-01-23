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
#include "Terminal.hpp"

namespace ousia {

/* Class Logger */

void Logger::log(Severity severity, const std::string &msg,
                 const SourceLocation &loc, MessageMode mode)
{
	// Assemble the message and pass it through the filter, then process it
	Message message{severity, mode, std::move(msg), loc};
	if (filterMessage(message)) {
		processMessage(message);
	}
}

LoggerFork Logger::fork() { return LoggerFork(this); }

/* Class LoggerFork */

void LoggerFork::processMessage(const Message &msg)
{
	calls.emplace_back(CallType::MESSAGE, messages.size());
	messages.push_back(msg);
}

void LoggerFork::processPushDefaultLocation(const SourceLocation &loc)
{
	calls.emplace_back(CallType::PUSH_LOCATION, locations.size());
	locations.push_back(loc);
}

void LoggerFork::processPopDefaultLocation()
{
	calls.emplace_back(CallType::POP_LOCATION, 0);
}

void LoggerFork::processSetDefaultLocation(const SourceLocation &loc)
{
	// Check whether setDefaultLocation was called immediately before, if yes,
	// simply override the data
	if (!calls.empty() && calls.back().type == CallType::SET_LOCATION) {
		locations.back() = loc;
	} else {
		calls.emplace_back(CallType::SET_LOCATION, locations.size());
		locations.emplace_back(loc);
	}
}

void LoggerFork::processSetSourceContextCallback(
    SourceContextCallback sourceContextCallback)
{
	// Check whether setSourceContextCallback was called immediately before,
	// if yes, simply override the data
	if (!calls.empty() && calls.back().type == CallType::SET_CONTEXT_CALLBACK) {
		callbacks.back() = sourceContextCallback;
	} else {
		calls.emplace_back(CallType::SET_CONTEXT_CALLBACK, callbacks.size());
		callbacks.emplace_back(sourceContextCallback);
	}
}

void LoggerFork::purge()
{
	calls.clear();
	messages.clear();
	locations.clear();
	callbacks.clear();
}

void LoggerFork::commit()
{
	for (const Call &call : calls) {
		switch (call.type) {
			case CallType::MESSAGE:
				if (parent->filterMessage(messages[call.dataIdx])) {
					parent->processMessage(messages[call.dataIdx]);
				}
				break;
			case CallType::PUSH_LOCATION:
				parent->processPushDefaultLocation(locations[call.dataIdx]);
				break;
			case CallType::POP_LOCATION:
				parent->processPopDefaultLocation();
				break;
			case CallType::SET_LOCATION:
				parent->processSetDefaultLocation(locations[call.dataIdx]);
				break;
			case CallType::SET_CONTEXT_CALLBACK:
				parent->processSetSourceContextCallback(
				    callbacks[call.dataIdx]);
				break;
		}
	}
	purge();
}

/* Class ScopedLogger */

ScopedLogger::ScopedLogger(Logger &parent, SourceLocation loc)
    : parent(parent), depth(0)
{
	pushDefaultLocation(loc);
}

ScopedLogger::~ScopedLogger()
{
	while (depth > 0) {
		popDefaultLocation();
	}
}

void ScopedLogger::processMessage(const Message &msg)
{
	parent.processMessage(msg);
}

bool ScopedLogger::filterMessage(const Message &msg)
{
	return parent.filterMessage(msg);
}

void ScopedLogger::processPushDefaultLocation(const SourceLocation &loc)
{
	parent.processPushDefaultLocation(loc);
	depth++;
}

void ScopedLogger::processPopDefaultLocation()
{
	depth--;
	parent.processPopDefaultLocation();
}

void ScopedLogger::processSetDefaultLocation(const SourceLocation &loc)
{
	parent.processSetDefaultLocation(loc);
}

void ScopedLogger::processSetSourceContextCallback(
    SourceContextCallback sourceContextCallback)
{
	parent.processSetSourceContextCallback(sourceContextCallback);
}

/* Class ConcreteLogger */

ConcreteLogger::ConcreteLogger(Severity minSeverity)
    : minSeverity(minSeverity), sourceContextCallback(NullSourceContextCallback)
{
}

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

void ConcreteLogger::processPushDefaultLocation(const SourceLocation &loc)
{
	locations.emplace_back(loc);
}

void ConcreteLogger::processPopDefaultLocation()
{
	if (!locations.empty()) {
		locations.pop_back();
	}
}

void ConcreteLogger::processSetDefaultLocation(const SourceLocation &loc)
{
	if (!locations.empty()) {
		locations.back() = loc;
	} else {
		locations.emplace_back(loc);
	}
}

void ConcreteLogger::processSetSourceContextCallback(
    SourceContextCallback sourceContextCallback)
{
	this->sourceContextCallback = sourceContextCallback;
}

const SourceLocation &ConcreteLogger::messageLocation(const Message &msg) const
{
	if (msg.loc.isValid()) {
		return msg.loc;
	} else if (!locations.empty()) {
		return locations.back();
	}
	return NullSourceLocation;
}

SourceContext ConcreteLogger::messageContext(const Message &msg) const
{
	return sourceContextCallback(messageLocation(msg));
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
	locations.clear();
	messageCounts.clear();
	sourceContextCallback = NullSourceContextCallback;
}

bool ConcreteLogger::hasError()
{
	return getSeverityCount(Severity::ERROR) > 0 ||
	       getSeverityCount(Severity::FATAL_ERROR) > 0;
}

bool ConcreteLogger::hasFatalError()
{
	return getSeverityCount(Severity::FATAL_ERROR) > 0;
}

/* Class TerminalLogger */

void TerminalLogger::processMessage(const Message &msg)
{
	Terminal t(useColor);

	// Fetch filename, position and context
	const SourceContext ctx = messageContext(msg);

	// Print the file name
	if (ctx.hasFile()) {
		os << t.bright() << ctx.filename << t.reset();
	}

	// Print line and column number
	if (ctx.hasLine()) {
		if (ctx.hasFile()) {
			os << ':';
		}
		os << t.bright() << ctx.startLine << t.reset();
		if (ctx.hasColumn()) {
			os << ':' << ctx.startColumn;
		}
	}

	// Print the optional seperator
	if (ctx.hasFile() || ctx.hasLine()) {
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
	/*	if (ctx.valid()) {
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
	    }*/
}
}

