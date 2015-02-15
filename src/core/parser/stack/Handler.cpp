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

#include <core/common/Exceptions.hpp>
#include <core/common/Logger.hpp>
#include <core/parser/ParserContext.hpp>

#include "Callbacks.hpp"
#include "Handler.hpp"
#include "State.hpp"

namespace ousia {
namespace parser_stack {

/* Class HandlerData */

HandlerData::HandlerData(ParserContext &ctx, /*Callbacks &callbacks,*/
                         const std::string &name, const State &state,
                         const SourceLocation &location)
    : ctx(ctx),
      /*callbacks(callbacks),*/
      name(name),
      state(state),
      location(location)
{
}

/* Class Handler */

Handler::Handler(const HandlerData &handlerData)
    : handlerData(handlerData), internalLogger(nullptr)
{
}

Handler::~Handler() {}

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

const SourceLocation &Handler::location() const { return handlerData.location; }

const std::string &Handler::name() const { return handlerData.name; }

void Handler::setWhitespaceMode(WhitespaceMode whitespaceMode)
{
	/*handlerData.callbacks.setWhitespaceMode(whitespaceMode);*/
}

void Handler::registerToken(const std::string &token)
{
	/*handlerData.callbacks.registerToken(token);*/
}

void Handler::unregisterToken(const std::string &token)
{
	/*handlerData.callbacks.unregisterToken(token);*/
}

const std::string &Handler::getName() const { return name(); }

const State &Handler::getState() const { return handlerData.state; }

void Handler::setLogger(Logger &logger) { internalLogger = &logger; }

void Handler::resetLogger() { internalLogger = nullptr; }

const SourceLocation &Handler::getLocation() const { return location(); }

/* Class EmptyHandler */

bool EmptyHandler::start(Variant::mapType &args)
{
	// Just accept anything
	return true;
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
	// Do not handle fields
}

bool EmptyHandler::annotationStart(const Variant &className,
                                   Variant::mapType &args)
{
	// Accept any data
	return true;
}

bool EmptyHandler::annotationEnd(const Variant &className,
                                 const Variant &elementName)
{
	// Accept any annotation
	return true;
}

bool EmptyHandler::data(Variant &data)
{
	// Support any data
	return true;
}

Handler *EmptyHandler::create(const HandlerData &handlerData)
{
	return new EmptyHandler(handlerData);
}

/* Class StaticHandler */

bool StaticHandler::start(Variant::mapType &args)
{
	// Do nothing in the default implementation, accept anything
	return true;
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

bool StaticHandler::annotationStart(const Variant &className,
                                    Variant::mapType &args)
{
	// No annotations supported
	return false;
}

bool StaticHandler::annotationEnd(const Variant &className,
                                  const Variant &elementName)
{
	// No annotations supported
	return false;
}

bool StaticHandler::data(Variant &data)
{
	logger().error("Did not expect any data here", data);
	return false;
}

/* Class StaticFieldHandler */

StaticFieldHandler::StaticFieldHandler(const HandlerData &handlerData,
                                       const std::string &argName)
    : StaticHandler(handlerData), argName(argName), handled(false)
{
}

bool StaticFieldHandler::start(Variant::mapType &args)
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

bool StaticFieldHandler::data(Variant &data)
{
	// Call the doHandle function if this has not been done before
	if (!handled) {
		handled = true;
		doHandle(data, args);
		return true;
	}

	// The doHandle function was already called, print an error message
	logger().error(
	    std::string("Found data, but the corresponding argument \"") + argName +
	        std::string("\" was already specified"),
	    data);

	// Print the location at which the attribute was originally specified
	auto it = args.find(argName);
	if (it != args.end()) {
		logger().note(std::string("Attribute was specified here:"), it->second);
	}
	return false;
}
}
}

