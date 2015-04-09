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

#include <core/common/RttiBuilder.hpp>
#include <core/model/Document.hpp>
#include <core/model/Ontology.hpp>
#include <core/model/Project.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

#include "DocumentHandler.hpp"
#include "OntologyHandler.hpp"
#include "State.hpp"
#include "TypesystemHandler.hpp"

namespace ousia {
namespace parser_stack {

/* OntologyHandler */

bool OntologyHandler::startCommand(Variant::mapType &args)
{
	// Create the Ontology node
	Rooted<Ontology> ontology =
	    context().getProject()->createOntology(args["name"].asString());
	ontology->setLocation(location());

	// If the ontology is defined inside a document, add the reference to the
	// document
	Rooted<Document> document = scope().select<Document>();
	if (document != nullptr) {
		document->reference(ontology);
	}

	// Push the typesystem onto the scope, set the POST_HEAD flag to true
	scope().push(ontology);
	scope().setFlag(ParserFlag::POST_HEAD, false);
	return true;
}

void OntologyHandler::end() { scope().pop(logger()); }

/* OntologyStructHandler */

bool OntologyStructHandler::startCommand(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	Rooted<Ontology> ontology = scope().selectOrThrow<Ontology>();

	Rooted<StructuredClass> structuredClass = ontology->createStructuredClass(
	    args["name"].asString(), args["cardinality"].asCardinality(), nullptr,
	    args["transparent"].asBool(), args["root"].asBool());
	structuredClass->setLocation(location());

	const std::string &isa = args["isa"].asString();
	if (!isa.empty()) {
		scope().resolve<StructuredClass>(
		    isa, structuredClass, logger(),
		    [](Handle<Node> superclass, Handle<Node> structuredClass,
		       Logger &logger) {
			    if (superclass != nullptr) {
				    structuredClass.cast<StructuredClass>()->setSuperclass(
				        superclass.cast<StructuredClass>(), logger);
			    }
			});
	}

	scope().push(structuredClass);
	return true;
}

void OntologyStructHandler::end() { scope().pop(logger()); }

/* OntologyAnnotationHandler */
bool OntologyAnnotationHandler::startCommand(Variant::mapType &args)
{
	scope().setFlag(ParserFlag::POST_HEAD, true);

	Rooted<Ontology> ontology = scope().selectOrThrow<Ontology>();

	Rooted<AnnotationClass> annotationClass =
	    ontology->createAnnotationClass(args["name"].asString());
	annotationClass->setLocation(location());

	scope().push(annotationClass);
	return true;
}

void OntologyAnnotationHandler::end() { scope().pop(logger()); }

/* OntologyAttributesHandler */

bool OntologyAttributesHandler::startCommand(Variant::mapType &args)
{
	// Fetch the current descriptor and add the attributes descriptor
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	Rooted<StructType> attrDesc = parent->getAttributesDescriptor();
	attrDesc->setLocation(location());

	scope().push(attrDesc);
	return true;
}

void OntologyAttributesHandler::end() { scope().pop(logger()); }

/* OntologyFieldHandler */

bool OntologyFieldHandler::startCommand(Variant::mapType &args)
{
	FieldDescriptor::FieldType type;
	if (args["subtree"].asBool()) {
		type = FieldDescriptor::FieldType::SUBTREE;
	} else {
		type = FieldDescriptor::FieldType::TREE;
	}

	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	auto res = parent->createFieldDescriptor(
	    logger(), type, args["name"].asString(), args["optional"].asBool());
	res.first->setLocation(location());
	if (res.second) {
		logger().warning(
		    std::string("Field \"") + res.first->getName() +
		        "\" was declared after main field. The order of fields "
		        "was changed to make the main field the last field.",
		    *res.first);
	}

	scope().push(res.first);
	return true;
}

void OntologyFieldHandler::end() { scope().pop(logger()); }

/* OntologyFieldRefHandler */

bool OntologyFieldRefHandler::startCommand(Variant::mapType &args)
{
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	const std::string &name = args["ref"].asString();

	auto loc = location();

	scope().resolveFieldDescriptor(name, parent, logger(),
	                               [loc](Handle<Node> field,
	                                     Handle<Node> parent, Logger &logger) {
		if (field != nullptr) {
			if (parent.cast<StructuredClass>()->addFieldDescriptor(
			        field.cast<FieldDescriptor>(), logger)) {
				logger.warning(
				    std::string("Field \"") + field->getName() +
				        "\" was referenced after main field was declared. The "
				        "order of fields was changed to make the main field "
				        "the last field.",
				    loc);
			}
		}
	});
	return true;
}

void OntologyFieldRefHandler::end() {}

/* OntologyPrimitiveHandler */

bool OntologyPrimitiveHandler::startCommand(Variant::mapType &args)
{
	Rooted<Descriptor> parent = scope().selectOrThrow<Descriptor>();

	FieldDescriptor::FieldType fieldType;
	if (args["subtree"].asBool()) {
		fieldType = FieldDescriptor::FieldType::SUBTREE;
	} else {
		fieldType = FieldDescriptor::FieldType::TREE;
	}

	auto res = parent->createPrimitiveFieldDescriptor(
	    new UnknownType(manager()), logger(), fieldType,
	    args["name"].asString(), args["optional"].asBool());
	res.first->setLocation(location());
	if (res.second) {
		logger().warning(
		    std::string("Field \"") + res.first->getName() +
		        "\" was declared after main field. The order of fields "
		        "was changed to make the main field the last field.",
		    *res.first);
	}

	const std::string &type = args["type"].asString();
	scope().resolveType(type, res.first, logger(),
	                    [](Handle<Node> type, Handle<Node> field,
	                       Logger &logger) {
		if (type != nullptr) {
			field.cast<FieldDescriptor>()->setPrimitiveType(type.cast<Type>());
		}
	});

	scope().push(res.first);
	return true;
}

void OntologyPrimitiveHandler::end() { scope().pop(logger()); }

/* OntologyChildHandler */

bool OntologyChildHandler::startCommand(Variant::mapType &args)
{
	Rooted<FieldDescriptor> field = scope().selectOrThrow<FieldDescriptor>();

	const std::string &name = args["ref"].asString();
	scope().resolve<StructuredClass>(
	    name, field, logger(),
	    [](Handle<Node> child, Handle<Node> field, Logger &logger) {
		    if (child != nullptr) {
			    field.cast<FieldDescriptor>()->addChild(
			        child.cast<StructuredClass>());
		    }
		});
	return true;
}

/* OntologyParentHandler */

bool OntologyParentHandler::startCommand(Variant::mapType &args)
{
	Rooted<StructuredClass> strct = scope().selectOrThrow<StructuredClass>();

	Rooted<ParserOntologyParentNode> parent{new ParserOntologyParentNode(
	    strct->getManager(), args["ref"].asString(), strct)};
	parent->setLocation(location());
	scope().push(parent);
	return true;
}

void OntologyParentHandler::end() { scope().pop(logger()); }

/* OntologyParentFieldHandler */

bool OntologyParentFieldHandler::startCommand(Variant::mapType &args)
{
	Rooted<ParserOntologyParentNode> parentNameNode =
	    scope().selectOrThrow<ParserOntologyParentNode>();
	FieldDescriptor::FieldType type;
	if (args["subtree"].asBool()) {
		type = FieldDescriptor::FieldType::SUBTREE;
	} else {
		type = FieldDescriptor::FieldType::TREE;
	}

	const std::string &name = args["name"].asString();
	const bool optional = args["optional"].asBool();
	Rooted<StructuredClass> strct =
	    parentNameNode->getParent().cast<StructuredClass>();

	// resolve the parent, create the declared field and add the declared
	// StructuredClass as child to it.
	scope().resolve<Descriptor>(
	    parentNameNode->getName(), strct, logger(),
	    [type, name, optional](Handle<Node> parent, Handle<Node> strct,
	                           Logger &logger) {
		    if (parent != nullptr) {
			    Rooted<FieldDescriptor> field =
			        (parent.cast<Descriptor>()->createFieldDescriptor(
			             logger, type, name, optional)).first;
			    field->addChild(strct.cast<StructuredClass>());
		    }
		});
	return true;
}

/* OntologyParentFieldRefHandler */

bool OntologyParentFieldRefHandler::startCommand(Variant::mapType &args)
{
	Rooted<ParserOntologyParentNode> parentNameNode =
	    scope().selectOrThrow<ParserOntologyParentNode>();

	const std::string &name = args["ref"].asString();
	Rooted<StructuredClass> strct =
	    parentNameNode->getParent().cast<StructuredClass>();
	auto loc = location();

	// resolve the parent, get the referenced field and add the declared
	// StructuredClass as child to it.
	scope().resolve<Descriptor>(
	    parentNameNode->getName(), strct, logger(),
	    [name, loc](Handle<Node> parent, Handle<Node> strct, Logger &logger) {
		    if (parent != nullptr) {
			    Rooted<FieldDescriptor> field =
			        parent.cast<Descriptor>()->getFieldDescriptor(name);
			    if (field == nullptr) {
				    logger.error(
				        std::string("Could not find referenced field ") + name,
				        loc);
				    return;
			    }
			    field->addChild(strct.cast<StructuredClass>());
		    }
		});
	return true;
}

/* Class OntologySyntaxHandler */

bool OntologySyntaxHandler::startCommand(Variant::mapType &args)
{
	scope().push(new ParserSyntaxNode(manager()));
	return true;
}

void OntologySyntaxHandler::end() { scope().pop(logger()); }

/* Class OntologyOpenCloseShortHandler */

namespace {
enum class TokenType { OPEN, CLOSE, SHORT };
}

OntologyOpenCloseShortHandler::OntologyOpenCloseShortHandler(
    const HandlerData &handlerData)
    : StaticHandler(handlerData), descr(nullptr)
{
}

bool OntologyOpenCloseShortHandler::startCommand(Variant::mapType &args)
{
	// Select the upper field, annotation and struct descriptor
	Rooted<StructuredClass> strct = scope().select<StructuredClass>();
	Rooted<AnnotationClass> anno = scope().select<AnnotationClass>();
	Rooted<FieldDescriptor> field = scope().select<FieldDescriptor>();

	// Fetch the token type this handler was created for
	TokenType type;
	if (name() == "open") {
		type = TokenType::OPEN;
	} else if (name() == "close") {
		type = TokenType::CLOSE;
	} else if (name() == "short") {
		type = TokenType::SHORT;
	} else {
		logger().error(std::string("Invalid syntax element \"") + name() +
		               std::string("\""));
		return false;
	}

	// We cannot define the short form inside a field
	if (field != nullptr && type == TokenType::SHORT) {
		logger().error(
		    std::string("Cannot define short syntax within a field."),
		    location());
		return false;
	}

	// Open, close and short syntax may not be defined within the field of an
	// annotation, only for the annotation itself
	if (anno != nullptr && field != nullptr) {
		logger().error(std::string("Cannot define ") + name() +
		                   std::string(" syntax within annotation field."),
		               location());
		return false;
	}

	// We cannot define a short form for an annotation
	if (anno != nullptr && type == TokenType::SHORT) {
		logger().error(
		    std::string("Cannot define short syntax for annotations"),
		    location());
		return false;
	}

	// Fetch the pointer for either the open, close or short token
	descr = nullptr;
	if (field != nullptr) {
		switch (type) {
			case TokenType::OPEN:
				descr = field->getOpenTokenPointer();
				break;
			case TokenType::CLOSE:
				descr = field->getCloseTokenPointer();
				break;
			default:
				break;
		}
	} else if (anno != nullptr) {
		switch (type) {
			case TokenType::OPEN:
				descr = anno->getOpenTokenPointer();
				break;
			case TokenType::CLOSE:
				descr = anno->getCloseTokenPointer();
				break;
			default:
				break;
		}
	} else if (strct != nullptr) {
		switch (type) {
			case TokenType::OPEN:
				descr = strct->getOpenTokenPointer();
				break;
			case TokenType::CLOSE:
				descr = strct->getCloseTokenPointer();
				break;
			case TokenType::SHORT:
				descr = strct->getShortTokenPointer();
				break;
		}
	}

	// Make sure a descriptor was set (the checks above should already prevent
	// this case from happening).
	if (descr == nullptr) {
		logger().error(
		    "Internal error: Could not find corresponding token descriptor",
		    location());
		return false;
	}

	// Make sure the descriptor does not already have any content
	if (!descr->isEmpty()) {
		if (field != nullptr) {
			logger().error(name() + std::string(" syntax for field \"") +
			               field->getName() +
			               std::string("\" was already defined"));
		} else if (anno != nullptr) {
			logger().error(name() + std::string(" syntax for annotation \"") +
			               anno->getName() +
			               std::string("\" was already defined"));
		} else if (strct != nullptr) {
			logger().error(name() + std::string(" syntax for structure \"") +
			               anno->getName() +
			               std::string("\" was already defined"));
		}
		return false;
	}

	// Push the corresponding nodes onto the stack
	switch (type) {
		case TokenType::OPEN:
			scope().push(new ParserSyntaxOpenNode(manager(), descr));
			break;
		case TokenType::CLOSE:
			scope().push(new ParserSyntaxCloseNode(manager(), descr));
			break;
		case TokenType::SHORT:
			scope().push(new ParserSyntaxShortNode(manager(), descr));
			break;
	}
	return true;
}

bool OntologyOpenCloseShortHandler::data()
{
	Variant str = readData();
	if (descr && descr->isEmpty()) {
		// Read the token descriptor
		*descr = TokenDescriptor(str.asString());

		// Make sure the token descriptor is actually valid, if not, reset it
		// (do not, however return false as the data per se was at the right
		// place)
		if (!descr->isValid()) {
			logger().error(
			    std::string("Given token \"") + str.asString() +
			    std::string(
			        "\" is not a valid user defined token (no whitespaces, "
			        "must start and end with a non-alphanumeric character, "
			        "must not override OSML tokens)."));
			*descr = TokenDescriptor();
		}
		return true;
	}
	logger().error("Did not expect any data here", str);
	return false;
}

void OntologyOpenCloseShortHandler::end()
{
	if (descr->isEmpty()) {
		logger().error(std::string("Expected valid token for ") + name() +
		                   std::string(" syntax descriptor."),
		               location());
	}
	scope().pop(logger());
}

/* Class OntologySyntaxTokenHandler */

bool OntologySyntaxTokenHandler::startCommand(Variant::mapType &args)
{
	// Select the ParserSyntaxTokenNode containing the reference at the
	// TokenDescriptor
	Rooted<ParserSyntaxTokenNode> tokenNode =
	    scope().selectOrThrow<ParserSyntaxTokenNode>();

	if (!tokenNode->descr->isEmpty()) {
		logger().error(
		    "Token was already set, did not expect another command here.",
		    location());
		return false;
	}

	// Select the correct special token
	TokenId id = Tokens::Empty;
	if (name() == "newline") {
		id = Tokens::Newline;
	} else if (name() == "paragraph") {
		id = Tokens::Paragraph;
	} else if (name() == "section") {
		id = Tokens::Section;
	} else if (name() == "indent") {
		id = Tokens::Indent;
	} else if (name() == "dedent") {
		id = Tokens::Dedent;
	} else {
		logger().error(
		    "Expected one of \"newline\", \"paragraph\", \"section\", "
		    "\"indent\", \"dedent\", but got \"" +
		        name() + "\"",
		    location());
		return false;
	}

	// Set the token descriptor
	*tokenNode->descr = TokenDescriptor(id);
	return true;
}

/* Class OntologyWhitespaceHandler */

bool OntologyWhitespaceHandler::startCommand(Variant::mapType &args)
{
	// Fetch the field descriptor, log an error if "whitespace" was not
	// specified inside a field descriptor
	Rooted<FieldDescriptor> field = scope().select<FieldDescriptor>();
	if (field == nullptr) {
		logger().error(
		    "Whitespace mode definition is only allowed inside fields.",
		    location());
		return false;
	}
	return true;
}

bool OntologyWhitespaceHandler::data()
{
	if (whitespaceModeStr != nullptr) {
		logger().error(
		    "Did not expect any more data, whitespace mode has already been "
		    "set.",
		    location());
		return false;
	}
	whitespaceModeStr = readData();
	return true;
}

void OntologyWhitespaceHandler::end()
{
	// Make sure the given whitespace mode is valid
	if (whitespaceModeStr == nullptr) {
		whitespaceModeStr = "";
	}
	const std::string &mode = whitespaceModeStr.asString();
	Rooted<FieldDescriptor> field = scope().selectOrThrow<FieldDescriptor>();
	if (mode == "trim") {
		field->setWhitespaceMode(WhitespaceMode::TRIM);
	} else if (mode == "collapse") {
		field->setWhitespaceMode(WhitespaceMode::COLLAPSE);
	} else if (mode == "preserve") {
		field->setWhitespaceMode(WhitespaceMode::PRESERVE);
	} else {
		logger().error(
		    "Expected \"trim\", \"collapse\" or \"preserve\" as whitespace "
		    "mode.",
		    whitespaceModeStr);
		return;
	}
}

/* Class ParserSyntaxTokenNode */

ParserSyntaxTokenNode::ParserSyntaxTokenNode(Manager &mgr,
                                             TokenDescriptor *descr)
    : Node(mgr), descr(descr)
{
}

namespace States {
const State Ontology = StateBuilder()
                           .parents({&None, &Document})
                           .createdNodeType(&RttiTypes::Ontology)
                           .elementHandler(OntologyHandler::create)
                           .arguments({Argument::String("name")});

const State OntologyStruct =
    StateBuilder()
        .parent(&Ontology)
        .createdNodeType(&RttiTypes::StructuredClass)
        .elementHandler(OntologyStructHandler::create)
        .arguments({Argument::String("name"),
                    Argument::Cardinality("cardinality", Cardinality::any()),
                    Argument::Bool("root", false),
                    Argument::Bool("transparent", false),
                    Argument::String("isa", "")});

const State OntologyAnnotation =
    StateBuilder()
        .parent(&Ontology)
        .createdNodeType(&RttiTypes::AnnotationClass)
        .elementHandler(OntologyAnnotationHandler::create)
        .arguments({Argument::String("name")});

const State OntologyAttributes =
    StateBuilder()
        .parents({&OntologyStruct, &OntologyAnnotation})
        .createdNodeType(&RttiTypes::StructType)
        .elementHandler(OntologyAttributesHandler::create)
        .arguments({});

const State OntologyAttribute =
    StateBuilder()
        .parent(&OntologyAttributes)
        .elementHandler(TypesystemStructFieldHandler::create)
        .arguments({Argument::String("name"), Argument::String("type"),
                    Argument::Any("default", Variant::fromObject(nullptr))});

const State OntologyField = StateBuilder()
                                .parents({&OntologyStruct, &OntologyAnnotation})
                                .createdNodeType(&RttiTypes::FieldDescriptor)
                                .elementHandler(OntologyFieldHandler::create)
                                .arguments({Argument::String("name", ""),
                                            Argument::Bool("subtree", false),
                                            Argument::Bool("optional", false)});

const State OntologyFieldRef =
    StateBuilder()
        .parents({&OntologyStruct, &OntologyAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(OntologyFieldRefHandler::create)
        .arguments({Argument::String("ref", DEFAULT_FIELD_NAME)});

const State OntologyStructPrimitive =
    StateBuilder()
        .parents({&OntologyStruct, &OntologyAnnotation})
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(OntologyPrimitiveHandler::create)
        .arguments(
            {Argument::String("name", ""), Argument::Bool("subtree", false),
             Argument::Bool("optional", false), Argument::String("type")});

const State OntologyStructChild =
    StateBuilder()
        .parent(&OntologyField)
        .elementHandler(OntologyChildHandler::create)
        .arguments({Argument::String("ref")});

const State OntologyStructParent =
    StateBuilder()
        .parent(&OntologyStruct)
        .createdNodeType(&RttiTypes::ParserOntologyParentNode)
        .elementHandler(OntologyParentHandler::create)
        .arguments({Argument::String("ref")});

const State OntologyStructParentField =
    StateBuilder()
        .parent(&OntologyStructParent)
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(OntologyParentFieldHandler::create)
        .arguments({Argument::String("name", ""),
                    Argument::Bool("subtree", false),
                    Argument::Bool("optional", false)});

const State OntologyStructParentFieldRef =
    StateBuilder()
        .parent(&OntologyStructParent)
        .createdNodeType(&RttiTypes::FieldDescriptor)
        .elementHandler(OntologyParentFieldRefHandler::create)
        .arguments({Argument::String("ref", DEFAULT_FIELD_NAME)});

const State OntologySyntax =
    StateBuilder()
        .parents({&OntologyStruct, &OntologyField, &OntologyStructPrimitive,
                  &OntologyAnnotation})
        .createdNodeType(&RttiTypes::ParserSyntaxNode)
        .elementHandler(OntologySyntaxHandler::create)
        .arguments(Arguments{});

const State OntologySyntaxToken =
    StateBuilder()
        .parents({&OntologySyntaxOpen, &OntologySyntaxClose, &OntologySyntax})
        .createdNodeType(&RttiTypes::ParserSyntaxTokenNode)
        .elementHandler(OntologySyntaxTokenHandler::create)
        .arguments(Arguments{});

const State OntologySyntaxOpen =
    StateBuilder()
        .parent(&OntologySyntax)
        .createdNodeType(&RttiTypes::ParserSyntaxOpenNode)
        .elementHandler(OntologyOpenCloseShortHandler::create)
        .arguments(Arguments{});

const State OntologySyntaxClose =
    StateBuilder()
        .parent(&OntologySyntax)
        .createdNodeType(&RttiTypes::ParserSyntaxCloseNode)
        .elementHandler(OntologyOpenCloseShortHandler::create)
        .arguments(Arguments{});

const State OntologySyntaxShort =
    StateBuilder()
        .parent(&OntologySyntax)
        .createdNodeType(&RttiTypes::ParserSyntaxShortNode)
        .elementHandler(OntologyOpenCloseShortHandler::create)
        .arguments(Arguments{});

const State OntologySyntaxWhitespace =
    StateBuilder()
        .parent(&OntologySyntax)
        .elementHandler(OntologyWhitespaceHandler::create)
        .arguments(Arguments{});
}
}

namespace RttiTypes {
const Rtti ParserOntologyParentNode =
    RttiBuilder<ousia::parser_stack::ParserOntologyParentNode>(
        "ParserOntologyParentNode").parent(&Node);
const Rtti ParserSyntaxNode =
    RttiBuilder<ousia::parser_stack::ParserSyntaxNode>("ParserSyntaxNode")
        .parent(&Node);
const Rtti ParserSyntaxTokenNode =
    RttiBuilder<ousia::parser_stack::ParserSyntaxTokenNode>(
        "ParserSyntaxTokenNode").parent(&Node);
const Rtti ParserSyntaxOpenNode =
    RttiBuilder<ousia::parser_stack::ParserSyntaxOpenNode>(
        "ParserSyntaxOpenNode").parent(&ParserSyntaxTokenNode);
const Rtti ParserSyntaxCloseNode =
    RttiBuilder<ousia::parser_stack::ParserSyntaxCloseNode>(
        "ParserSyntaxCloseNode").parent(&ParserSyntaxTokenNode);
const Rtti ParserSyntaxShortNode =
    RttiBuilder<ousia::parser_stack::ParserSyntaxShortNode>(
        "ParserSyntaxShortNode").parent(&ParserSyntaxTokenNode);
}
}
