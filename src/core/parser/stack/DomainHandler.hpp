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
 * @file DomainHandler.hpp
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_DOMAIN_HANDLER_HPP_
#define _OUSIA_DOMAIN_HANDLER_HPP_

#include <core/common/Variant.hpp>
#include <core/parser/ParserStack.hpp>

namespace ousia {

// Forward declarations
class Rtti;

class DomainHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainHandler{handlerData};
	}
};

class DomainStructHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainStructHandler{handlerData};
	}
};

class DomainAnnotationHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainAnnotationHandler{handlerData};
	}
};

class DomainAttributesHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainAttributesHandler{handlerData};
	}
};

class DomainFieldHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainFieldHandler{handlerData};
	}
};

class DomainFieldRefHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainFieldRefHandler{handlerData};
	}
};

class DomainPrimitiveHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainPrimitiveHandler{handlerData};
	}
};

class DomainChildHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainChildHandler{handlerData};
	}
};

class DomainParent : public Node {
public:
	using Node::Node;
};

namespace RttiTypes {
extern const Rtti DomainParent;
}

class DomainParentHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentHandler{handlerData};
	}
};

class DomainParentFieldHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentFieldHandler{handlerData};
	}
};

class DomainParentFieldRefHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentFieldRefHandler{handlerData};
	}
};
}
#endif
