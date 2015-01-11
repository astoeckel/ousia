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

#include "Scope.hpp"

namespace ousia {
namespace parser {

Rooted<Node> Scope::resolve(const std::vector<std::string> &path,
                            const RttiType &type, Logger &logger)
{
	// Go up the stack and try to resolve the
	for (auto it = nodes.rbegin(); it != nodes.rend(); it++) {
		std::vector<ResolutionResult> res = (*it)->resolve(path, type);

		// Abort if the object could not be resolved
		if (res.empty()) {
			continue;
		}

		// Log an error if the object is not unique
		if (res.size() > 1) {
			logger.error(std::string("The reference ") +
			             Utils::join(path, ".") + (" is ambigous!"));
			logger.note("Referenced objects are:");
			for (const ResolutionResult &r : res) {
				logger.note(std::string("\t") + Utils::join(r.path(), "."));
			}
		}
		return res[0].node;
	}
	return nullptr;
}
}
}
