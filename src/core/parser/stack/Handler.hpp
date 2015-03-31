/*
    Ousía
    Copyright (C) 2014, 2015 Benjamin Paaßen, Andreas Stöckel

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
 * @file Handler.hpp
 *
 * Contains the definition of the Handler class, used for representing Handlers
 * for certain syntactic elements.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_PARSER_STACK_HANDLER_HPP_
#define _OUSIA_PARSER_STACK_HANDLER_HPP_

#include <string>

#include <core/common/Location.hpp>
#include <core/common/Variant.hpp>
#include <core/common/Whitespace.hpp>
#include <core/common/Token.hpp>
#include <core/model/Node.hpp>
#include <core/model/Syntax.hpp>

namespace ousia {

// Forward declarations
class ParserScope;
class ParserContext;
class Logger;
class TokenizedData;
class Variant;

namespace parser_stack {

// More forward declarations
class HandlerCallbacks;
class State;

/**
 * Enum describing the type of the Handler instance -- a document handler may
 * be created for handling a simple command, a token or an annotation start and
 * end.
 */
enum class HandlerType { COMMAND, ANNOTATION_START, ANNOTATION_END, TOKEN };

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
	 * Reference at a class implementing the HandlerCallbacks interface, used
	 * for modifying the behaviour of the parser (like registering tokens,
	 * setting the data type or changing the whitespace handling mode).
	 */
	HandlerCallbacks &callbacks;

	/**
	 * Contains the current state of the state machine.
	 */
	const State &state;

	/**
	 * Token containing the name of the command that is being handled, the
	 * location of the element in the source code or the token id of the token
	 * that is being handled.
	 */
	Token token;

	/**
	 * Type describing for which purpose the HandlerData instance was created.
	 */
	HandlerType type;

	/**
	 * Constructor of the HandlerData class.
	 *
	 * @param ctx is the parser context the handler should be executed in.
	 * @param callbacks is an instance of Callbacks used to notify
	 * the parser about certain state changes.
	 * @param state is the state this handler was called for.
	 * @param token contains name, token id and location of the command that is
	 * being handled.
	 * @param type describes the purpose of the Handler instance at hand.
	 */
	HandlerData(ParserContext &ctx, HandlerCallbacks &callbacks,
	            const State &state, const Token &token, HandlerType type);
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

	/**
	 * Current size of the internal "token" stack.
	 */
	size_t tokenStackDepth;

protected:
	/**
	 * Constructor of the Handler class.
	 *
	 * @param data is a structure containing all data being passed to the
	 * handler.
	 */
	Handler(const HandlerData &handlerData);

	/**
	 * Calls the corresponding function in the HandlerCallbacks instance. This
	 * method registers the given tokens as tokens that are generally available,
	 * tokens must be explicitly enabled using the "pushTokens" and "popTokens"
	 * method. Tokens that have not been registered are not guaranteed to be
	 * reported (except for special tokens, these do not have to be registerd).
	 *
	 * @param token is the token string that should be made available.
	 * @return the TokenId that will be used to refer to the token.
	 */
	TokenId registerToken(const std::string &token);

	/**
	 * Calls the corresponding function in the HandlerCallbacks instance. This
	 * method unregisters the given token. Note that for a token to be no longer
	 * reported, this function has to be called as many times as registerToken()
	 * for the corresponding token.
	 *
	 * @param id is the id of the Token that should be unregistered.
	 */
	void unregisterToken(TokenId id);

	/**
	 * Pushes a list of TokenSyntaxDescriptor instances onto the internal stack.
	 * The tokens described in the token list are the tokens that are currently
	 * enabled.
	 *
	 * @param tokens is a list of TokenSyntaxDescriptor instances that should be
	 * stored on the stack.
	 */
	void pushTokens(const std::vector<SyntaxDescriptor> &tokens);

	/**
	 * Calls the corresponding function in the HandlerCallbacks instance.
	 * Removes the previously pushed list of tokens from the stack.
	 */
	void popTokens();

	/**
	 * Calls the corresponding method in the HandlerCallbacks instance. Reads a
	 * string variant form the current input stream. This function must be
	 * called from the data() method.
	 *
	 * @return a string variant containing the current text data. The return
	 * value depends on the currently set whitespace mode and the tokens that
	 * were enabled using the enableTokens callback method.
	 */
	Variant readData();

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
	//	void pushWhitespaceMode(WhitespaceMode whitespaceMode);

	/**
	 * Pops a previously pushed whitespace mode. Calls to this function should
	 * occur in the "end" callback and the "fieldEnd" callback. This function
	 * can only undo pushs that were performed by the pushWhitespaceMode()
	 * method of the same handler.
	 */
	//	void popWhitespaceMode();

