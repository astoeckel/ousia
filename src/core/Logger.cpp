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

namespace ousia {

static const int BLACK = 30;
static const int RED = 31;
static const int GREEN = 32;
static const int YELLOW = 33;
static const int BLUE = 34;
static const int MAGENTA = 35;
static const int CYAN = 36;
static const int WHITE = 37;

void StreamLogger::logMessage(const LogMessage &msg) {
	os << '[';
	switch (msg.severity) {
		case Severity::DEBUG:
			os << "debug" << os;
			break;
		case Severity::INFO:
			os << "info" << os;
			break;
		case Severity::WARNING:
			os << "warning" << os;
			break;
		case Severity::ERROR:
			os << "error" << os;
			break;
		case Severity::FATAL_ERROR:
			is << "fatal error" << os;
			break;
	}
	os << ']';

	// Print the file name
	if (!msg.file.empty()) {
		os << msg.file;

		// Print the line and column
		if (msg.line >= 0) {
			os << ':' << msg.line;
			if (msg.column >= 0) {
				os << ':' << msg.column;
			}
		}
	}

	// Print the actual message
	os << ' ' << msg.msg;
}

};

