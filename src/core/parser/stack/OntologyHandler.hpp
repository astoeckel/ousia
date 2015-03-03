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
 * @file OntologyHandler.hpp
 *
 * Contains the Handler classes used for parsing Ontology descriptors. This
 * includes the "ontology" tag and all describing tags below the "ontology" tag.
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

class OntologyHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyHandler{handlerData};
	}
};

class OntologyStructHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyStructHandler{handlerData};
	}
};

class OntologyAnnotationHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyAnnotationHandler{handlerData};
	}
};

class OntologyAttributesHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyAttributesHandler{handlerData};
	}
};

class OntologyFieldHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyFieldHandler{handlerData};
	}
};

class OntologyFieldRefHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyFieldRefHandler{handlerData};
	}
};

class OntologyPrimitiveHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyPrimitiveHandler{handlerData};
	}
};

class OntologyChildHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyChildHandler{handlerData};
	}
};

class OntologyParent : public Node {
public:
	using Node::Node;
};

class OntologyParentHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyParentHandler{handlerData};
	}
};

class OntologyParentFieldHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyParentFieldHandler{handlerData};
	}
};

class OntologyParentFieldRefHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;

	static Handler *create(const HandlerData &handlerData)
	{
		return new OntologyParentFieldRefHandler{handlerData};
	}
};

namespace States {
/**
 * State representing a "ontology" struct.
 */
extern const State Ontology;

/**
 * State representing a "struct" tag within a ontology description.
 */
extern const State OntologyStruct;

/**
 * State representing an "annotation" tag within a ontology description.
 */
extern const State OntologyAnnotation;

/**
 * State representing an "attributes" tag within a structure or annotation.
 */
extern const State OntologyAttributes;

/**
 * State representing an "attribute" tag within the "attributes".
 */
extern const State OntologyAttribute;

/**
 * State representing a "field" tag within a structure or annotation.
 */
extern const State OntologyField;

/**
 * State representing a "fieldref" tag within a structure or annotation.
 */
extern const State OntologyFieldRef;

/**
 * State representing a "primitive" tag within a structure or annotation.
 */
extern const State OntologyStructPrimitive;

/**
 * State representing a "child" tag within a structure or annotation.
 */
extern const State OntologyStructChild;

/**
 * State representing a "parent" tag within a structure or annotation.
 */
extern const State OntologyStructParent;

/**
 * State representing a "field" tag within a "parent" tag.
 */
extern const State OntologyStructParentField;

/**
 * State representing a "fieldRef" tag within a "parent" tag.
 */
extern const State OntologyStructParentFieldRef;
}
}

namespace RttiTypes {
extern const Rtti OntologyParent;
}
}
#endif
