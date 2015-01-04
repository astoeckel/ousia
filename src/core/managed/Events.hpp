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
 * @file Events.hpp
 *
 * Contains classes and structures used for event handling within the Manager
 * and Managed classes.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_EVENTS_HPP_
#define _OUSIA_EVENTS_HPP_

#include <cstdint>

#include <string>

namespace ousia {

// Forward declarations
class Managed;
class Event;

using EventId = size_t;

/**
 * EventType is an enum containing all possible node events. New event types
 * should be added here.
 */
enum class EventType : int {
	/**
     * Generic update event which may be triggered if some important property
     * of the node is changed.
     */
	UPDATE,

	/**
     * The NAME_CHANGE event is used to inform listeners that the name of the
     * node has changed.
     */
	NAME_CHANGE,

	/**
     * The ADD_CHILD event is used to inform listeners that the node got a new
     * child in any of its child node lists.
     */
	ADD_CHILD,

	/**
     * The DELETE_CHILD event is used to inform listeners that the node got a
     * new child in any of its child node lists.
     */
	DELETE_CHILD
};

/**
 * Definition of the EventHandler function.
 *
 * @param event is a reference to the object holding the event data.
 * @param owner is a reference to the managed object that was given in the
 * registerEventHandler function.
 * @param data is some user defined data.
 */
using EventHandler = void (*)(const Event &event, Managed *owner, void *data);

/**
 * The Event class and its child classes are responsible for containing the
 * actual event data which further describes the event to the event handlers.
 */
struct Event {
	/**
	 * Actual event type.
	 */
	EventType type;

	/**
	 * Node on which the event was triggered.
	 */
	Managed *sender;

	/**
	 * Constructor of the Event class.
	 *
	 * @param type is the actual event type.
	 */
	Event(EventType type) : type(type), sender(nullptr){};

	Event(const Event &) = delete;
	Event &operator=(const Event &) = delete;
};

/**
 * Event used when the name of a node has changed.
 */
struct NameChangeEvent : public Event {
	/**
	 * Reference to a string containing the old name of the node.
	 */
	const std::string &oldName;

	/**
	 * Reference to a string containing the new name of the node.
	 */
	const std::string &newName;

	/**
	 * Constructor of the NameChangeEvent class.
	 *
	 * @param oldName is the old name of the node. The given string is not
	 * copied, only the reference is passed around.
	 * @param newName is the new name of the node. The given string is not
	 * copied, only the reference is passed around.
	 */
	NameChangeEvent(const std::string &oldName, const std::string &newName)
	    : Event(EventType::NAME_CHANGE), oldName(oldName), newName(newName)
	{
	}
};

/**
 * Struct containing the data which describes a single registered event handler.
 */
struct EventHandlerDescriptor {
	/**
	 * Event type.
	 */
	EventType type;

	/**
	 * Reference to the event handler containing the events.
	 */
	EventHandler handler;

	/**
	 * Weak reference to the owner object.
	 */
	uint64_t ownerUid;

	/**
	 * User defined data.
	 */
	void *data;

	/**
	 * Constructor of the EventHandlerDescriptor struct.
	 *
	 * @param type is the event type for which the handler is registered.
	 * @param handler is the function pointer which is going to be called once
	 * the associated event handler has fired.
	 * @param ownerUid is a user-specified object which owns the method that is
	 * going to be called. This can be used to make sure that the method which
	 * handles the events has access to its owned object as long as the event
	 * handler lives.
	 * @param data is some arbitrary user defined data.
	 */
	EventHandlerDescriptor(EventType type, EventHandler handler,
	                       uint64_t ownerUid, void *data)
	    : type(type), handler(handler), ownerUid(ownerUid), data(data)
	{
	}

	/**
	 * Returns a human readable name of the given event type.
	 */
	static const char *getEventTypeName(EventType type);

	/**
	 * Returns the name of the current event type.
	 */
	const char *getEventTypeName() const
	{
		return getEventTypeName(this->type);
	}
};
}

#endif /* _OUSIA_EVENTS_HPP_ */

