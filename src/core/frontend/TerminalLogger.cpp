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

#include <core/common/Utils.hpp>

#include "Terminal.hpp"
#include "TerminalLogger.hpp"

namespace ousia {

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
	if (ctx.hasText()) {
		// Iterate over each line in the text
		std::vector<std::string> lines = Utils::split(ctx.text, '\n');

		const size_t relLen = ctx.relLen ? ctx.relLen : 1;
		const size_t relPos = ctx.relPos;
		const size_t pstart = relPos;
		const size_t pend = relPos + relLen;

		size_t lstart = 0;
		size_t lend = 0;
		for (size_t n = 0; n < lines.size(); n++) {
			bool firstLine = n == 0;
			bool lastLine = n == lines.size() - 1;

			// Indicate truncation and indent non-first lines
			if (ctx.truncatedStart && firstLine) {
				os << "[...] ";
			}
			if (!firstLine) {
				os << "\t";
			}

			// Print the actual line
			os << lines[n];

			// Indicate truncation
			if (ctx.truncatedEnd && lastLine) {
				os << " [...]";
			}
			os << std::endl;

			// Repeat truncation or indendation space in the next line
			if (ctx.truncatedStart && firstLine) {
				os << "      ";
			}
			if (!firstLine) {
				os << "\t";
			}

			// Print the position indicators
			lend = lastLine ? pend : lstart + lines[n].size();
			for (size_t i = lstart; i < lend; i++) {
				if (i >= pstart && i < pend) {
					os << t.color(Terminal::GREEN);
					for (; i < std::min(lend, pend); i++) {
						if (relLen == 1) {
							os << '^';
						} else {
							os << '~';
						}
						if (i < ctx.text.size() && ctx.text[i] == '\t') {
							os << '\t';
						}
					}
					os << t.reset();
				} else {
					if (i < ctx.text.size() && ctx.text[i] == '\t') {
						os << '\t';
					} else {
						os << ' ';
					}
				}
			}
			os << std::endl;

			lstart = lend;
		}
	}
}

}

