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

#include "Events.hpp"

namespace ousia {

const char *EventHandlerDescriptor::getEventTypeName(EventType type)
{
	switch (type) {
		case EventType::UPDATE:
			return "update";
		case EventType::NAME_CHANGE:
			return "name_change";
		case EventType::ADD_CHILD:
			return "add_child";
		case EventType::DELETE_CHILD:
			return "delete_child";
	}
	return "";
}

}

