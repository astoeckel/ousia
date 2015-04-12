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

#include <algorithm>

#include <core/common/RttiBuilder.hpp>
#include <core/common/Utils.hpp>
#include <core/common/VariantReader.hpp>
#include <core/model/Document.hpp>
#include <core/model/Ontology.hpp>
#include <core/model/Project.hpp>
#include <core/model/Typesystem.hpp>
#include <core/parser/utils/TokenizedData.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>

#include "DocumentHandler.hpp"
#include "State.hpp"

namespace ousia {
namespace parser_stack {

/* DocumentHandler */

bool DocumentHandler::startCommand(Variant::mapType &args)
{
	Rooted<Document> document =
	    context().getProject()->createDocument(args["name"].asString());
	document->setLocation(location());

	scope().push(document);
	scope().setFlag(ParserFlag::POST_HEAD, false);
	scope().setFlag(ParserFlag::POST_USER_DEFINED_TOKEN_REGISTRATION, false);

	return true;
}

void DocumentHandler::end() { scope().pop(logger()); }

/* DocumentField */

DocumentField::DocumentField(Manager &mgr, Handle<Node> parent, size_t fieldIdx,
                             bool transparent, bool explicitField)
    : Node(mgr, parent),
      fieldIdx(fieldIdx),
      transparent(transparent),
      explicitField(explicitField)
{
}

Rooted<FieldDescriptor> DocumentField::getDescriptor()
{
	// Fetch the FieldDescriptor from the parent node. The parent node should
	// either be a structured entity or an annotation entity
	Rooted<Managed> parent = getParent();
	if (parent->isa(&RttiTypes::StructuredEntity)) {
		return parent.cast<StructuredEntity>()
		    ->getDescriptor()
		    ->getFieldDescriptor(fieldIdx);
	} else if (parent->isa(&RttiTypes::AnnotationEntity)) {
		return parent.cast<AnnotationEntity>()
		    ->getDescriptor()
		    ->getFieldDescriptor(fieldIdx);
	}

	// Well, we never should get here
	// TODO: Introduce macro for unreachable code?
	assert(!"This should never be reached");
	return nullptr;
}

/* DocumentChildHandler */

DocumentChildHandler::DocumentChildHandler(const HandlerData &handlerData)
    : Handler(handlerData),
      isExplicitField(false),
      isGreedy(true),
      inImplicitDefaultField(false)
{
	// Register all user defined tokens if this has not yet been done
	if (!scope().getFlag(ParserFlag::POST_USER_DEFINED_TOKEN_REGISTRATION)) {
		registerUserDefinedTokens();
	}
}

void DocumentChildHandler::registerUserDefinedTokens()
{
	// Set the POST_USER_DEFINED_TOKEN_REGISTRATION flag, to prevent this method
	// from being called again
	scope().setFlag(ParserFlag::POST_USER_DEFINED_TOKEN_REGISTRATION, true);

	// Fetch the underlying document and all ontologies registered in the
	// document and register all user defined tokens in the parser
	Rooted<Document> doc = scope().selectOrThrow<Document>();
	for (Rooted<Ontology> ontology : doc->getOntologies()) {
		std::vector<TokenDescriptor *> tokens =
		    ontology->getAllTokenDescriptors();
		for (TokenDescriptor *token : tokens) {
			if (!token->special) {
				token->id = registerToken(token->token);
			}
		}
	}
}

void DocumentChildHandler::preamble(Rooted<Node> &parentNode, size_t &fieldIdx,
                                    DocumentEntity *&parent)
{
	// Check if the parent in the structure tree was an explicit field
	// reference.
	if (parentNode->isa(&RttiTypes::DocumentField)) {
		fieldIdx = parentNode.cast<DocumentField>()->fieldIdx;
		parentNode = scope().selectOrThrow(
		    {&RttiTypes::StructuredEntity, &RttiTypes::AnnotationEntity});
	}

	// Reference the parent entity explicitly.
	parent = nullptr;
	if (parentNode->isa(&RttiTypes::StructuredEntity)) {
		parent = static_cast<DocumentEntity *>(
		    parentNode.cast<StructuredEntity>().get());
	} else if (parentNode->isa(&RttiTypes::AnnotationEntity)) {
		parent = static_cast<DocumentEntity *>(
		    parentNode.cast<AnnotationEntity>().get());
	}
}

void DocumentChildHandler::pushScopeTokens()
{
	// List containing the unfiltered syntax descriptors
	std::vector<SyntaxDescriptor> descrs;

	// Skip the DocumentField and the curresponding StructuredEntity
	// if we're currently in the implicit default field of a non-greedy
	// structure.
	size_t explicitSkipCount = (!isGreedy && inImplicitDefaultField) ? 2 : 0;

	// Fetch the current scope stack and search the first non-transparent field
	// or structure
	const ManagedVector<Node> &stack = scope().getStack();
	for (auto sit = stack.crbegin(); sit != stack.crend(); sit++) {
		Rooted<Node> nd = *sit;

		// TODO: Why can't this functionality be in a common base class?

		// Check whether the field is transparent, if not, fetch the tokens
		if (nd->isa(&RttiTypes::DocumentField)) {
			Rooted<DocumentField> field = nd.cast<DocumentField>();
			if (!field->transparent) {
				if (explicitSkipCount > 0) {
					explicitSkipCount--;
					continue;
				}
				descrs = field->getDescriptor()->getPermittedTokens();
				break;
			}
		}

		// Check whether the sturcture is transparent, if not, fetch the tokens
		if (nd->isa(&RttiTypes::StructuredEntity)) {
			Rooted<StructuredEntity> entity = nd.cast<StructuredEntity>();
			if (!entity->isTransparent()) {
				if (explicitSkipCount > 0) {
					explicitSkipCount--;
					continue;
				}
				descrs = entity->getDescriptor()->getPermittedTokens();
				break;
			}
		}
	}

	// Push the filtered tokens onto the stack
	pushTokens(descrs);
}

void DocumentChildHandler::pushDocumentField(Handle<Node> parent,
                                             Handle<FieldDescriptor> fieldDescr,
                                             size_t fieldIdx, bool transparent,
                                             bool explicitField)
{
	// Push the field onto the scope
	Rooted<DocumentField> field = new DocumentField(manager(), parent, fieldIdx,
	                                                transparent, explicitField);
	field->setLocation(location());
	scope().push(field);
}

void DocumentChildHandler::popDocumentField()
{
	// Pop the field from the scope, make sure it actually is a DocumentField
	assert(scope().getLeaf()->isa(&RttiTypes::DocumentField));
	scope().pop(logger());
}

void DocumentChildHandler::createPath(const NodeVector<Node> &path,
                                      DocumentEntity *&parent, size_t p0)
{
	size_t S = path.size();
	for (size_t p = p0; p < S; p = p + 2) {
		// add the field.
		const ssize_t fieldIdx =
		    parent->getDescriptor()->getFieldDescriptorIndex();
		const Rooted<FieldDescriptor> fieldDescr =
		    parent->getDescriptor()->getFieldDescriptor(fieldIdx);
		pushDocumentField(scope().getLeaf(), fieldDescr, fieldIdx, true, false);

		// add the transparent/implicit structure element.
		Rooted<StructuredEntity> transparent =
		    parent->createChildStructuredEntity(path[p].cast<StructuredClass>(),
		                                        Variant::mapType{},
		                                        path[p - 1]->getName(), "");
		transparent->setLocation(location());
		transparent->setTransparent(true);
		scope().push(transparent);
		parent = static_cast<DocumentEntity *>(transparent.get());
	}
	// add the field.
	const ssize_t fieldIdx = parent->getDescriptor()->getFieldDescriptorIndex();
	const Rooted<FieldDescriptor> fieldDescr =
	    parent->getDescriptor()->getFieldDescriptor(fieldIdx);
	pushDocumentField(scope().getLeaf(), fieldDescr, fieldIdx, true, false);

	// Generally allow explicit fields in the new field
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, false);
}

void DocumentChildHandler::createPath(const size_t &firstFieldIdx,
                                      const NodeVector<Node> &path,
                                      DocumentEntity *&parent)
{
	// Add the first element
	Rooted<StructuredEntity> transparent = parent->createChildStructuredEntity(
	    path[0].cast<StructuredClass>(), firstFieldIdx);
	transparent->setLocation(location());
	transparent->setTransparent(true);
	scope().push(transparent);
	parent = static_cast<DocumentEntity *>(transparent.get());

	createPath(path, parent, 2);

	// Generally allow explicit fields in the new field
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, false);
}

