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
 * first command is not a typesystem, domain or any other declarative command,
 * the DocumentHandler will be implicitly called.
 */
class DocumentHandler : public StaticHandler {
public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
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
 */
class DocumentField : public Node {
public:
	using Node::Node;
};

/**
 * The DocumentChildHandler class performs the actual parsing of the user
 * defined elements in an Ousía document.
 */
class DocumentChildHandler : public StaticHandler {
private:
	/**
	 * Code shared by both the start() and the end() method. Checks whether the
	 * parser currently is in a field and returns the name of this field.
	 *
	 * @param parentNode is the next possible parent node (a document,
	 * a structured entity, an annotation entity or a field).
	 * @param fieldName is an output parameter to which the name of the current
	 * field is written (or unchanged if we're not in a field).
	 * @param parent is an output parameter to which the parent document entity
	 * will be written.
	 * @param inField is set to true if we actually are in a field.
	 */
	void preamble(Handle<Node> parentNode, std::string &fieldName,
	              DocumentEntity *&parent, bool &inField);

	/**
	 * Constructs all structured entites along the given path and inserts them
	 * into the document graph.
	 *
	 * @param path is a path containing an alternating series of structured
	 * classes and fields.
	 * @pram parent is the root entity from which the process should be started.
	 */
	void createPath(const NodeVector<Node> &path, DocumentEntity *&parent);

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

public:
	using StaticHandler::StaticHandler;

	bool start(Variant::mapType &args) override;
	void end() override;
	bool data(Variant &data) override;

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

