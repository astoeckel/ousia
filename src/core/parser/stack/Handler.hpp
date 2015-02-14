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

#include <core/common/Location.hpp>
#include <core/common/Variant.hpp>

namespace ousia {

// Forward declarations
class ParserContext;
class Callbacks;
class Logger;
class Project;

namespace parser_stack {

// More forward declarations
class State;

/**
 * Class collecting all the data that is being passed to a Handler
 * instance.
 */
class HandlerData {
public:
	/**
	 * Reference to the ParserContext instance that should be used to resolve
	 * references to nodes in the Graph.
	 */
	ParserContext &ctx;

	/**
	 * Reference at an instance of the Callbacks class, used for
	 * modifying the behaviour of the parser (like registering tokens, setting
	 * the data type or changing the whitespace handling mode).
	 */
	Callbacks &callbacks;

	/**
	 * Contains the name of the command that is being handled.
	 */
	std::string name;

	/**
	 * Contains the current state of the state machine.
	 */
	const State &state;

	/**
	 * Current source code location.
	 */
	SourceLocation location;

	/**
	 * Constructor of the HandlerData class.
	 *
	 * @param ctx is the parser context the handler should be executed in.
	 * @param callbacks is an instance of Callbacks used to notify
	 * the parser about certain state changes.
	 * @param name is the name of the string.
	 * @param state is the state this handler was called for.
	 * @param location is the location at which the handler is created.
	 */
	HandlerData(ParserContext &ctx, Callbacks &callbacks, std::string name,
	            const State &state, const SourceLocation &location);
};

/**
 * The Handler class provides a context for handling a generic stack element.
 * It has to beoverridden and registered in the StateStack class to form
 * handlers for concrete XML tags.
 */
class Handler {
private:
	/**
	 * Structure containing the internal handler data.
	 */
	const HandlerData internalData;

protected:
	/**
	 * Constructor of the Handler class.
	 *
	 * @param data is a structure containing all data being passed to the
	 * handler.
	 */
	Handler(const HandlerData &internalData);

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
	 * Reference at the State descriptor for which this Handler was created.
	 *
	 * @return a const reference at the constructing State descriptor.
	 */
	const State &state();

	/**
	 * Returns the current location in the source file.
	 *
	 * @return the current location in the source file.
	 */
	SourceLocation location();

public:
	/**
	 * Virtual destructor.
	 */
	virtual ~Handler();

	/**
	 * Calls the corresponding function in the Callbacks instance. Sets the
	 * whitespace mode that specifies how string data should be processed. The
	 * calls to this function are placed on a stack by the underlying Stack
	 * class.
	 *
	 * @param whitespaceMode specifies one of the three WhitespaceMode constants
	 * PRESERVE, TRIM or COLLAPSE.
	 */
	void setWhitespaceMode(WhitespaceMode whitespaceMode);

	/**
	 * Calls the corresponding function in the Callbacks instance.
	 * Registers the given token as token that should be reported to the handler
	 * using the "token" function.
	 *
	 * @param token is the token string that should be reported.
	 */
	void registerToken(const std::string &token);

	/**
	 * Calls the corresponding function in the Callbacks instance.
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
	 * @return true if the handler was successful in starting the element it
	 * represents, false otherwise.
	 */
	virtual bool start(Variant::mapType &args) = 0;

	/**
	 * Called before the command for which this handler is defined ends (is
	 * forever removed from the stack).
	 */
	virtual void end() = 0;

	/**
	 * Called when a new field starts, while the handler is active. This
	 * function should return true if the field is supported, false otherwise.
	 * No error should be logged if the field cannot be started, the caller will
	 * take care of that (since it is always valid to start a default field,
	 * even though the corresponding structure does not have a field, as long as
	 * no data is fed into the field).
	 *
	 * @param isDefaultField is set to true if the field that is being started
	 * is the default/tree field. The handler should set the value of this
	 * variable to true if the referenced field is indeed the default field.
	 * @param isImplicit is set to true if the field is implicitly being started
	 * by the stack (this field always implies isDefaultField being set to
	 * true).
	 * @param fieldIndex is the numerical index of the field.
	 */
	virtual bool fieldStart(bool &isDefaultField, bool isImplicit,
	                        size_t fieldIndex) = 0;

	/**
	 * Called when a previously opened field ends, while the handler is active.
	 * Note that a "fieldStart" and "fieldEnd" are always called alternately.
	 */
	virtual void fieldEnd() = 0;

	/**
	 * Called whenever an annotation starts while this handler is active. The
	 * function should return true if starting the annotation was successful,
	 * false otherwise.
	 *
	 * @param className is a string variant containing the name of the
	 * annotation class and the location of the name in the source code.
	 * @param args is a map from strings to variants (argument name and value).
	 * @return true if the mentioned annotation could be started here, false
	 * if an error occurred.
	 */
	virtual bool annotationStart(Variant className, Variant::mapType &args) = 0;

	/**
	 * Called whenever an annotation ends while this handler is active. The 
	 * function should return true if ending the annotation was successful,
	 * false otherwise.
	 *
	 * @param className is a string variant containing the name of the
	 * annotation class and the location of the class name in the source code.
	 * @param elementName is a string variant containing the name of the
	 * annotation class and the location of the element name in the source code.
	 * @return true if the mentioned annotation could be started here, false if
	 * an error occurred.
	 */
	virtual bool annotationEnd(Variant className, Variant elementName) = 0;

	/**
	 * Called whenever raw data (int the form of a string) is available for the
	 * Handler instance.
	 *
	 * @param data is a string variant containing the character data and its
	 * location.
	 */
	virtual void data(Variant data) = 0;
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
 * the State descriptor.
 */
/*class EmptyHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override;

	void end() override;

	static Handler *create(const HandlerData &handlerData);
};*/

}
}

#endif /* _OUSIA_PARSER_STATE_HANDLER_HPP_ */