void DocumentChildHandler::rollbackPath()
{
	// Remove the topmost field
	popDocumentField();

	// Pop all remaining transparent elements.
	while (scope().getLeaf()->isa(&RttiTypes::StructuredEntity) &&
	       scope().getLeaf().cast<StructuredEntity>()->isTransparent()) {
		// Pop the transparent element.
		scope().pop(logger());

		// Pop the transparent field.
		popDocumentField();
	}
}

static std::string extractNameAttribute(Variant::mapType &args)
{
	// Extract the special "name" attribute from the input arguments.
	// The remaining attributes will be forwarded to the newly constructed
	// element.
	std::string res;
	auto it = args.find("name");
	if (it != args.end()) {
		res = it->second.asString();
		args.erase(it);
	}
	return res;
}

bool DocumentChildHandler::startCommand(Variant::mapType &args)
{
	std::string nameAttr = extractNameAttribute(args);

	scope().setFlag(ParserFlag::POST_HEAD, true);
	while (true) {
		Rooted<Node> parentNode = scope().getLeaf();

		Rooted<StructuredEntity> entity;
		// handle the root note specifically.
		if (parentNode->isa(&RttiTypes::Document)) {
			// if we already have a root node, stop.
			if (parentNode.cast<Document>()->getRoot() != nullptr) {
				logger().warning(
				    "This document already has a root node. The additional "
				    "node is ignored.",
				    location());
				return false;
			}
			Rooted<StructuredClass> strct = scope().resolve<StructuredClass>(
			    Utils::split(name(), ':'), logger());
			if (strct == nullptr) {
				// if we could not resolve the name, throw an exception.
				throw LoggableException(
				    std::string("\"") + name() + "\" could not be resolved.",
				    location());
			}
			entity = parentNode.cast<Document>()->createRootStructuredEntity(
			    strct, args, nameAttr);
		} else {
			assert(parentNode->isa(&RttiTypes::DocumentField));

			size_t fieldIdx;
			DocumentEntity *parent;

			preamble(parentNode, fieldIdx, parent);

			/*
			 * Try to find a FieldDescriptor for the given tag if we are not in
			 * a field already. This does _not_ try to construct transparent
			 * paths in between.
			 */
			{
				ssize_t newFieldIdx =
				    parent->getDescriptor()->getFieldDescriptorIndex(name());
				if (newFieldIdx != -1) {
					// Check whether explicit fields are allowed here, if not
					if (scope().getFlag(ParserFlag::POST_EXPLICIT_FIELDS)) {
						logger().note(
						    std::string(
						        "Data or structure commands have already been "
						        "given, command \"") +
						        name() +
						        std::string(
						            "\" is not interpreted as explicit "
						            "field. Move explicit field "
						            "references to the beginning."),
						    location());
					} else {
						pushDocumentField(
						    parentNode,
						    parent->getDescriptor()->getFieldDescriptor(
						        newFieldIdx),
						    newFieldIdx, false, true);
						pushScopeTokens();
						isExplicitField = true;
						return true;
					}
				}
			}

			// Otherwise create a new StructuredEntity
			Rooted<StructuredClass> strct = scope().resolve<StructuredClass>(
			    Utils::split(name(), ':'), logger());
			if (strct == nullptr) {
				// if we could not resolve the name, throw an exception.
				throw LoggableException(
				    std::string("\"") + name() + "\" could not be resolved.",
				    location());
			}

			// calculate a path if transparent entities are needed in between.
			Rooted<FieldDescriptor> field =
			    parent->getDescriptor()->getFieldDescriptor(fieldIdx);
			size_t lastFieldIdx = fieldIdx;
			auto pathRes = field->pathTo(strct, logger());
			if (!pathRes.second) {
				if (scope().getLeaf().cast<DocumentField>()->transparent) {
					// if we have transparent elements above us in the structure
					// tree we try to unwind them before we give up.
					// pop the implicit field.
					popDocumentField();
					// pop the implicit element.
					scope().pop(logger());
					continue;
				}
				throw LoggableException(
				    std::string("An instance of \"") + strct->getName() +
				        "\" is not allowed as child of field \"" +
				        field->getNameOrDefaultName() + "\" of descriptor \"" +
				        parent->getDescriptor()->getName() + "\"",
				    location());
			}
			if (!pathRes.first.empty()) {
				createPath(lastFieldIdx, pathRes.first, parent);
				lastFieldIdx =
				    parent->getDescriptor()->getFieldDescriptorIndex();
			}
			// create the entity for the new element at last.
			entity = parent->createChildStructuredEntity(strct, lastFieldIdx,
			                                             args, nameAttr);
		}

		// We're past the region in which explicit fields can be defined in the
		// parent structure element
		scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, true);

		// Push the entity onto the stack
		entity->setLocation(location());
		scope().push(entity);
		pushScopeTokens();

		return true;
	}
}

