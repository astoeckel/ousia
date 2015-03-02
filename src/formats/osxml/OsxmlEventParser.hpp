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
 * @file OsxmlEventParser.hpp
 *
 * The OsxmlEventParser class is responsible for parsing an XML file and calling
 * the corresponding event handler functions if an XML item is found. Event
 * handling is performed using a listener interface.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OSXML_EVENT_PARSER_HPP_
#define _OSXML_EVENT_PARSER_HPP_

#include <memory>
#include <string>

namespace ousia {

// Forward declarations
class Logger;
class Variant;
class OsxmlEventParserData;

/**
 * Interface which defines the callback functions which are called by the
 * OsxmlEventParser whenever an event occurs.
 */
class OsxmlEvents {
public:
	/**
	 * Virtual destructor.
	 */
	virtual ~OsxmlEvents();

	/**
	 * Called whenever a command starts. Note that this implicitly always starts
	 * the default field of the command.
	 *
	 * @param name is a string variant containing name and location of the
	 * command.
	 * @param args is a map containing the arguments that were given to the
	 * command.
	 */
	virtual void commandStart(const Variant &name,
	                          const Variant::mapType &args) = 0;

	/**
	 * Called whenever an annotation starts. Note that this implicitly always
	 * starts the default field of the annotation.
	 *
	 * @param className is a string variant containing the name of the
	 * annotation class and the location of the annotation definition.
	 * @param args is a map variant containing the arguments that were given
	 * to the annotation definition.
	 */
	virtual void annotationStart(const Variant &className,
	                             const Variant::mapType &args) = 0;

	/**
	 * Called whenever the range of an annotation ends. The callee must
	 * disambiguate the actual annotation that is finished here.
	 *
	 * @param className is a string variant containing the name of the
	 * annotation class that should end here. May be empty (or nullptr), if no
	 * elementName has been specified at the end of the annotation.
	 * @param elementName is the name of the annotation element that should be
	 * ended here. May be empty (or nullptr), if no elementName has been
	 * specified at the end of the annotation.
	 */
	virtual void annotationEnd(const Variant &className,
	                           const Variant &elementName) = 0;

	/**
	 * Called whenever the command or annotation tags end.
	 */
	virtual void rangeEnd() = 0;

	/**
	 * Called whenever string data is found.
	 *
	 * @param data is a TokenizedData instance containing the string data that
	 * was found in the XML file.
	 */
	virtual void data(const TokenizedData &data) = 0;
};

/**
 * The OsxmlEventParser class is a wrapper around eXpat which implements the
 * specialities of the osxml formats class (like annotation ranges). It notifies
 * a specified event handler whenever a command, annotation or data has been
 * reached.
 */
class OsxmlEventParser {
private:
	/**
	 * Reference at the internal CharReader instance.
	 */
	CharReader &reader;

	/**
	 * Set of callback functions to be called whenever an event is triggered.
	 */
	OsxmlEvents &events;

	/**
	 * Reference at the Logger object to which error messages or warnings should
	 * be logged.
	 */
	Logger &logger;

	/**
	 * Data to be used by the internal functions.
	 */
	std::unique_ptr<OsxmlEventParserData> data;

public:
	/**
	 * Constructor fo the OsxmlEventParser. Takes a reference at the OsxmlEvents
	 * of which the callback functions are called.
	 *
	 * @param reader is a reference to the CharReader instance from which the
	 * XML should be read.
	 * @param events is a refence at an instance of the OsxmlEvents class. All
	 * events are forwarded to this class.
	 * @param logger is the Logger instance to which log messages should be
	 * written.
	 */
	OsxmlEventParser(CharReader &reader, OsxmlEvents &events, Logger &logger);

	/**
	 * Destructor of OsxmlEventParser (needed for unique_ptr to incomplete type)
	 */
	~OsxmlEventParser();

	/**
	 * Performs the actual parsing. Reads the XML using eXpat and calles the
	 * callbacks in the event listener instance whenever something interesting
	 * happens.
	 */
	void parse();

	/**
	 * Returns the internal CharReader reference.
	 *
	 * @return the CharReader reference.
	 */
	CharReader &getReader() const;

	/**
	 * Returns the internal Logger reference.
	 *
	 * @return the internal Logger reference.
	 */
	Logger &getLogger() const;

	/**
	 * Returns the internal OsxmlEvents reference.
	 *
	 * @return the internal OsxmlEvents reference.
	 */
	OsxmlEvents &getEvents() const;

	/**
	 * Used internally to fetch a reference at the internal data.
	 *
	 * @return a reference at the internal OsxmlEventParserData structure.
	 */
	OsxmlEventParserData &getData() const;
};
}

#endif /* _OSXML_EVENT_PARSER_HPP_ */

