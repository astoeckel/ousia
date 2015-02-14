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
 * @file ImportIncludeHandler.hpp
 *
 * Contains the conceptually similar handlers for the "include" and "import"
 * commands.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_IMPORT_INCLUDE_HANDLER_HPP_
#define _OUSIA_IMPORT_INCLUDE_HANDLER_HPP_

#include <core/common/Variant.hpp>
#include <core/parser/ParserStack.hpp>

namespace ousia {

/**
 * The ImportHandler is responsible for handling the "import" command. An import
 * creates a reference to a specified file. The specified file is parsed (if
 * this has not already been done) outside of the context of the current file.
 * If the specified resource has already been parsed, a reference to the already
 * parsed file is inserted. Imports are only possible before no other content
 * has been parsed.
 */
class ImportHandler : public StaticFieldHandler {
public:
	using StaticFieldHandler::StaticFieldHandler;

	void doHandle(const Variant &fieldData,
	                      const Variant::mapType &args) override;

	/**
	 * Creates a new instance of the ImportHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new ImportHandler{handlerData};
	}
};

/**
 * The IncludeHandler is responsible for handling the "include" command. The
 * included file is parsed in the context of the current file and will change
 * the content that is currently being parsed. Includes are possible at (almost)
 * any position in the source file.
 */
class IncludeHandler : public StaticFieldHandler {
public:
	using StaticFieldHandler::StaticFieldHandler;

	void doHandle(const Variant &fieldData,
	                      const Variant::mapType &args) override;

	/**
	 * Creates a new instance of the IncludeHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new IncludeHandler{handlerData};
	}
};
}
#endif