bool DocumentChildHandler::startAnnotation(Variant::mapType &args)
{
	std::string nameAttr = extractNameAttribute(args);

	scope().setFlag(ParserFlag::POST_HEAD, true);

	size_t fieldIdx;
	DocumentEntity *parent;
	while (true) {
		Rooted<Node> parentNode = scope().getLeaf();

		// Make sure the parent node is a DocumentField
		if (parentNode->isa(&RttiTypes::Document)) {
			logger().error(
			    "Cannot start or end annotation at the document level.",
			    location());
			return false;
		}
		assert(parentNode->isa(&RttiTypes::DocumentField));

		preamble(parentNode, fieldIdx, parent);

		if (!parent->getDescriptor()
		         ->getFieldDescriptor(fieldIdx)
		         ->isPrimitive()) {
			break;
		}

		// If we are inside a primitive field and have transparent elements on
		// the stack we unwind the stack until we are inside
		// a non-primitive field.
		if (scope().getLeaf().cast<DocumentField>()->transparent) {
			// if we have transparent elements above us in the structure
			// tree we try to unwind them before we give up.
			// pop the implicit field.
			popDocumentField();
			// pop the implicit element.
			scope().pop(logger());
			continue;
		} else {
			logger().error(
			    "Cannot start or end annotation within the primitive field \"" +
			        parent->getDescriptor()
			            ->getFieldDescriptor(fieldIdx)
			            ->getNameOrDefaultName() +
			        "\" of descriptor \"" + parent->getDescriptor()->getName() +
			        "\".",
			    location());
			return false;
		}
	}

	// Create the anchor
	Rooted<Anchor> anchor = parent->createChildAnchor(fieldIdx);
	anchor->setLocation(location());

	// Resolve the AnnotationClass
	Rooted<AnnotationClass> annoClass;
	if (!name().empty()) {
		annoClass = scope().resolve<AnnotationClass>(Utils::split(name(), ':'),
		                                             logger());
	}

	switch (type()) {
		case HandlerType::ANNOTATION_START: {
			// Create the AnnotationEntity itself.
			if (annoClass == nullptr) {
				// if we could not resolve the name, throw an exception.
				throw LoggableException(
				    std::string("\"") + name() + "\" could not be resolved.",
				    location());
			}
			Rooted<Document> doc = scope().selectOrThrow<Document>();
			Rooted<AnnotationEntity> anno = doc->createChildAnnotation(
			    annoClass, anchor, nullptr, args, nameAttr);

			// Push the entity onto the stack
			anno->setLocation(location());
			scope().push(anno);
			break;
		}
		case HandlerType::ANNOTATION_END: {
			// if we want to end an annotation, look for the matching start
			// Anchor ...
			Rooted<Anchor> start =
			    parent->searchStartAnchor(fieldIdx, annoClass, nameAttr);
			if (start == nullptr) {
				logger().error(
				    "Did not find matching annotation start for annotation "
				    "end.",
				    *anchor);
				parent->removeStructureNodeFromField(anchor, fieldIdx);
				return false;
			}
			// ... and set the end Anchor.
			start->getAnnotation()->setEnd(anchor);
			break;
		}
		default:
			throw OusiaException(
			    "Internal Error: Invalid handler type in startAnnotation");
	}
	// We're past the region in which explicit fields can be defined in the
	// parent structure element
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, true);
	return true;
}

