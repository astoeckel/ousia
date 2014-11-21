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

#ifndef _OUSIA_LOGGER_HPP_
#define _OUSIA_LOGGER_HPP_

#include <ostream>
#include <string>
#include <vector>

namespace ousia {

enum class Severity : int {
	DEBUG = 0,
	INFO = 1,
	WARNING = 2,
	ERROR = 3,
	FATAL_ERROR = 4
};

struct LogMessage {
	Severity severity;
	std::string msg;
	std::string file;
	int line;
	int column;

	LogMessage(Severity severity, std::string msg, std::string file, int line,
	           int column)
	    : severity(severity),
	      msg(std::move(msg)),
	      file(std::move(file)),
	      line(line),
	      column(column){};
};

class Logger {
private:
	Severity maxLogSeverity = Severity::DEBUG;
	std::string curFile;

protected:
	virtual void logMessage(const LogMessage &msg){};

public:
	Logger(){};

	Logger(const Logger &) = delete;

	virtual ~Logger();

	void log(Severity severity, const std::string &msg, const std::string &file,
	         int line = -1, int column = -1)
	{
		// Copy the current severity level
		if (static_cast<int>(severity) > static_cast<int>(maxSeverity)) {
			maxSeverity = severity;
		}

		// Call the actual log message function
		logMessage(LogMessage{severity, msg, file, line, column});
	}

	void log(Severity severity, const std::string &msg, int line = -1,
	         int column = -1)
	{
		log(severity, msg, curFile, line, column);
	}

	void debug(const std::string &msg, int line = -1, int column = -1)
	{
		log(Severity::DEBUG, msg, line, column);
	}

	void info(const std::string &msg, int line = -1, int column = -1)
	{
		log(Severity::INFO, msg, line, column);
	}

	void warning(const std::string &msg, int line = -1, int column = -1)
	{
		log(Severity::WARNING, msg, line, column);
	}

	void error(const std::string &msg, int line = -1, int column = -1)
	{
		log(Severity::ERROR, msg, line, column);
	}

	void fatalError(const std::string &msg, int line = -1, int column = -1)
	{
		log(Severity::FATAL_ERROR, msg, line, column);
	}

	Severity getMaxSeverity() { return maxSeverity; }
};

class StreamLogger {
private:
	std::ostream &os;
	bool useColor;

protected:
	void logMessage(const LogMessage &msg) override;

public:
	StreamLogger(std::ostream &os, bool useColor = false)
	    : os(os), useColor(useColor)
	{
	}
};
}

#endif /* _OUSIA_LOGGER_HPP_ */

