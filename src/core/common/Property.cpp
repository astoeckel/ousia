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

#include "Exceptions.hpp"
#include "Logger.hpp"
#include "Property.hpp"
#include "VariantConverter.hpp"

namespace ousia {

/* Class PropertyType */

const PropertyType PropertyType::None;

/* Class GetterFunction */

void GetterFunction::validateArguments(Variant::arrayType &args) const
{
	if (!args.empty()) {
		throw PropertyException(
		    std::string("Getter function has no arguments, but got ") +
		    std::to_string(args.size()));
	}
}

void GetterFunction::validateResult(Variant &res) const
{
	ExceptionLogger logger;
	if (propertyType != nullptr) {
		VariantConverter::convert(res, propertyType->type,
		                          propertyType->innerType, logger);
	}
}

Variant GetterFunction::get(void *obj)
{
	return call(Variant::arrayType{}, obj);
}

/* Class SetterFunction */

void SetterFunction::validateArguments(Variant::arrayType &args) const
{
	// Make sure exactly one argument is given
	if (args.size() != 1U) {
		throw PropertyException(
		    std::string(
		        "Expected exactly one argument to be passed to the property "
		        "setter, but got ") +
		    std::to_string(args.size()));
	}

	// Convert the one argument to the requested type, throw an exception if
	// this fails.
	ExceptionLogger logger;
	if (propertyType != nullptr) {
		VariantConverter::convert(args[0], propertyType->type,
		                          propertyType->innerType, logger);
	}
}

void SetterFunction::set(const Variant &value, void *obj)
{
	call(Variant::arrayType{value}, obj);
}

/* Class PropertyDescriptor */

PropertyDescriptor::PropertyDescriptor(const PropertyType &type,
                                       std::shared_ptr<GetterFunction> getter,
                                       std::shared_ptr<SetterFunction> setter)
    : type(std::make_shared<PropertyType>(type)), getter(getter), setter(setter)
{
	if (!this->getter->isValid()) {
		throw PropertyException(
		    "Getter must be valid, writeonly properties are not "
		    "supported!");
	}

	// Assign the property type reference to the getter and setter
	this->getter->propertyType = this->type;
	this->setter->propertyType = this->type;
}
}