bool DocumentChildHandler::startToken(Handle<Node> node, bool greedy)
{
	// Copy the "greedy" flag. If not greedy, set the inImplicitDefaultField
	// flag to true, in order to push the tokens of the previous command.
	isGreedy = greedy;
	inImplicitDefaultField = !greedy;

	bool isStruct = node->isa(&RttiTypes::StructuredClass);
	//	bool isField = node->isa(&RttiTypes::FieldDescriptor);
	//	bool isAnnotation = node->isa(&RttiTypes::AnnotationClass);

	if (!isStruct) {
		// TODO: Implement
		return false;
	}

	Rooted<StructuredClass> strct = node.cast<StructuredClass>();

	scope().setFlag(ParserFlag::POST_HEAD, true);
	while (true) {
		// Make sure the parent node is not the document
		Rooted<Node> parentNode = scope().getLeaf();
		if (parentNode->isa(&RttiTypes::Document)) {
			logger().error(
			    "Tokens are not allowed on the root document level.");
			return false;
		}
		assert(parentNode->isa(&RttiTypes::DocumentField));

		// TODO: Move this to more generic method
		// Fetch the parent document entity and the parent field index
		size_t fieldIdx;
		DocumentEntity *parent;
		preamble(parentNode, fieldIdx, parent);

		// Calculate a path if transparent entities are needed in between.
		Rooted<FieldDescriptor> field =
		    parent->getDescriptor()->getFieldDescriptor(fieldIdx);
		size_t lastFieldIdx = fieldIdx;
		auto pathRes = field->pathTo(strct, logger());
		if (!pathRes.second) {
			// If we have transparent elements above us in the structure tree,
			// try to unwind them before we give up.
			if (scope().getLeaf().cast<DocumentField>()->transparent) {
				// Pop the implicit field.
				popDocumentField();

				// Pop the implicit element.
				scope().pop(logger());
				continue;
			}
			throw LoggableException(
			    std::string("An instance of \"") + strct->getName() +
			        "\" is not allowed as child of field \"" +
			        field->getNameOrDefaultName() + "\" of descriptor \"" +
			        parent->getDescriptor()->getName() + "\"",
			    location());
		}

		// Create the path (if one is available)
		if (!pathRes.first.empty()) {
			createPath(lastFieldIdx, pathRes.first, parent);
			lastFieldIdx = parent->getDescriptor()->getFieldDescriptorIndex();
		}

		// Create the entity for the new element at last.
		Rooted<StructuredEntity> entity = parent->createChildStructuredEntity(
		    strct, lastFieldIdx, Variant::mapType{}, "");

		// We're past the region in which explicit fields can be defined in the
		// parent structure element
		scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, true);

		// Push the entity onto the stack
		entity->setLocation(location());
		scope().push(entity);
		pushScopeTokens();

		return true;
	}
}

