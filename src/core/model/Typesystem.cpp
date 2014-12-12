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

#include "Typesystem.hpp"

#include <core/common/Utils.hpp>

namespace ousia {
namespace model {

EnumerationType EnumerationType::createValidated(
    Manager &mgr, std::string name, Handle<Typesystem> system,
    const std::vector<std::string> &values, Logger &logger)
{
	std::map<std::string, size_t> unique_values;
	for (size_t i = 0; i < values.size(); i++) {
		if (!Utils::isIdentifier(values[i])) {
			logger.error(values[i] + " is no valid identifier.");
		}

		if (!(unique_values.insert(std::make_pair(values[i], i))).second) {
			logger.error(std::string("The value ") + values[i] +
			             " was duplicated.");
		}
	}
	return std::move(EnumerationType(mgr, name, system, unique_values));
}
}
}

