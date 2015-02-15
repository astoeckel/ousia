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
 * Contains the Handler classes used for parsing Domain descriptors. This
 * includes the "domain" tag and all describing tags below the "domain" tag.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_DOMAIN_HANDLER_HPP_
#define _OUSIA_DOMAIN_HANDLER_HPP_

#include <core/common/Variant.hpp>
#include <core/model/Node.hpp>

#include "Handler.hpp"

namespace ousia {

// Forward declarations
class Rtti;

namespace parser_stack {

// TODO: Documentation

class DomainHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainHandler{handlerData};
	}
};

class DomainStructHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainStructHandler{handlerData};
	}
};

class DomainAnnotationHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainAnnotationHandler{handlerData};
	}
};

class DomainAttributesHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainAttributesHandler{handlerData};
	}
};

class DomainFieldHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainFieldHandler{handlerData};
	}
};

class DomainFieldRefHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainFieldRefHandler{handlerData};
	}
};

class DomainPrimitiveHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainPrimitiveHandler{handlerData};
	}
};

class DomainChildHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainChildHandler{handlerData};
	}
};

class DomainParent : public Node {
public:
	using Node::Node;
};

class DomainParentHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentHandler{handlerData};
	}
};

class DomainParentFieldHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentFieldHandler{handlerData};
	}
};

class DomainParentFieldRefHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new DomainParentFieldRefHandler{handlerData};
	}
};

namespace States {
/**
 * State representing a "domain" struct.
 */
extern const State Domain;

/**
 * State representing a "struct" tag within a domain description.
 */
extern const State DomainStruct;

/**
 * State representing an "annotation" tag within a domain description.
 */
extern const State DomainAnnotation;

/**
 * State representing an "attributes" tag within a structure or annotation.
 */
extern const State DomainAttributes;

/**
 * State representing an "attribute" tag within the "attributes".
 */
extern const State DomainAttribute;

/**
 * State representing a "field" tag within a structure or annotation.
 */
extern const State DomainField;

/**
 * State representing a "fieldref" tag within a structure or annotation.
 */
extern const State DomainFieldRef;

/**
 * State representing a "primitive" tag within a structure or annotation.
 */
extern const State DomainStructPrimitive;

/**
 * State representing a "child" tag within a structure or annotation.
 */
extern const State DomainStructChild;

/**
 * State representing a "parent" tag within a structure or annotation.
 */
extern const State DomainStructParent;

/**
 * State representing a "field" tag within a "parent" tag.
 */
extern const State DomainStructParentField;

/**
 * State representing a "fieldRef" tag within a "parent" tag.
 */
extern const State DomainStructParentFieldRef;
}
}

namespace RttiTypes {
extern const Rtti DomainParent;
}
}
#endif
