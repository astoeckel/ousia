/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#ifndef _OUSIA_PARSER_STATE_HANDLER_HPP_
#define _OUSIA_PARSER_STATE_HANDLER_HPP_

#include <memory>
#include <string>

#include <core/utils/Location.hpp>

namespace ousia {

// Forward declarations
class ParserContext;
class ParserState;
class ParserStateCallbacks;

/**
 * Class collecting all the data that is being passed to a ParserStateHandler
 * instance.
 */
class ParserStateHandlerData {
public:
	/**
	 * Reference to the ParserContext instance that should be used to resolve
	 * references to nodes in the Graph.
	 */
	ParserContext &ctx;

	/**
	 * Contains the name of the tag that is being handled.
	 */
	const std::string name;

	/**
	 * Contains the current state of the state machine.
	 */
	const ParserState &state;

	/**
	 * Contains the state of the state machine when the parent node was handled.
	 */
	const ParserState &parentState;

	/**
	 * Current source code location.
	 */
	const SourceLocation location;

	/**
	 * Constructor of the HandlerData class.
	 *
	 * @param ctx is the parser context the handler should be executed in.
	 * @param name is the name of the string.
	 * @param state is the state this handler was called for.
	 * @param parentState is the state of the parent command.
	 * @param location is the location at which the handler is created.
	 */
	ParserStateHandlerData(ParserContext &ctx, std::string name,
	                       const ParserState &state,
	                       const ParserState &parentState,
	                       const SourceLocation location);
};

/**
 * The handler class provides a context for handling an XML tag. It has to be
 * overridden and registered in the StateStack class to form handlers for
 * concrete XML tags.
 */
class ParserStateHandler {
private:
	/**
	 * Structure containing the internal handler data.
	 */
	const ParserStateHandlerData data;

protected:
	/**
	 * Constructor of the Handler class.
	 *
	 * @param data is a structure containing all data being passed to the
	 * handler.
	 */
	ParserStateHandler(const ParserStateHandlerData &data){};

public:
	/**
	 * Virtual destructor.
	 */
	virtual ~Handler(){};

	/**
	 * Returns a reference at the ParserContext.
	 *
	 * @return a reference at the ParserContext.
	 */
	ParserContext &context() { return handlerData.ctx; }

	/**
	 * Returns the command name for which the handler was created.
	 *
	 * @return a const reference at the command name.
	 */
	const std::string &name() { return handlerData.name; }

	/**
	 * Returns a reference at the ParserScope instance.
	 *
	 * @return a reference at the ParserScope instance.
	 */
	ParserScope &scope() { return handlerData.ctx.getScope(); }

	/**
	 * Returns a reference at the Manager instance which manages all nodes.
	 *
	 * @return a referance at the Manager instance.
	 */
	Manager &manager() { return handlerData.ctx.getManager(); }

	/**
	 * Returns a reference at the Logger instance used for logging error
	 * messages.
	 *
	 * @return a reference at the Logger instance.
	 */
	Logger &logger() { return handlerData.ctx.getLogger(); }

	/**
	 * Returns a reference at the Project Node, representing the project into
	 * which the file is currently being parsed.
	 *
	 * @return a referance at the Project Node.
	 */
	Rooted<Project> project() { return handlerData.ctx.getProject(); }

	/**
	 * Reference at the ParserState descriptor for which this Handler was
	 * created.
	 *
	 * @return a const reference at the constructing ParserState descriptor.
	 */
	const ParserState &state() { return handlerData.state; }

	/**
	 * Reference at the ParserState descriptor of the parent state of the state
	 * for which this Handler was created. Set to ParserStates::None if there
	 * is no parent state.
	 *
	 * @return a const reference at the parent state of the constructing
	 * ParserState descriptor.
	 */
	const ParserState &parentState() { return handlerData.parentState; }

	/**
	 * Returns the current location in the source file.
	 *
	 * @return the current location in the source file.
	 */
	SourceLocation location() { return handlerData.location; }

	/**
	 * Called when the command that was specified in the constructor is
	 * instanciated.
	 *
	 * @param args is a map from strings to variants (argument name and value).
	 */
	virtual void start(Variant::mapType &args) = 0;

	/**
	 * Called whenever the command for which this handler is defined ends.
	 */
	virtual void end() = 0;

	/**
	 * Called whenever raw data (int the form of a string) is available for the
	 * Handler instance. In the default handler an exception is raised if the
	 * received data contains non-whitespace characters.
	 *
	 * @param data is a pointer at the character data that is available for the
	 * Handler instance.
	 * @param field is the field number (the interpretation of this value
	 * depends on the format that is being parsed).
	 */
	virtual void data(const std::string &data, int field);
};

/**
 * HandlerConstructor is a function pointer type used to create concrete
 * instances of the Handler class.
 *
 * @param handlerData is the data that should be passed to the new handler
 * instance.
 * @return a newly created handler instance.
 */
using HandlerConstructor = Handler *(*)(const HandlerData &handlerData);

/**
 * The DefaultHandler class is used in case no element handler is specified in
 * the ParserState descriptor.
 */
class DefaultParserStateHandler : public ParserStateHandler {
public:
	using ParserStateHandler::ParserStateHandler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData);
};
}

#endif /* _OUSIA_PARSER_STATE_HANDLER_HPP_ */

