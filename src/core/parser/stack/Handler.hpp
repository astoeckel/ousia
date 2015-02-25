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

#ifndef _OUSIA_PARSER_STACK_HANDLER_HPP_
#define _OUSIA_PARSER_STACK_HANDLER_HPP_

#include <string>

#include <core/common/Location.hpp>
#include <core/common/Variant.hpp>
#include <core/common/Whitespace.hpp>

namespace ousia {

// Forward declarations
class ParserScope;
class ParserContext;
class Logger;
class TokenizedData;

namespace parser_stack {

// More forward declarations
class Callbacks;
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
	//	Callbacks &callbacks;

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
	HandlerData(ParserContext &ctx,
	            /*Callbacks &callbacks,*/ const std::string &name,
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
	const HandlerData handlerData;

	/**
	 * Reference at the current logger. If not nullptr, this will override the
	 * logger from the ParserContext specified in the handlerData.
	 */
	Logger *internalLogger;

protected:
	/**
	 * Constructor of the Handler class.
	 *
	 * @param data is a structure containing all data being passed to the
	 * handler.
	 */
	Handler(const HandlerData &handlerData);

	/**
	 * Returns a reference at the ParserContext.
	 *
	 * @return a reference at the ParserContext.
	 */
	ParserContext &context();

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
	 * Returns the location of the element in the source file, for which this
	 * Handler was created.
	 *
	 * @return the location of the Handler in the source file.
	 */
	const SourceLocation &location() const;

	/**
	 * Returns the command name for which the handler was created.
	 *
	 * @return a const reference at the command name.
	 */
	const std::string &name() const;

	/**
	 * Calls the corresponding function in the Callbacks instance. Sets the
	 * whitespace mode that specifies how string data should be processed. The
	 * calls to this function are placed on a stack by the underlying Stack
	 * class. This function should be called from the "fieldStart" callback and
	 * the "start" callback. If no whitespace mode is pushed in the "start"
	 * method the whitespace mode "TRIM" is implicitly assumed.
	 *
	 * @param whitespaceMode specifies one of the three WhitespaceMode constants
	 * PRESERVE, TRIM or COLLAPSE.
	 */
	void pushWhitespaceMode(WhitespaceMode whitespaceMode);

	/**
	 * Pops a previously pushed whitespace mode. Calls to this function should
	 * occur in the "end" callback and the "fieldEnd" callback. This function
	 * can only undo pushs that were performed by the pushWhitespaceMode()
	 * method of the same handler.
	 */
	void popWhitespaceMode();

	/**
	 * Calls the corresponding function in the Callbacks instance. Sets the
	 * whitespace mode that specifies how string data should be processed. The
	 * calls to this function are placed on a stack by the underlying Stack
	 * class. This function should be called from the "fieldStart" callback and
	 * the "start" callback. If no whitespace mode is pushed in the "start"
	 * method the whitespace mode "TRIM" is implicitly assumed.
	 *
	 * @param tokens is a list of tokens that should be reported to this handler
	 * instance via the "token" method.
	 */
	void pushTokens(const std::vector<std::string> &tokens);

	/**
	 * Pops a previously pushed whitespace mode. Calls to this function should
	 * occur in the "end" callback and the "fieldEnd" callback. This function
	 * can only undo pushs that were performed by the pushWhitespaceMode()
	 * method of the same handler.
	 */
	void popWhitespaceMode();


	/**
	 * Calls the corresponding function in the Callbacks instance. This method
	 * registers the given tokens as tokens that are generally available, tokens
	 * must be explicitly enabled using the "pushTokens" and "popTokens" method.
	 * Tokens that have not been registered are not guaranteed to be reported,
	 * even though they are 
	 */
	void registerTokens(const std::vector<std::string> &tokens);

public:
	/**
	 * Virtual destructor.
	 */
	virtual ~Handler();

	/**
	 * Returns the command name for which the handler was created.
	 *
	 * @return a const reference at the command name.
	 */
	const std::string &getName() const;

	/**
	 * Reference at the State descriptor for which this Handler was created.
	 *
	 * @return a const reference at the constructing State descriptor.
	 */
	const State &getState() const;

	/**
	 * Sets the internal logger to the given logger instance.
	 *
	 * @param logger is the Logger instance to which the logger should be set.
	 */
	void setLogger(Logger &logger);

	/**
	 * Resets the logger instance to the logger instance provided in the
	 * ParserContext.
	 */
	void resetLogger();

	/**
	 * Returns the location of the element in the source file, for which this
	 * Handler was created.
	 *
	 * @return the location of the Handler in the source file.
	 */
	const SourceLocation &getLocation() const;

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
	 * @param isDefault is set to true if the field that is being started is the
	 * default/tree field. The handler should set the value of this variable to
	 * true if the referenced field is indeed the default field.
	 * @param fieldIdx is the numerical index of the field.
	 */
	virtual bool fieldStart(bool &isDefault, size_t fieldIdx) = 0;

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
	virtual bool annotationStart(const Variant &className,
	                             Variant::mapType &args) = 0;

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
	virtual bool annotationEnd(const Variant &className,
	                           const Variant &elementName) = 0;

	/**
	 * Called whenever raw data (int the form of a string) is available for the
	 * Handler instance. Should return true if the data could be handled, false
	 * otherwise.
	 *
	 * @param data is an instance of TokenizedData containing the segmented
	 * character data and its location.
	 * @return true if the data could be handled, false otherwise.
	 */
	virtual bool data(TokenizedData &data) = 0;
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
 * The EmptyHandler class is used in case no element handler is specified in
 * the State descriptor. It just accepts all data and does nothing.
 */
class EmptyHandler : public Handler {
protected:
	using Handler::Handler;

public:
	bool start(Variant::mapType &args) override;
	void end() override;
	bool fieldStart(bool &isDefault, size_t fieldIdx) override;
	void fieldEnd() override;
	bool annotationStart(const Variant &className,
	                     Variant::mapType &args) override;
	bool annotationEnd(const Variant &className,
	                   const Variant &elementName) override;
	bool data(TokenizedData &data) override;

	/**
	 * Creates an instance of the EmptyHandler class.
	 */
	static Handler *create(const HandlerData &handlerData);
};

/**
 * The StaticHandler class is used to handle predifined commands which do
 * neither support annotations, nor multiple fields. Child classes can decide
 * whether a single data field should be used.
 */
class StaticHandler : public Handler {
protected:
	using Handler::Handler;

public:
	bool start(Variant::mapType &args) override;
	void end() override;
	bool fieldStart(bool &isDefault, size_t fieldIdx) override;
	void fieldEnd() override;
	bool annotationStart(const Variant &className,
	                     Variant::mapType &args) override;
	bool annotationEnd(const Variant &className,
	                   const Variant &elementName) override;
	bool data(TokenizedData &data) override;
};

/**
 * The StaticFieldHandler class is used to handle predifined commands which do
 * neither support annotations, nor multiple fields. Additionally, it captures a
 * data entry from a single default field.
 */
class StaticFieldHandler : public StaticHandler {
private:
	/**
	 * Set to the name of the data argument that should be used instead of the
	 * data field, if no data field is given.
	 */
	std::string argName;

	/**
	 * Set to true, once the "doHandle" function has been called.
	 */
	bool handled;

	/**
	 * Map containing the arguments given in the start function.
	 */
	Variant::mapType args;

protected:
	/**
	 * Constructor of the StaticFieldHandler class.
	 *
	 * @param handlerData is a structure containing the internal data that
	 * should be stored inside the handler.
	 * @param name of the data argument that -- if present -- should be used
	 * instead of the data field. If empty, data is not captured from the
	 * arguments. If both, data in the data field and the argument, are given,
	 * this results in an error.
	 */
	StaticFieldHandler(const HandlerData &handlerData,
	                   const std::string &argName);

	/**
	 * Function that should be overriden in order to handle the field data and
	 * the other arguments. This function is not called if no data was given.
	 *
	 * @param fieldData is the captured field data.
	 * @param args are the arguments that were given in the "start" function.
	 */
	virtual void doHandle(const Variant &fieldData,
	                      Variant::mapType &args) = 0;

public:
	bool start(Variant::mapType &args) override;
	void end() override;
	bool data(TokenizedData &data) override;
};
}
}

#endif /* _OUSIA_PARSER_STACK_HANDLER_HPP_ */

