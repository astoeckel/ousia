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
 * @file DocumentHandler.hpp
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_DOCUMENT_HANDLER_HPP_
#define _OUSIA_DOCUMENT_HANDLER_HPP_

#include <core/common/Variant.hpp>

#include "Handler.hpp"

namespace ousia {

// Forward declarations
class Rtti;
class DocumentEntity;
class FieldDescriptor;

class DocumentHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DocumentHandler{handlerData};
	}
};

class DocumentField : public Node {
public:
	using Node::Node;
};

class DocumentChildHandler : public StaticHandler {
private:
	void preamble(Handle<Node> parentNode, std::string &fieldName,
	              DocumentEntity *&parent, bool &inField);

	void createPath(const NodeVector<Node> &path, DocumentEntity *&parent);

	std::pair<bool, Variant> convertData(Handle<FieldDescriptor> field,
	                                     Logger &logger,
	                                     const std::string &data);

public:
	using Handler::Handler;

	bool start(Variant::mapType &args) override;

	void end() override;

	bool data(const Variant &data) override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DocumentChildHandler{handlerData};
	}
};

namespace RttiTypes {
extern const Rtti DocumentField;
}
}
#endif
