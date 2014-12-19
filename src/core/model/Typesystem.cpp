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

/* Class Type */

bool Type::build(Variant &var, Logger &logger) const
{
	try {
		return doBuild(var, logger);
	}
	catch (LoggableException ex) {
		logger.log(ex);
		var = create();
		return false;
	}
}

/* Class StringType */

bool StringType::doBuild(Variant &var, Logger &logger) const
{
	if (!var.isPrimitive()) {
		throw LoggableException{"Expected a string or primitive input."};
	}

	if (!var.isString()) {
		logger.note(std::string("Implicit type conversion from ") +
		         var.getTypeName() + " to string.");
	}
	var = Variant{var.toString().c_str()};
	return true;
}

/* Class EnumType */

EnumType EnumType::createValidated(Manager &mgr, std::string name,
                                   Handle<Typesystem> system,
                                   const std::vector<std::string> &values,
                                   Logger &logger)
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
	return std::move(EnumType(mgr, name, system, unique_values));
}

/* RTTI type registrations */

const ManagedType Type_T("Type", typeid(Type));
const ManagedType StringType_T("StringType", typeid(StringType), {&Type_T});
const ManagedType IntType_T("IntType", typeid(IntType), {&Type_T});
const ManagedType DoubleType_T("DoubleType", typeid(DoubleType), {&Type_T});
const ManagedType BoolType_T("BoolType", typeid(BoolType), {&Type_T});
const ManagedType EnumType_T("EnumType", typeid(EnumType), {&Type_T});
const ManagedType StructType_T("StructType", typeid(EnumType), {&Type_T});
}
}