public:
	/**
	 * Enum type representing the possible outcomes of the endToken() method.
	 */
	enum class EndTokenResult { ENDED_THIS, ENDED_HIDDEN, ENDED_NONE };

	/**
	 * Virtual destructor.
	 */
	virtual ~Handler();

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
	 * Returns the name of the command or annotation the handler is currently
	 * handling. In case the command is currently handling a token, the name
	 * corresponds to the token string sequence.
	 *
	 * @return the name of the command or the string sequence of the token that
	 * is being handled by this handler.
	 */
	const std::string &name() const;

	/**
	 * Returns the token id of the token that is currently being handled by the
	 * handler. In case the handler currently handles a command or annotation,
	 * the token id is set to Tokens::Data.
	 *
	 * @return the current token id or Tokens::Data if no token is being
	 * handled.
	 */
	TokenId tokenId() const;

	/**
	 * Returns a reference at the Token instance, containing either the token
	 * that is currently being handled or the name of the command and annotation
	 * and their location.
	 *
	 * @return a const reference at the internal token instance.
	 */
	const Token &token() const;

	/**
	 * Returns the location of the element in the source file, for which this
	 * Handler was created.
	 *
	 * @return the location of the Handler in the source file.
	 */
	const SourceLocation &location() const;

	/**
	 * Returns the type describing the purpose for which the handler instance
	 * was created.
	 */
	HandlerType type() const;

	/**
	 * Returns a reference at the State descriptor for which this Handler was
	 * created.
	 *
	 * @return a const reference at the constructing State descriptor.
	 */
	const State &state() const;

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
	 * Called whenever the handler should handle the start of a command. This
	 * method (or any other of the "start" methods) is called exactly once,
	 * after the constructor. The name of the command that is started here can
	 * be accessed using the name() method.
	 *
	 * @param args is a map from strings to variants (argument name and value).
	 * @return true if the handler was successful in starting an element with
	 * the given name represents, false otherwise.
	 */
	virtual bool startCommand(Variant::mapType &args) = 0;

	/**
	 * Called whenever the handler should handle the start of an annotation.
	 * This method (or any other of the "start" methods) is called exactly once,
	 * after the constructor. This method is only called if the
	 * "supportsAnnotations" flag of the State instance referencing this Handler
	 * is set to true. The name of the command that is started here can be
	 * accessed using the name() method.
	 *
	 * @param args is a map from strings to variants (argument name and value).
	 */
	virtual bool startAnnotation(Variant::mapType &args) = 0;

	/**
	 * Called whenever the handler should handle the start of a token. This
	 * method (or any other of the "start" methods) is called exactly once,
	 * after the constructor. This method is only called if the "supportsTokens"
	 * flag of the State instance referencing this Handler is set to true. The
	 * token id of the token that is should be handled can be accessed using the
	 * tokenId() method.
	 *
	 * @param node is the node for which this token was registered.
	 */
	virtual bool startToken(Handle<Node> node) = 0;

	/**
	 * Called whenever a token is marked as "end" token and this handler happens
	 * to be the currently active handler. This operation may have three
	 * outcomes:
	 * <ol>
	 *   <li>The token marks the end of the complete handler and the calling
	 *   code should call the "end" method.</li>
	 *   <li>The token marks the end of some element that is unknown the calling
	 *   code. So the operation itself was a success, but the calling code
	 *   should not call the "end" method.
	 *   <li>The token did not match anything in this context. Basically this
	 *   should never happen, but who knows.</li>
	 * </ol>
	 *
	 * @param id is the Token for which the handler should be started.
	 * @param node is the node for which this token was registered.
	 */
	virtual EndTokenResult endToken(const Token &token, Handle<Node> node) = 0;

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
	 * Called whenever raw data (int the form of a string) is available for the
	 * Handler instance. Should return true if the data could be handled, false
	 * otherwise. The actual data variant must be retrieved using the "text()"
	 * callback.
	 *
	 * @return true if the data could be handled, false otherwise.
	 */
	virtual bool data() = 0;
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
	bool startCommand(Variant::mapType &args) override;
	bool startAnnotation(Variant::mapType &args) override;
	bool startToken(Handle<Node> node) override;
	EndTokenResult endToken(const Token &token, Handle<Node> node) override;
	void end() override;
	bool fieldStart(bool &isDefault, size_t fieldIdx) override;
	void fieldEnd() override;
	bool data() override;

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
	bool startCommand(Variant::mapType &args) override;
	bool startAnnotation(Variant::mapType &args) override;
	bool startToken(Handle<Node> node) override;
	EndTokenResult endToken(const Token &token, Handle<Node> node) override;
	void end() override;
	bool fieldStart(bool &isDefault, size_t fieldIdx) override;
	void fieldEnd() override;
	bool data() override;
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
	virtual void doHandle(const Variant &fieldData, Variant::mapType &args) = 0;

public:
	bool startCommand(Variant::mapType &args) override;
	bool data() override;
	void end() override;
};
}
}

#endif /* _OUSIA_PARSER_STACK_HANDLER_HPP_ */

