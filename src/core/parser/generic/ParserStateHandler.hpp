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
	 * Reference at an instance of the ParserStateCallbacks class, used for
	 * modifying the behaviour of the parser (like registering tokens, setting
	 * the data type or changing the whitespace handling mode).
	 */
	ParserStateCallbacks &callbacks;

	/**
	 * Contains the name of the command that is being handled.
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
	 * @param callbacks is an instance of ParserStateCallbacks used to notify
	 * the parser about certain state changes.
	 * @param name is the name of the string.
	 * @param state is the state this handler was called for.
	 * @param parentState is the state of the parent command.
	 * @param location is the location at which the handler is created.
	 */
	ParserStateHandlerData(ParserContext &ctx, ParserStateCallbacks &callbacks,
	                       std::string name, const ParserState &state,
	                       const ParserState &parentState,
	                       const SourceLocation &location);
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
	ParserContext &context();

	/**
	 * Returns the command name for which the handler was created.
	 *
	 * @return a const reference at the command name.
	 */
	const std::string &name();

	/**
	 * Returns a reference at the ParserScope instance.
	 *
	 * @return a reference at the ParserScope instance.
	 */
	ParserScope &scope();

	/**
	 * Returns a reference at the Manager instance which manages all nodes.
	 *
	 * @return a referance at the Manager instance.
	 */
	Manager &manager();

	/**
	 * Returns a reference at the Logger instance used for logging error
	 * messages.
	 *
	 * @return a reference at the Logger instance.
	 */
	Logger &logger();

	/**
	 * Returns a reference at the Project Node, representing the project into
	 * which the file is currently being parsed.
	 *
	 * @return a referance at the Project Node.
	 */
	Rooted<Project> project();

	/**
	 * Reference at the ParserState descriptor for which this Handler was
	 * created.
	 *
	 * @return a const reference at the constructing ParserState descriptor.
	 */
	const ParserState &state();

	/**
	 * Returns the current location in the source file.
	 *
	 * @return the current location in the source file.
	 */
	SourceLocation location();

	/**
	 * Calls the corresponding function in the ParserStateCallbacks instance.
	 * Sets the whitespace mode that specifies how string data should be
	 * processed.
	 *
	 * @param whitespaceMode specifies one of the three WhitespaceMode constants
	 * PRESERVE, TRIM or COLLAPSE.
	 */
	void setWhitespaceMode(WhitespaceMode whitespaceMode);

	/**
	 * Calls the corresponding function in the ParserStateCallbacks instance.
	 * Sets the type as which the variant data should be parsed.
	 *
	 * @param type is one of the VariantType constants, specifying with which
	 * type the data that is passed to the ParserStateHandler in the "data"
	 * function should be handled.
	 */
	void setDataType(VariantType type);

	/**
	 * Calls the corresponding function in the ParserStateCallbacks instance.
	 * Checks whether the given token is supported by the parser. The parser
	 * returns true, if the token is supported, false if this token cannot be
	 * registered. Note that parsers that do not support the registration of
	 * tokens at all should always return "true".
	 *
	 * @param token is the token that should be checked for support.
	 * @return true if the token is generally supported (or the parser does not
	 * support registering tokens at all), false if the token is not supported,
	 * because e.g. it is a reserved token or it interferes with other tokens.
	 */
	bool supportsToken(const std::string &token);

	/**
	 * Calls the corresponding function in the ParserStateCallbacks instance.
	 * Registers the given token as token that should be reported to the handler
	 * using the "token" function.
	 *
	 * @param token is the token string that should be reported.
	 */
	void registerToken(const std::string &token);

	/**
	 * Calls the corresponding function in the ParserStateCallbacks instance.
	 * Unregisters the given token, it will no longer be reported to the handler
	 * using the "token" function.
	 *
	 * @param token is the token string that should be unregistered.
	 */
	void unregisterToken(const std::string &token);

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