EndTokenResult DocumentChildHandler::endToken(Handle<Node> node,
                                              size_t maxStackDepth)
{
	// Fetch the current scope stack
	const ManagedVector<Node> &stack = scope().getStack();

	bool found = false;  // true once the given node has been found
	bool repeat = false;
	size_t scopeStackDepth = 0;    // # of elems on the scope stack
	size_t currentStackDepth = 0;  // # of "explicit" elems on the parser stack

	// Iterate over the elements in the scope stack
	for (auto sit = stack.crbegin(); sit != stack.crend();
	     sit++, scopeStackDepth++) {
		Rooted<Node> leaf = *sit;
		bool isExplicit = false;
		if (leaf->isa(&RttiTypes::DocumentField)) {
			Rooted<DocumentField> field = leaf.cast<DocumentField>();
			if (field->getDescriptor() == node) {
				// If the field is transparent, end it by incrementing the depth
				// counter -- both the field itself and the consecutive element
				// need to be removed
				found = true;
				if (field->transparent) {
					repeat = true;
					scopeStackDepth++;
				}
			}
			isExplicit = field->explicitField;
		} else if (leaf->isa(&RttiTypes::StructuredEntity)) {
			Rooted<StructuredEntity> entity = leaf.cast<StructuredEntity>();
			found = entity->getDescriptor() == node;
			repeat = found && entity->isTransparent();
			isExplicit = !entity->isTransparent();
		}

		// TODO: End annotations!

		// If the given structure is a explicit sturcture (represents a handler)
		// increment the stack depth and abort once the maximum stack depth has
		// been surpassed.
		if (isExplicit) {
			currentStackDepth++;
		}
		if (found || currentStackDepth > maxStackDepth) {
			break;
		}
	}

	// Abort with a value smaller than zero if the element has not been found
	if (!found || currentStackDepth > maxStackDepth) {
		return EndTokenResult();
	}

	// If the element has been found, return the number of handlers that have to
	// be popped from the parser stack
	if (currentStackDepth > 0) {
		return EndTokenResult(currentStackDepth, true, repeat);
	}

	// End all elements that were marked for being closed
	for (size_t i = 0; i < scopeStackDepth + 1; i++) {
		scope().pop(logger());
	}
	return EndTokenResult(0, true, false);
}

