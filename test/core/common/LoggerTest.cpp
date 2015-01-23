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

static SourceContext contextCallback(const SourceLocation &location)
{
	SourceContext ctx;
	ctx.filename = "testfile.test";
	ctx.startLine = 10;
	ctx.endLine = 10;
	ctx.startColumn = 20;
	ctx.endColumn = 20;
	return ctx;
}

TEST(TerminalLogger, log)
{
	// Test for manual visual expection only -- no assertions
	TerminalLogger logger{std::cerr, true};
	logger.setSourceContextCallback(contextCallback);

	logger.debug("This is a test debug message");
	logger.note("This is a test note");
	logger.warning("This is a test warning");
	logger.error("This is a test error");
	logger.fatalError("This is a test fatal error!");

	logger.error("This is a test error with context");

	try {
		throw LoggableException{"An exception"};
	}
	catch (const LoggableException &ex) {
		logger.log(ex);
	}
}

TEST(TerminalLogger, fork)
{
	// Test for manual visual expection only -- no assertions
	TerminalLogger logger{std::cerr, true};

	logger.setSourceContextCallback(contextCallback);

	LoggerFork fork = logger.fork();

	fork.error("This is a test error with context");

	// Print all error messages
	fork.commit();
}
}

