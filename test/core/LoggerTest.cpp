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

#include <gtest/gtest.h>

#include <core/Logger.hpp>

namespace ousia {

struct Pos {
	int line, column;
	Pos(int line, int column) : line(line), column(column){};
	int getLine() const { return line; }
	int getColumn() const { return column; }
};

TEST(TerminalLogger, log)
{
	// Test for manual visual expection only -- no assertions
	TerminalLogger logger{std::cerr, true};
	logger.pushFilename("test.odp");

	logger.debug("This is a test debug message", 10, 20);
	logger.debug("This is a test debug message with no column", 10);
	logger.debug("This is a test debug message with no line");
	logger.debug("This is a test debug message with no file", "");
	logger.debug("This is a test debug message with no file but a line", "",
	             10);
	logger.debug(
	    "This is a test debug message with no file but a line and a column", "",
	    10, 20);
	logger.note("This is a test note", 10, 20);
	logger.warning("This is a test warning", 10, 20);
	logger.error("This is a test error", 10, 20);
	logger.fatalError("This is a test fatal error!", 10, 20);

	try {
		throw LoggableException{"A fatal exception", true};
	}
	catch (const LoggableException &ex) {
		logger.log(ex);
	}

	try {
		throw LoggableException{"A fatal exception at position", true, Pos(10, 20)};
	}
	catch (const LoggableException &ex) {
		logger.log(ex);
	}

	logger.logAt(Severity::ERROR, "This is a positioned log message",
	             Pos(10, 20));
	logger.debugAt("This is a positioned debug message", Pos(10, 20));
	logger.noteAt("This is a positioned log error", Pos(10, 20));
}
}

