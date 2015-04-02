/*
    Ousía
    Copyright (C) 2015  Benjamin Paaßen, Andreas Stöckel

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

#include <iomanip>

#include "TestLogger.hpp"

namespace ousia {
namespace test {

Logger::Logger(std::ostream &os, bool useColor)
    : os(os), terminal(useColor), first(true)
{
}

void Logger::fail(const std::string &msg)
{
	os << terminal.color(Terminal::RED, true) << "[Fail]" << terminal.reset()
	   << " " << msg << std::endl;
	first = false;
}

void Logger::success(const std::string &msg)
{
	os << terminal.color(Terminal::GREEN, true) << "[Success]"
	   << terminal.reset() << " " << msg << std::endl;
	first = false;
}

void Logger::note(const std::string &msg)
{
	os << terminal.color(Terminal::BLUE, true) << "[Note]" << terminal.reset()
	   << " " << msg << std::endl;
	first = false;
}

void Logger::result(std::istream &is, const std::set<int> &errLines)
{
	std::string line;
	is.clear();
	is.seekg(0);
	size_t lineNumber = 0;
	while (std::getline(is, line)) {
		lineNumber++;
		const bool hasErr = errLines.count(lineNumber);
		if (hasErr) {
			os << terminal.background(Terminal::RED);
		}
		os << terminal.color(Terminal::BLACK, !hasErr) << (hasErr ? "!" : " ")
		   << std::setw(3) << lineNumber << ":" << terminal.reset() << " "
		   << line << std::endl;
	}
	first = false;
}

void Logger::headline(const std::string &msg)
{
	if (!first) {
		os << std::endl;
	}
	os << "== " << terminal.bright() << msg << terminal.reset()
	   << " ==" << std::endl;
	first = false;
}
}
}
