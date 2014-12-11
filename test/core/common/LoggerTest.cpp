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
	TextCursor::Position pos;
	TextCursor::Context ctx;

	Pos(TextCursor::Position pos = TextCursor::Position{},
	    TextCursor::Context ctx = TextCursor::Context{})
	    : pos(pos), ctx(ctx){};

	TextCursor::Position getPosition() { return pos; }
	TextCursor::Context getContext() { return ctx; }
};

TEST(TerminalLogger, log)
{
	// Test for manual visual expection only -- no assertions
	TerminalLogger logger{std::cerr, true};
	logger.pushFile("test.odp");

	logger.debug("This is a test debug message", TextCursor::Position{10, 20});
	logger.debug("This is a test debug message with no column",
	             TextCursor::Position{10});
	logger.debug("This is a test debug message with no line");
	logger.note("This is a test note", TextCursor::Position{10, 20});
	logger.warning("This is a test warning", TextCursor::Position{10, 20});
	logger.error("This is a test error", TextCursor::Position{10, 20});
	logger.fatalError("This is a test fatal error!",
	                  TextCursor::Position{10, 20});

	logger.error("This is a test error with context",
	             TextCursor::Position{10, 20},
	             TextCursor::Context{"int bla = blub;", 10, true, false});

	Pos pos(TextCursor::Position{10, 20});

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

	logger.logAt(Severity::ERROR, "This is a positioned log message", pos);
}
}