void DocumentChildHandler::end()
{
	// Distinguish the handler type
	switch (type()) {
		case HandlerType::COMMAND:
		case HandlerType::ANNOTATION_START:
		case HandlerType::TOKEN:
			if (!isExplicitField) {
				// pop the "main" element.
				scope().pop(logger());
			} else {
				// in case of explicit fields, roll back.
				rollbackPath();
			}
			break;
		case HandlerType::ANNOTATION_END:
			// We have nothing to pop from the stack
			break;
	}
}

bool DocumentChildHandler::fieldStart(bool &isDefault, bool isImplicit,
                                      size_t fieldIdx)
{
	if (isExplicitField) {
		// In case of explicit fields we do not want to create another field.
		isDefault = true;
		return fieldIdx == 0;
	}
	inImplicitDefaultField = isImplicit;

	Rooted<Node> parentNode = scope().getLeaf();
	assert(parentNode->isa(&RttiTypes::StructuredEntity) ||
	       parentNode->isa(&RttiTypes::AnnotationEntity));
	size_t dummy;
	DocumentEntity *parent;

	preamble(parentNode, dummy, parent);

	ManagedVector<FieldDescriptor> fields =
	    parent->getDescriptor()->getFieldDescriptors();

	if (isDefault) {
		if (fields.empty()) {
			return false;
		}
		fieldIdx = fields.size() - 1;
	} else {
		if (fieldIdx >= fields.size()) {
			return false;
		}
		isDefault = fieldIdx == fields.size() - 1;
	}

	// push the field on the stack.
	pushDocumentField(parentNode, fields[fieldIdx], fieldIdx, false, false);
	pushScopeTokens();

	// Generally allow explicit fields in the new field
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, false);

	return true;
}

void DocumentChildHandler::fieldEnd()
{
	if (!isExplicitField) {
		popTokens();
		rollbackPath();
	}
	inImplicitDefaultField = false;
}

