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

#include <core/common/Logger.hpp>

namespace ousia {

struct Pos {
	SourceLocation pos;

	Pos(SourceLocation pos = SourceLocation{})
	    : pos(pos) {};

	SourceLocation getLocation() { return pos; }
};

static SourceContext contextCallback(const SourceLocation &location,
	                                     void *)
{
	return SourceContext{"int bla = blub;", 10, true, false};
}

TEST(TerminalLogger, log)
{
	// Test for manual visual expection only -- no assertions
	TerminalLogger logger{std::cerr, true};
	logger.pushFile("test.odp");

	logger.debug("This is a test debug message", SourceLocation{10, 20});
	logger.debug("This is a test debug message with no column",
	             SourceLocation{10});
	logger.debug("This is a test debug message with no line");
	logger.note("This is a test note", SourceLocation{10, 20});
	logger.warning("This is a test warning", SourceLocation{10, 20});
	logger.error("This is a test error", SourceLocation{10, 20});
	logger.fatalError("This is a test fatal error!", SourceLocation{10, 20});

	logger.pushFile("test2.odp", SourceLocation{}, contextCallback);
	logger.error("This is a test error with context", SourceLocation{10, 20});
	logger.popFile();

	Pos pos(SourceLocation{10, 20});

	try {
		throw LoggableException{"An exception"};
	}
	catch (const LoggableException &ex) {
		logger.log(ex);
	}

	try {
		throw LoggableException{"An exception at position", pos};
	}
	catch (const LoggableException &ex) {
		logger.log(ex);
	}

	logger.log(Severity::ERROR, "This is a positioned log message", pos);
}

TEST(TerminalLogger, fork)
{
	// Test for manual visual expection only -- no assertions
	TerminalLogger logger{std::cerr, true};

	LoggerFork fork = logger.fork();

	fork.pushFile("test.odp", SourceLocation{}, contextCallback);
	fork.error("This is a test error with context", SourceLocation{10, 20});
	fork.pushFile("test2.odp");
	fork.error("This is a test error without context");
	fork.popFile();
	fork.error("Another error");
	fork.popFile();
	fork.error("Another error");

	// Print all error messages
	fork.commit();
}
}

