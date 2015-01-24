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
 * @file TerminalLogger.hpp
 *
 * Provides the TerminalLogger class, which is used to log messages to an output
 * stream, possibly a Terminal.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_TERMINAL_LOGGER_HPP_
#define _OUSIA_TERMINAL_LOGGER_HPP_

#include <ostream>

#include <core/common/Logger.hpp>

namespace ousia {

/**
 * Class extending the Logger class and printing (possibly colored) log messages
 * to the given stream.
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
	 * @param minSeverity is the minimum severity below which log messages
	 * are discarded.
	 */
	TerminalLogger(std::ostream &os, bool useColor = false,
	               Severity minSeverity = DEFAULT_MIN_SEVERITY)
	    : ConcreteLogger(minSeverity), os(os), useColor(useColor)
	{
	}
};

}

#endif /* _OUSIA_TERMINAL_LOGGER_HPP_ */