bool DocumentChildHandler::convertData(Handle<FieldDescriptor> field,
                                       Variant &data, Logger &logger)
{
	bool valid = true;
	Rooted<Type> type = field->getPrimitiveType();

	// If the content is supposed to be of type string, we only need to check
	// for "magic" values -- otherwise just call the "parseGenericString"
	// function on the string data
	if (type->isa(&RttiTypes::StringType)) {
		const std::string &str = data.asString();
		// TODO: Referencing constants with "." separator should also work
		if (Utils::isIdentifier(str)) {
			data.markAsMagic();
		}
	} else {
		// Parse the string as generic string, assign the result
		auto res = VariantReader::parseGenericString(
		    data.asString(), logger, data.getLocation().getSourceId(),
		    data.getLocation().getStart());
		data = res.second;
	}

	// Now try to resolve the value for the primitive type
	return valid && scope().resolveValue(data, type, logger);
}

bool DocumentChildHandler::data()
{
	// We're past the region in which explicit fields can be defined in the
	// parent structure element
	scope().setFlag(ParserFlag::POST_EXPLICIT_FIELDS, true);

	Rooted<Node> parentField = scope().getLeaf();
	assert(parentField->isa(&RttiTypes::DocumentField));

	size_t fieldIdx;
	DocumentEntity *parent;

	preamble(parentField, fieldIdx, parent);

	Rooted<Descriptor> desc = parent->getDescriptor();

	// Retrieve the actual FieldDescriptor
	Rooted<FieldDescriptor> field = desc->getFieldDescriptor(fieldIdx);

	// If it is a primitive field directly, try to parse the content.
	if (field->isPrimitive()) {
		// Add it as primitive content.
		Variant text = readData();  // TODO: Eliminate readData method
		if (!convertData(field, text, logger())) {
			return false;
		}

		parent->createChildDocumentPrimitive(text, fieldIdx);
		return true;
	}

	// Search through all permitted default fields of the parent class that
	// allow primitive content at this point and could be constructed via
	// transparent intermediate entities.
	ManagedVector<FieldDescriptor> defaultFields = field->getDefaultFields();

	// Try to parse the data using the type specified by the respective field.
	// If that does not work we proceed to the next possible field.
	std::vector<LoggerFork> forks;
	for (auto primitiveField : defaultFields) {
		// Then try to parse the content using the type specification.
		forks.emplace_back(logger().fork());

		// Try to parse the data
		Variant text = readData();  // TODO: Eliminate readData method
		if (!convertData(primitiveField, text, forks.back())) {
			continue;
		}

		// The conversion worked, commit any possible warnings
		forks.back().commit();

		// Construct the necessary path
		NodeVector<Node> path = field->pathTo(primitiveField, logger());
		createPath(fieldIdx, path, parent);

		// Then create the primitive element
		parent->createChildDocumentPrimitive(text);
		return true;
	}

	// No field was found that might take the data -- dump the error messages
	// from the loggers -- or, if there were no primitive fields, clearly state
	// this fact
	Variant text = readData();
	if (defaultFields.empty()) {
		logger().error("Got data, but field \"" +
		                   field->getNameOrDefaultName() +
		                   "\" of structure \"" + name() +
		                   "\" does not have any primitive field",
		               text);
	} else {
		logger().error("Could not read data with any of the possible fields:",
		               text);
		size_t f = 0;
		for (auto field : defaultFields) {
			logger().note(std::string("Field ") +
			                  Utils::join(field->path(), ".") +
			                  std::string(":"),
			              SourceLocation{}, MessageMode::NO_CONTEXT);
			forks[f].commit();
			f++;
		}
	}
	return false;
}

namespace States {
const State Document = StateBuilder()
                           .parent(&None)
                           .createdNodeType(&RttiTypes::Document)
                           .elementHandler(DocumentHandler::create)
                           .arguments({Argument::String("name", "")});

const State DocumentChild = StateBuilder()
                                .parents({&Document, &DocumentChild})
                                .createdNodeTypes({&RttiTypes::StructureNode,
                                                   &RttiTypes::AnnotationEntity,
                                                   &RttiTypes::DocumentField})
                                .elementHandler(DocumentChildHandler::create)
                                .supportsAnnotations(true)
                                .supportsTokens(true);
}
}

namespace RttiTypes {
const Rtti DocumentField = RttiBuilder<ousia::parser_stack::DocumentField>(
                               "DocumentField").parent(&Node);
}
}
