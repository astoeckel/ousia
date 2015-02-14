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
 * @file GenericParser.hpp
 *
 * The GenericParser class builds an abstraction layer that separates the
 * underlying document format (e.g. osdm or osdmx) from the actual process of
 * building the document model. It provides a set of genric functions that
 * should be called by the inheriting concrete parser class, e.g. indicating a
 * command with parameters, the start/end of a field or the start/end of an
 * annotation. The GenericParser maintains an internal stack of
 * ParserStateHandlers and relays the commands to the elements of this stack.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_GENERIC_PARSER_HPP_
#define _OUSIA_GENERIC_PARSER_HPP_

#include <core/parser/Parser.hpp>

#include "ParserStateStack.hpp"
#include "ParserStateHandler.hpp"
#include "ParserState.hpp"

namespace ousia {

/**
 * The abstract GenericParser class is merely a convenience class for Parsers
 * which use the ParserStateStack class. It maintains a ParserStateStack
 * instance and provides functions which directly forward the given data to the
 * ParserStateStack. It also implements the ParserStateCallbacks inteface which
 * is used by ParserStateHandlers to influence the parsing process (such as
 * setting the whitespace mode or registering new entities).
 */
class GenericParser : public Parser, public ParserStateCallbacks {

private:
	/**
	 * Internal ParserStateStack instance.
	 */
	ParserStateStack stack;

protected:
	/**
	 * Forwards the "command" event to the ParserStateStack instance.
	 *
	 * @param name is the name of the command (including the namespace
	 * separator ':') and its corresponding location. Must be a string variant.
	 * @param args is a map variant containing the arguments that were passed to
	 * the command.
	 */
	void command(Variant name, Variant args)
	{
		stack.command(std::move(name), std::move(args));
	}

	/**
	 * Forwards the "fieldStart" event to the ParserStateStack instance.
	 */
	void fieldStart()
	{
		stack.fieldStart();
	}

	/**
	 * Forwards the "fieldEnd" event to the ParserStateStack instance.
	 */
	void fieldEnd()
	{
		stack.fieldEnd();
	}

	/**
	 * Forwards the "data" event to the ParserStateStack instance.
	 *
	 * @param data is a variant of any type containing the data that was parsed
	 * as data.
	 */
	void data(Variant data)
	{
		stack.data(std::move(data));
	}

	/**
	 * Forwards the "annotationStart" event to the ParserStateStack instance.
	 *
	 * @param name is the name of the annotation class.
	 * @param args is a map variant containing the arguments that were passed
	 * to the annotation.
	 */
	void annotationStart(Variant name, Variant args)
	{
		stack.annotationStart(std::move(name), std::move(args));
	}

	/**
	 * Forwards the "annotationEnd" event to the ParserStateStack instance.
	 *
	 * @param name is the name of the annotation class that was ended.
	 * @param annotationName is the name of the annotation that was ended.
	 */
	void annotationEnd(Variant name, Variant annotationName)
	{
		stack.annotationEnd(std::move(name), std::move(annotationName));
	}

	/**
	 * Forwards the "token" call to the ParserStateStack instance.
	 *
	 * @param token is string variant containing the token that was encountered.
	 */
	void token(Variant token)
	{
		stack.token(std::move(token));
	}
};

}

#endif _OUSIA_GENERIC_PARSER_HPP_

