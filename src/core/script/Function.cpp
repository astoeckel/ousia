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

#include "Function.hpp"

namespace ousia {
namespace script {

std::pair<bool, std::vector<Variant>> ArgumentValidator::setError(int idx,
		const std::string &msg, std::vector<Variant> &res)
{
	errorIndex = idx;
	errorMessage = msg;
	return std::make_pair(false, res);
}

void ArgumentValidator::resetError()
{
	errorIndex = -1;
	errorMessage = "";
}

std::pair<bool, std::vector<Variant>> ArgumentValidator::validate(
		const std::vector<Variant> &args)
{
	std::vector<Variant> res;

	// Reset any old error
	resetError();

	// Sanity check: Do not allow too many arguments
	if (args.size() > descriptors.size()) {
		return setError(descriptors.size(), "Expected " + std::to_string(descriptors.size()) +
				" arguments but got " + std::to_string(args.size()), res);
	}

	// Iterate over the given arguments and check their type
	res.reserve(descriptors.size());
	for (unsigned int i = 0; i < args.size(); i++) {
		// TODO: Implicit type conversion
		const VariantType tGiven = args[i].getType();
		const VariantType tExpected = descriptors[i].type;
		if (tGiven != tExpected) {
			return setError(i, std::string("Expected type ") + Variant::getTypeName(tExpected)
				+ " but got " + Variant::getTypeName(tGiven), res);
		}
		res.push_back(args[i]);
	}

	// Make sure the remaining arguments have a default value, and if they have
	// one, add it to the result
	for (unsigned int i = args.size(); i < descriptors.size(); i++) {
		if (!descriptors[i].hasDefault) {
			return setError(i, "Expected argument " + std::to_string(i), res);
		}
		res.push_back(descriptors[i].defaultValue);
	}

	return std::make_pair(true, res);
}

}
}

