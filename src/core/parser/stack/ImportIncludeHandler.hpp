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
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_IMPORT_INCLUDE_HANDLER_HPP_
#define _OUSIA_IMPORT_INCLUDE_HANDLER_HPP_

#include <core/common/Variant.hpp>
#include <core/parser/ParserStack.hpp>

namespace ousia {

class ImportIncludeHandler : public Handler {
protected:
	bool srcInArgs = false;
	std::string rel;
	std::string type;
	std::string src;

public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void data(const std::string &data, int field) override;
};

class ImportHandler : public ImportIncludeHandler {
public:
	using ImportIncludeHandler::ImportIncludeHandler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new ImportHandler{handlerData};
	}
};

class IncludeHandler : public ImportIncludeHandler {
public:
	using ImportIncludeHandler::ImportIncludeHandler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new IncludeHandler{handlerData};
	}
};
}
#endif
