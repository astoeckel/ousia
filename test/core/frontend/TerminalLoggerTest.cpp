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

#include <core/common/CharReader.hpp>
#include <core/common/SourceContextReader.hpp>
#include <core/frontend/TerminalLogger.hpp>

namespace ousia {

struct Pos {
	SourceLocation pos;

	Pos(SourceLocation pos = SourceLocation{}) : pos(pos){};

	SourceLocation getLocation() { return pos; }
};

static const std::string testStr =
    "\\link[ontology]{book}\n"                             // 1
    "\\link[ontology]{meta}\n"                             // 2
    "\n"                                                   // 3
    "\\meta{\n"                                            // 4
    "\t\\title{The Adventures Of Tom Sawyer}\n"            // 5
    "\t\\author{Mark Twain}\n"                             // 6
    "}\n"                                                  // 7
    "\n"                                                   // 8
    "\\book{\n"                                            // 9
    "\n"                                                   // 10
    "\n"                                                   // 11
    "\\chapter\n"                                          // 12
    "<<TOM!>>\n"                                           // 13
    "\n"                                                   // 14
    "No answer.\n"                                         // 15
    "\n"                                                   // 16
    "<<TOM!>>\n"                                           // 17
    "\n"                                                   // 18
    "No answer.\n"                                         // 19
    "\n"                                                   // 20
    "<<What's gone with that boy, I wonder? You TOM!>>\n"  // 21
    "}\n";                                                 // 22

static SourceContextReader contextReader{};

static SourceContext contextCallback(const SourceLocation &location)
{
	CharReader reader{testStr, 0};
	return contextReader.readContext(reader, location,
	                                 "the_adventures_of_tom_sawyer.opd");
}

static SourceContext truncatedContextCallback(const SourceLocation &location)
{
	CharReader reader{testStr, 0};
	return contextReader.readContext(reader, location, 60,
	                                 "the_adventures_of_tom_sawyer.opd");
}

TEST(TerminalLogger, log)
{
	// Test for manual visual expection only -- no assertions
	TerminalLogger logger{std::cerr, true};
	logger.setSourceContextCallback(contextCallback);

	logger.debug("This is a test debug message");
	logger.note("This is a test note");
	logger.note("This is a test note with point context",
	            SourceLocation{0, 49});
	logger.note("This is a test note with range context",
	            SourceLocation{0, 49, 55});
	logger.note("This is a test note with multiline context",
	            SourceLocation{0, 49, 150});

	logger.setSourceContextCallback(truncatedContextCallback);
	logger.note("This is a test note with truncated multiline context",
	            SourceLocation{0, 49, 150});
	logger.setSourceContextCallback(contextCallback);

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

	try {
		throw LoggableException{"An exception with context",
		                        SourceLocation{0, 41, 46}};
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

	fork.error("This is a test error without");

	fork.error("This is a test error with context", SourceLocation{0, 6, 12});

	// Print all error messages
	fork.commit();
}
}
