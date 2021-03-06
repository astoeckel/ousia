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
 * Contains the Handler instances used for parsing actual documents. This file
 * declares to classes: The Document handler which parses the "document" command
 * that introduces a new document and the "DocumentChildHandler" which parses
 * the actual user defined tags.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_DOCUMENT_HANDLER_HPP_
#define _OUSIA_PARSER_STACK_DOCUMENT_HANDLER_HPP_

#include <core/common/Variant.hpp>
#include <core/model/Node.hpp>

#include "Handler.hpp"

namespace ousia {

// Forward declarations
class Rtti;
class DocumentEntity;
class FieldDescriptor;

namespace parser_stack {
/**
 * The DocumentHandler class parses the "document" tag that is used to introduce
 * a new document. Note that this tag is not mandatory in osml files -- if the
 * first command is not a typesystem, ontology or any other declarative command,
 * the DocumentHandler will be implicitly called.
 */
class DocumentHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool startCommand(Variant::mapType &args) override;
	void end() override;

	/**
	 * Creates a new instance of the ImportHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new DocumentHandler{handlerData};
	}
};

/**
 * Temporary Node that is being pushed onto the ParserScope in order to indicate
 * the field the parser is currently in. The name of the Node is stored in the
 * "name" field of the parent Node class.
 *
 * TODO: Invent some common base class for the DocumentField, StructuredEntity
 * and AnnotationEntity classes.
 */
class DocumentField : public Node {
public:
	const size_t fieldIdx;
	const bool transparent;
	const bool explicitField;

	/**
	 * Constructor of the DocumentField class.
	 *
	 * @param mgr is the parent Manager instance.
	 * @param parent is the structure the field belongs to.
	 * @param fieldIdx is the index of the field within the parent fields.
	 * @param transparent is set to true if this field has been created as part
	 * of an implicitly created structure.
	 * @param explicitField is set to true if the field has been created as part
	 * of an explicit field reference.
	 */
	DocumentField(Manager &mgr, Handle<Node> parent, size_t fieldIdx,
	              bool transparent, bool explicitField);

	/**
	 * Returns the FieldDescriptor represented by this DocumentField instance.
	 */
	Rooted<FieldDescriptor> getDescriptor();
};

/**
 * The DocumentChildHandler class performs the actual parsing of the user
 * defined elements in an Ousía document.
 */
class DocumentChildHandler : public Handler {
private:
	/**
	 * If set to true, this handler represents an explicit field.
	 */
	bool isExplicitField : 1;

	/**
	 * Set to false, if this handler was started from a token and is not greedy.
	 * True otherwise.
	 */
	bool isGreedy : 1;

	/**
	 * Set to true if the handler currently is in an implicit field.
	 */
	bool inImplicitDefaultField : 1;

	/**
	 * Registers all user defined tokens in the parser.
	 */
	void registerUserDefinedTokens();

	/**
	 * Code shared by the startCommand(), startAnnotation(), fieldStart() and
	 * the data() method. Checks whether the parser currently is in a field and
	 * returns the name of this field.
	 *
	 * @param parentNode is the next possible parent node (a document,
	 * a structured entity, an annotation entity or a field).
	 * @param fieldIdx is an output parameter to which the index of the current
	 * field is written (or unchanged if we're not in a field).
	 * @param parent is an output parameter to which the parent document entity
	 * will be written.
	 */
	void preamble(Rooted<Node> &parentNode, size_t &fieldIdx,
	              DocumentEntity *&parent);

	/**
	 * Creates transparent elements that are stored in the given path.
	 *
	 * @param path   a NodeVector of alternating FieldDescriptors and
	 *               StructuredClasses. For each of the StructuredClasses at
	 *               index p an instance is created and added to the field at
	 *               index p-1 of the previously created instance of the
	 *               StructuredClass at index p-2.
	 * @param parent is the parent DocumentEntity for the first transparent
	 *               element. This will be reset for each created transparent
	 *               element.
	 * @param p0     is the index of the path vector of the first
	 *               StructuredClass for which an instance shall be created.
	 *               This is 1 per default.
	 */
	void createPath(const NodeVector<Node> &path, DocumentEntity *&parent,
	                size_t p0 = 1);

	/**
	 * Creates transparent elements that are stored in the given path.
	 *
	 * @param fieldIdx  is the index of the field for which the first instance
	 *                  shall be added.
	 * @param path      a NodeVector of alternating FieldDescriptors and
	 *                  StructuredClasses. For each of the StructuredClasses at
	 *                  index p an instance is created and added to the field at
	 *                  index p-1 of the previously created instance of the
	 *                  StructuredClass at index p-2. The first element has
	 *                  to be a StructuredClass.
	 * @param parent    is the parent DocumentEntity for the first transparent
	 *                  element. This will be reset for each created transparent
	 *                  element.
	 */
	void createPath(const size_t &fieldIdx, const NodeVector<Node> &path,
	                DocumentEntity *&parent);

	/**
	 * Removes the transparent elements created by the createPath() method from
	 * the Scope.
	 */
	void rollbackPath();

	/**
	 * Tries to convert the given data to the type that is specified in the
	 * given primitive field.
	 *
	 * @param field is the primitive field for which the data is intended.
	 * @param data is the is the data that should be converted, the result is
	 * written into this argument as output variable.
	 * @param logger is the Logger instance to which error messages should be
	 * written. Needed to allow the convertData function to write to a forked
	 * Logger instance.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool convertData(Handle<FieldDescriptor> field, Variant &data,
	                 Logger &logger);

	/**
	 * Fetches the top-most non-transparent descriptor from the scope, gets the
	 * permitted tokens of this descriptor and filters them to those tokens
	 * which are actually possible, according to the current content of the
	 * ParserScope.
	 *
	 * @param fieldDescr is the field descriptor of which the tokens should be
	 * registered.
	 */
	void pushScopeTokens();

	/**
	 * Pushes a new DocumentField onto the scope stack and registers all
	 * premitted tokens in the parser.
	 */
	void pushDocumentField(Handle<Node> parent,
	                       Handle<FieldDescriptor> fieldDescr, size_t fieldIdx,
	                       bool transparent, bool explicitField);

	/**
	 * Pops a DocumentField from the scope stack and retracts the permitted
	 * tokens from the parser.
	 */
	void popDocumentField();

public:
	DocumentChildHandler(const HandlerData &handlerData);

	bool startCommand(Variant::mapType &args) override;
	bool startAnnotation(Variant::mapType &args) override;
	bool startToken(Handle<Node> node, bool greedy) override;
	EndTokenResult endToken(Handle<Node> node, size_t maxStackDepth) override;
	void end() override;
	bool data() override;
	bool fieldStart(bool &isDefault, bool isImplicit, size_t fieldIdx) override;
	void fieldEnd() override;

	/**
	 * Creates a new instance of the DocumentChildHandler.
	 *
	 * @param handlerData is the data that is passed to the constructor of the
	 * Handler base class and used there to e.g. access the ParserContext and
	 * the Callbacks instance.
	 */
	static Handler *create(const HandlerData &handlerData)
	{
		return new DocumentChildHandler{handlerData};
	}
};

namespace States {
/**
 * State constant representing the "document" tag.
 */
extern const State Document;

/**
 * State contstant representing any user-defined element within a document.
 */
extern const State DocumentChild;
}
}

namespace RttiTypes {
/**
 * RttiType for the internally used DocumentField class.
 */
extern const Rtti DocumentField;
}
}

#endif /* _OUSIA_PARSER_STACK_DOCUMENT_HANDLER_HPP_ */

