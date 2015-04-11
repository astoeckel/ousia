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

#include <cassert>

#include <core/common/Exceptions.hpp>
#include <core/common/Logger.hpp>
#include <core/common/Variant.hpp>
#include <core/parser/utils/TokenizedData.hpp>
#include <core/parser/ParserContext.hpp>

#include "Callbacks.hpp"
#include "Handler.hpp"
#include "State.hpp"

namespace ousia {
namespace parser_stack {

/* Class HandlerData */

HandlerData::HandlerData(ParserContext &ctx, HandlerCallbacks &callbacks,
                         const State &state, const Token &token,
                         HandlerType type)
    : ctx(ctx), callbacks(callbacks), state(state), token(token), type(type)
{
}

/* Class Handler */

Handler::Handler(const HandlerData &handlerData)
    : handlerData(handlerData), internalLogger(nullptr), tokenStackDepth(0)
{
}

Handler::~Handler()
{
	while (tokenStackDepth > 0) {
		popTokens();
	}
}

ParserContext &Handler::context() { return handlerData.ctx; }

ParserScope &Handler::scope() { return handlerData.ctx.getScope(); }

Manager &Handler::manager() { return handlerData.ctx.getManager(); }

Logger &Handler::logger()
{
	if (internalLogger != nullptr) {
		return *internalLogger;
	}
	return handlerData.ctx.getLogger();
}

const std::string &Handler::name() const { return handlerData.token.content; }

TokenId Handler::tokenId() const { return handlerData.token.id; }

const Token &Handler::token() const { return handlerData.token; }

const SourceLocation &Handler::location() const
{
	return handlerData.token.location;
}

HandlerType Handler::type() const { return handlerData.type; }

const State &Handler::state() const { return handlerData.state; }

Variant Handler::readData() { return handlerData.callbacks.readData(); }

void Handler::pushTokens(const std::vector<SyntaxDescriptor> &tokens)
{
	tokenStackDepth++;
	handlerData.callbacks.pushTokens(tokens);
}

void Handler::popTokens()
{
	assert(tokenStackDepth > 0 && "popTokens called too often");
	tokenStackDepth--;
	handlerData.callbacks.popTokens();
}

TokenId Handler::registerToken(const std::string &token)
{
	return handlerData.callbacks.registerToken(token);
}

void Handler::unregisterToken(TokenId id)
{
	handlerData.callbacks.unregisterToken(id);
}

void Handler::setLogger(Logger &logger) { internalLogger = &logger; }

void Handler::resetLogger() { internalLogger = nullptr; }

const SourceLocation &Handler::getLocation() const { return location(); }

/* Class EmptyHandler */

bool EmptyHandler::startCommand(Variant::mapType &args)
{
	// Well, we'll support any command we get, don't we?
	return true;
}

bool EmptyHandler::startAnnotation(Variant::mapType &args)
{
	// Do not support annotations. Annotations are too complicated for poor
	// EmptyHandler.
	return false;
}

bool EmptyHandler::startToken(Handle<Node> node)
{
	// EmptyHandler does not support tokens.
	return false;
}

EndTokenResult EmptyHandler::endToken(Handle<Node> node, size_t maxStackDepth)
{
	// There are no tokens to end here.
	return EndTokenResult();
}

void EmptyHandler::end()
{
	// Do nothing if a command ends
}

bool EmptyHandler::fieldStart(bool &isDefaultField, size_t fieldIndex)
{
	// Accept any field
	return true;
}

void EmptyHandler::fieldEnd()
{
	// Do not handle field ends
}

bool EmptyHandler::data()
{
	// Support any data
	return true;
}

Handler *EmptyHandler::create(const HandlerData &handlerData)
{
	return new EmptyHandler(handlerData);
}

/* Class StaticHandler */

bool StaticHandler::startCommand(Variant::mapType &args)
{
	// Do nothing in the default implementation, accept anything
	return true;
}

bool StaticHandler::startAnnotation(Variant::mapType &args) { return false; }

bool StaticHandler::startToken(Handle<Node> node) { return false; }

EndTokenResult StaticHandler::endToken(Handle<Node> node, size_t maxStackDepth)
{
	// There are no tokens to end here.
	return EndTokenResult();
}

void StaticHandler::end()
{
	// Do nothing here
}

bool StaticHandler::fieldStart(bool &isDefault, size_t fieldIdx)
{
	// Return true if either the default field is requested or the field index
	// is zero. This simulates that there is exactly one field (a default field)
	if (fieldIdx == 0) {
		isDefault = true;
		return true;
	}
	return false;
}

void StaticHandler::fieldEnd()
{
	// Do nothing here
}

bool StaticHandler::data()
{
	logger().error("Did not expect any data here", readData());
	return false;
}

/* Class StaticFieldHandler */

StaticFieldHandler::StaticFieldHandler(const HandlerData &handlerData,
                                       const std::string &argName)
    : StaticHandler(handlerData), argName(argName), handled(false)
{
}

bool StaticFieldHandler::startCommand(Variant::mapType &args)
{
	if (!argName.empty()) {
		auto it = args.find(argName);
		if (it != args.end() && !it->second.toString().empty()) {
			handled = true;
			doHandle(it->second, args);
			return true;
		}
	}

	this->args = args;
	return true;
}

void StaticFieldHandler::end()
{
	if (!handled) {
		if (!argName.empty()) {
			logger().error(std::string("Required argument \"") + argName +
			                   std::string("\" is missing."),
			               location());
		} else {
			logger().error("Command requires data, but no data given",
			               location());
		}
	}
}

bool StaticFieldHandler::data()
{
	// Fetch the actual text data
	Variant stringData = readData();

	// Call the doHandle function if this has not been done before
	if (!handled) {
		handled = true;
		doHandle(stringData, args);
		return true;
	}

	// The doHandle function was already called, print an error message
	logger().error(
	    std::string("Found data, but the corresponding argument \"") + argName +
	        std::string("\" was already specified"),
	    stringData);

	// Print the location at which the attribute was originally specified
	auto it = args.find(argName);
	if (it != args.end()) {
		logger().note(std::string("Attribute was specified here:"), it->second);
	}
	return false;
}
}
}

