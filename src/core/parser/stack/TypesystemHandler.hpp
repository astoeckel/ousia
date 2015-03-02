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

/**
 * @file TypesystemHandler.hpp
 *
 * Contains the Handler classes used to parse Typesystem descriptions. The
 * Handlers parse all the tags found below and including the "typesystem" tag.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_TYPESYSTEM_HANDLER_HPP_
#define _OUSIA_TYPESYSTEM_HANDLER_HPP_

#include <core/common/Variant.hpp>

#include "Handler.hpp"

namespace ousia {
namespace parser_stack {

/**
 * Handles the occurance of the "typesystem" tag. Creates a new Typesystem
 * instance and places it on the ParserScope.
 */
class TypesystemHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	/**
	 * Creates a new instance of the TypesystemHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemHandler{handlerData};
	}
};

/**
 * Handles the occurance of the "enum" tag. Creates a new EnumType instance and
 * places it on the ParserScope.
 */
class TypesystemEnumHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	/**
	 * Creates a new instance of the TypesystemEnumHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemEnumHandler{handlerData};
	}
};

/**
 * Handles the occurance of the "entry" tag within an "enum" tag. Creates a new
 * EnumType instance and places it on the ParserScope.
 */
class TypesystemEnumEntryHandler : public StaticFieldHandler {
public:
	using StaticFieldHandler::StaticFieldHandler;

	void doHandle(const Variant &fieldData, Variant::mapType &args) override;

	/**
	 * Creates a new instance of the TypesystemEnumEntryHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemEnumEntryHandler{handlerData, "name"};
	}
};

/**
 * Handles the occurance of the "struct" tag within a typesystem description.
 * Creates a new StructType instance and places it on the ParserScope.
 */
class TypesystemStructHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	/**
	 * Creates a new instance of the TypesystemStructHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemStructHandler{handlerData};
	}
};

/**
 * Handles the occurance of the "field" tag within a typesystem structure
 * description. Places a new Attribute instance in the StructType instance
 * that is currently at the top of the scope.
 */
class TypesystemStructFieldHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;

	/**
	 * Creates a new instance of the TypesystemStructFieldHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemStructFieldHandler{handlerData};
	}
};

/**
 * Handles the occurance of the "constant" tag within a typesystem structure
 * description. Places a new Constant instance in the current typesystem.
 */
class TypesystemConstantHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;

	/**
	 * Creates a new instance of the TypesystemConstantHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new TypesystemConstantHandler{handlerData};
	}
};

namespace States {
/**
 * State representing the "typesystem" tag.
 */
extern const State Typesystem;
/**
 * State representing the "enum" tag within a typesystem.
 */
extern const State TypesystemEnum;
/**
 * State representing the "entry" tag within an enum.
 */
extern const State TypesystemEnumEntry;
/**
 * State representing the "struct" tag within a typesystem.
 */
extern const State TypesystemStruct;
/**
 * State representing the "field" tag within a typesystem structure.
 */
extern const State TypesystemStructField;
/**
 * State representing the "constant" tag within a typesystem.
 */
extern const State TypesystemConstant;
}
}
}
#endif
