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

#include "Managed.hpp"
#include "Manager.hpp"

//#define MANAGER_DEBUG_PRINT

#if defined(MANAGER_DEBUG_PRINT) || defined(MANAGER_GRAPHVIZ_EXPORT)
#include <iostream>
#include <fstream>
#include <core/common/Rtti.hpp>
#include <core/common/Property.hpp>
#endif

namespace ousia {

/* Private Class ScopedIncrement */

/**
 * The ScopedIncrement class is used by the Manager to safely increment a
 * variable when a scope is entered and to decrement it when the scope is left.
 */
class ScopedIncrement {
private:
	/**
	 * Reference to the variable that should be incremented.
	 */
	int &i;

public:
	/**
	 * Constructor of ScopedIncrement. Increments the given variable.
	 *
	 * @param i is the variable that should be incremented.
	 */
	ScopedIncrement(int &i) : i(i) { i++; }

	/**
	 * Destructor of ScopedIncrement. Decrements the referenced variable.
	 */
	~ScopedIncrement() { i--; }
};

/* Class Manager::ObjectDescriptor */

bool Manager::ObjectDescriptor::hasInRef() const
{
	return rootRefCount > 0 || !refIn.empty();
}

void Manager::ObjectDescriptor::incrDegree(RefDir dir, Managed *o)
{
	// If the given Managed is null it refers to an input rooted reference
	if (o == nullptr) {
		rootRefCount++;
		return;
	}

	// Fetch a reference to either the input or the output reference map
	auto &m = dir == RefDir::IN ? refIn : refOut;

	// Insert a new entry or increment the corresponding reference counter
	auto it = m.find(o);
	if (it == m.end()) {
		m.emplace(std::make_pair(o, 1));
	} else {
		it->second++;
	}
}

bool Manager::ObjectDescriptor::decrDegree(RefDir dir, Managed *o, bool all)
{
	// If the given Managed is null it refers to an input rooted reference
	if (o == nullptr) {
		if (rootRefCount > 0) {
			if (all) {
				rootRefCount = 0;
			} else {
				rootRefCount--;
			}
			return true;
		}
		return false;
	}

	// Fetch a reference to either the input or the output reference map
	auto &m = dir == RefDir::IN ? refIn : refOut;

	// Decrement corresponding reference counter, delete the entry if the
	// reference counter reaches zero
	auto it = m.find(o);
	if (it != m.end()) {
		it->second--;
		if (it->second == 0 || all) {
			m.erase(it);
		}
		return true;
	}
	return false;
}

/* Class Manager */

Manager::~Manager()
{
	// Perform a final sweep
	sweep();

	// All objects should have been deleted!
	assert(objects.empty());

	// Free all objects managed by the Managed manager (we'll get here if
	// assertions
	// are disabled)
	if (!objects.empty()) {
		ScopedIncrement incr{deletionRecursionDepth};
		for (auto &e : objects) {
			delete e.first;
		}
	}
}

/* Class Manager: Garbage collection */

Manager::ObjectDescriptor *Manager::getDescriptor(Managed *o)
{
	if (o) {
		auto it = objects.find(o);
		if (it != objects.end()) {
			return &(it->second);
		}
	}
	return nullptr;
}

void Manager::manage(Managed *o)
{
#ifdef MANAGER_DEBUG_PRINT
	std::cout << "manage " << o << std::endl;
#endif
	objects.emplace(o, ObjectDescriptor{nextUid});
	uids.emplace(nextUid, o);
	nextUid++;
}

void Manager::addRef(Managed *tar, Managed *src)
{
#ifdef MANAGER_DEBUG_PRINT
	std::cout << "addRef " << tar << " <- " << src << std::endl;
#endif

	// Make sure the source and target manager are the same
	if (src) {
		assert(&tar->getManager() == &src->getManager());
	}

	// Fetch the Managed descriptors for the two objects
	ObjectDescriptor *dTar = getDescriptor(tar);
	ObjectDescriptor *dSrc = getDescriptor(src);

	// Store the tar <- src reference
	assert(dTar);
	dTar->incrDegree(RefDir::IN, src);
	if (src) {
		// Store the src -> tar reference
		assert(dSrc);
		dSrc->incrDegree(RefDir::OUT, tar);
	} else {
		// We have just added a root reference, remove the element from the
		// list of marked objects
		marked.erase(tar);
	}
}

void Manager::deleteRef(Managed *tar, Managed *src, bool all)
{
#ifdef MANAGER_DEBUG_PRINT
	std::cout << "deleteRef " << tar << " <- " << src << std::endl;
#endif

	// Fetch the Managed descriptors for the two objects
	ObjectDescriptor *dTar = getDescriptor(tar);
	ObjectDescriptor *dSrc = getDescriptor(src);

	// Decrement the output degree of the source Managed first
	if (dSrc) {
		dSrc->decrDegree(RefDir::OUT, tar, all);
	}

	// Decrement the input degree of the input Managed
	if (dTar && dTar->decrDegree(RefDir::IN, src, all)) {
		// If the Managed has a zero in degree, it can be safely deleted,
		// otherwise if it has no root reference, add it to the "marked" set
		// which is subject to tracing garbage collection
		if (!dTar->hasInRef()) {
			deleteObject(tar, dTar);
		} else if (dTar->rootRefCount == 0) {
			// Insert the Managed into the list of objects to be inspected by
			// garbage
			// collection
			marked.insert(tar);
		}
	}

	// Call the tracing garbage collector if the marked size is larger than the
	// actual value
	if (marked.size() >= threshold) {
		sweep();
	}
}

void Manager::deleteObject(Managed *o, ObjectDescriptor *descr)
{
#ifdef MANAGER_DEBUG_PRINT
	std::cout << "deleteObject " << o << std::endl;
#endif

	// Abort if the Managed already is on the "deleted" list
	if (deleted.count(o)) {
		return;
	}

	// Increment the recursion depth counter. The "deleteRef" function called
	// below may descend further into this function and the actual deletion
	// should be done in a single step.
	{
		ScopedIncrement incr{deletionRecursionDepth};

		// Add the Managed to the "deleted" set
		deleted.insert(o);

		// Make sure all input references are deleted
		while (!descr->refIn.empty()) {
			deleteRef(o, descr->refIn.begin()->first, true);
		}

		// Add the Managed object to the orderedDeleted list -- this should
		// happen after all input references have been removed
		orderedDeleted.push_back(o);

		// Remove all output references of this Managed
		while (!descr->refOut.empty()) {
			deleteRef(descr->refOut.begin()->first, o, true);
		}

		// Remove the uid, data and event store entry
		uids.erase(descr->uid);
		store.erase(o);
		events.erase(o);

		// Remove the Managed from the "marked" set
		marked.erase(o);
	}

	purgeDeleted();
}

void Manager::purgeDeleted()
{
	// Perform the actual deletion if the recursion level is zero
	if (deletionRecursionDepth == 0 && !deleted.empty()) {
		// Increment the recursion depth so this function does not get called
		// again while deleting objects
		ScopedIncrement incr{deletionRecursionDepth};

		for (size_t i = 0; i < orderedDeleted.size(); i++) {
			Managed *m = orderedDeleted[i];
			deleted.erase(m);
			marked.erase(m);
			objects.erase(m);
			delete m;
		}
		orderedDeleted.clear();
		assert(deleted.empty());
	}
}

void Manager::sweep()
{
	// Only execute sweep on the highest recursion level
	if (deletionRecursionDepth > 0) {
		return;
	}

	// Set containing objects which are reachable from a rooted Managed
	std::unordered_set<Managed *> reachable;

	// Deletion of objects may cause other objects to be added to the "marked"
	// list
	// so we repeat this process until objects are no longer deleted
	while (!marked.empty()) {
		// Repeat until all objects in the "marked" list have been visited
		while (!marked.empty()) {
			// Increment the deletionRecursionDepth counter to prevent deletion
			// of objects while sweep is running
			ScopedIncrement incr{deletionRecursionDepth};

			// Fetch the next Managed in the "marked" list and remove it
			Managed *curManaged = *(marked.begin());

			// Perform a breadth-first search starting from the current Managed
			bool isReachable = false;
			std::unordered_set<Managed *> visited{curManaged};
			std::queue<Managed *> queue{{curManaged}};
			while (!queue.empty() && !isReachable) {
				// Pop the next element from the queue, remove the element from
				// the marked list as we obviously have evaluated it
				curManaged = queue.front();
				queue.pop();
				marked.erase(curManaged);

				// Fetch the Managed descriptor
				ObjectDescriptor *descr = getDescriptor(curManaged);
				if (!descr) {
					continue;
				}

				// If this Managed is rooted, the complete visited subgraph is
				// rooted
				if (descr->rootRefCount > 0) {
					isReachable = true;
					break;
				}

				// Iterate over all objects leading to the current one
				for (auto &src : descr->refIn) {
					Managed *srcManaged = src.first;

					// Abort if the Managed already in the reachable list,
					// otherwise add the Managed to the queue if it was not
					// visited
					if (reachable.find(srcManaged) != reachable.end()) {
						isReachable = true;
						break;
					} else if (visited.find(srcManaged) == visited.end()) {
						visited.insert(srcManaged);
						queue.push(srcManaged);
					}
				}
			}

			// Insert the objects into the list of to be deleted objects or
			// reachable objects depending on the "isReachable" flag
			if (isReachable) {
				for (auto o : visited) {
					reachable.insert(o);
				}
			} else {
				for (auto o : visited) {
					deleteObject(o, getDescriptor(o));
				}
			}
		}

		// Now purge all objects marked for deletion
		purgeDeleted();
	}
}

/* Class Managed: Unique IDs */

ManagedUid Manager::getUid(Managed *o)
{
	const auto it = objects.find(o);
	if (it != objects.end()) {
		return it->second.uid;
	}
	return 0;
}

Managed *Manager::getManaged(ManagedUid uid)
{
	const auto it = uids.find(uid);
	if (it != uids.end()) {
		return it->second;
	}
	return nullptr;
}

/* Class Manager: Attached data */

void Manager::storeData(Managed *ref, const std::string &key, Managed *data)
{
	// Add the new reference from the reference object to the data object
	addRef(data, ref);

	// Make sure a data map for the given reference object exists
	auto &map =
	    store.emplace(ref, std::map<std::string, Managed *>{}).first->second;

	// Insert the given data for the key, decrement the references if
	auto it = map.find(key);
	if (it == map.end()) {
		map.insert(it, std::make_pair(key, data));
	} else {
		// Do nothing if the same data is stored
		if (it->second == data) {
			return;
		}

		// Delete the reference from "ref" to the previously stored element.
		deleteRef(it->second, ref);

		// Insert a new reference and add the element to the map
		map.insert(map.erase(it), std::make_pair(key, data));
	}
}

Managed *Manager::readData(Managed *ref, const std::string &key) const
{
	// Try to find the reference element in the store
	auto storeIt = store.find(ref);
	if (storeIt != store.end()) {
		// Try to find the key in the map for the element
		auto &map = storeIt->second;
		auto mapIt = map.find(key);
		if (mapIt != map.end()) {
			return mapIt->second;
		}
	}
	return nullptr;
}

std::map<std::string, Managed *> Manager::readData(Managed *ref) const
{
	// Try to find the map for the given reference element and return it
	auto storeIt = store.find(ref);
	if (storeIt != store.end()) {
		return storeIt->second;
	}
	return std::map<std::string, Managed *>{};
}

bool Manager::deleteData(Managed *ref, const std::string &key)
{
	// Find the reference element in the store
	auto storeIt = store.find(ref);
	if (storeIt != store.end()) {
		// Delete the key from the data map
		auto &map = storeIt->second;
		auto mapIt = map.find(key);
		if (mapIt != map.end()) {
			// Delete the reference from "ref" to the previously stored element
			deleteRef(mapIt->second, ref);

			// Remove the element from the list
			map.erase(mapIt);

			return true;
		}
	}
	return false;
}

/* Class Manager: Event handling */

EventId Manager::registerEvent(Managed *ref, EventType type,
                               EventHandler handler, Managed *owner, void *data)
{
	// Create a event handler descriptor and store it along with the
	auto &vec = events.emplace(ref, std::vector<EventHandlerDescriptor>{})
	                .first->second;
	const EventHandlerDescriptor descr(type, handler, getUid(owner), data);
	for (size_t i = 0; i < vec.size(); i++) {
		if (!vec[i].handler) {
			vec[i] = descr;
			return i;
		}
	}
	vec.push_back(descr);
	return vec.size() - 1;
}

bool Manager::unregisterEvent(Managed *ref, EventId id)
{
	auto eventsIt = events.find(ref);
	if (eventsIt != events.end()) {
		auto &vec = eventsIt->second;
		if (id < vec.size() && vec[id].handler) {
			// Remove the handler from the list by resetting handler and owner
			// to nullptr
			EventHandlerDescriptor &descr = vec[id];
			descr.handler = nullptr;
			descr.ownerUid = 0;
			return true;
		}
	}
	return false;
}

bool Manager::unregisterEvent(Managed *ref, EventType type,
                              EventHandler handler, Managed *owner, void *data)
{
	auto eventsIt = events.find(ref);
	if (eventsIt != events.end()) {
		const ManagedUid ownerUid = getUid(owner);
		auto &vec = eventsIt->second;
		for (EventHandlerDescriptor &descr : vec) {
			if (descr.type == type && descr.handler == handler &&
			    descr.ownerUid == ownerUid && descr.data == data) {
				// Remove the handler from the list by resetting handler and
				// owner to nullptr
				descr.handler = nullptr;
				descr.ownerUid = 0;
				return true;
			}
		}
	}
	return false;
}

bool Manager::triggerEvent(Managed *ref, Event &ev)
{
	bool hasHandler = false;
	auto eventsIt = events.find(ref);
	if (eventsIt != events.end()) {
		std::vector<EventHandlerDescriptor> descrs = eventsIt->second;
		for (auto it = descrs.begin(); it != descrs.end();) {
			const EventHandlerDescriptor &descr = *it;
			if (descr.type == ev.type && descr.handler) {
				// Resolve the given owner uid to a managed pointer -- erase the
				// event handler if the owner no longer exists
				Managed *owner = nullptr;
				if (descr.ownerUid != 0) {
					owner = getManaged(descr.ownerUid);
					if (!owner) {
						it = descrs.erase(it);
						continue;
					}
				}

				// Call the event handler
				ev.sender = ref;
				descr.handler(ev, owner, descr.data);
				hasHandler = true;
			}
			it++;
		}
	}
	return hasHandler;
}

#ifdef MANAGER_GRAPHVIZ_EXPORT

// Note: This is just an ugly hack used for development. Do not complain. You
// have been warned.

enum class EdgeType { NORMAL, DATA, AGGREGATE };

void Manager::exportGraphviz(const char *filename)
{
	std::fstream fs(filename, std::ios_base::out);
	if (!fs.good()) {
		throw "Error while opening output file.";
	}

	fs << "digraph G {" << std::endl;
	fs << "\tnode [shape=plaintext,fontsize=10]" << std::endl;

	size_t idx = 0;
	for (auto &object : objects) {
		// References to the object descriptor and the object
		Managed *objectPtr = object.first;
		ObjectDescriptor &objectDescr = object.second;

		// Read flags, data and events
		bool isMarked = marked.count(objectPtr) > 0;
		bool isDeleted = deleted.count(objectPtr) > 0;
		std::map<std::string, Managed *> storeData =
		    store.count(objectPtr) > 0 ? store[objectPtr]
		                               : std::map<std::string, Managed *>{};
		std::vector<EventHandlerDescriptor> eventData =
		    events.count(objectPtr) > 0 ? events[objectPtr]
		                                : std::vector<EventHandlerDescriptor>{};

		// Read type information and Node name (if available)
		const RttiType &type = objectPtr->type();
		const std::string &typeName = type.name;

		// Fetch the name of the object if the object has a "name" property
		std::string name;
		if (type.hasProperty("name")) {
			name = type.getProperty("name")->get(objectPtr).toString();
		}

		// Print the node
		uintptr_t p = reinterpret_cast<uintptr_t>(objectPtr);
		fs << "\tn" << std::hex << std::noshowbase << p << " [" << std::endl;

		// Print the label
		fs << "\t\tlabel=<"
		   << "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">"
		   << "<TR><TD>" << std::hex << std::showbase << p << "</TD></TR>"
		   << "<TR><TD><I>" << typeName << "</I></TD></TR>";

		// Print any name
		if (!name.empty()) {
			fs << "<TR><TD><B>" << name << "</B></TD></TR>";
		}

		// Print any available data element
		{
			for (auto &d : storeData) {
				fs << "<TR><TD PORT=\"data_" << d.first
				   << "\"><FONT COLOR=\"orangered2\">" << d.first
				   << "</FONT></TD></TR>";
			}
		}

		// Print any available event
		{
			std::unordered_set<const char *> eventTypes;
			for (auto &d : eventData) {
				if (d.handler) {
					eventTypes.insert(d.getEventTypeName());
				}
			}
			for (const char *name : eventTypes) {
				fs << "<TR><TD PORT=\"ev_" << name
				   << "\"><FONT COLOR=\"darkolivegreen4\">" << name
				   << "</FONT></TD></TR>";
			}
		}

		fs << "</TABLE>>" << std::endl;

		// Color deleted and marked objects in other colors
		if (isDeleted) {
			fs << ",color=firebrick4";
		} else if (isMarked) {
			fs << ",color=gray40";
		}

		fs << "\t]" << std::endl;

		// Create edges to all outgoing nodes
		for (const auto &e : objectDescr.refOut) {
			// Copy the number of references -- repeat until all references have
			// been drawn
			int edgeCount = e.second;
			while (edgeCount > 0) {
				// Get the type of the target element
				uintptr_t pTar = reinterpret_cast<uintptr_t>(e.first);
				const RttiType &typeTar = e.first->type();

				// Get some information about the edge
				std::string port = "";
				EdgeType et = EdgeType::NORMAL;

				if (et == EdgeType::NORMAL) {
					for (auto it = storeData.begin(); it != storeData.end();
					     it++) {
						if (it->second == e.first) {
							et = EdgeType::DATA;
							port = std::string(":data_") + it->first;
							storeData.erase(it);
							break;
						}
					}
				}
				if (et == EdgeType::NORMAL && type.composedOf(typeTar)) {
					et = EdgeType::AGGREGATE;
				}

				// Print the edge
				fs << "\tn" << std::hex << std::noshowbase << p << port
				   << " -> n" << std::hex << std::noshowbase << pTar << " [";
				int c = edgeCount;
				switch (et) {
					/*		case EdgeType::EVENT:
					            fs <<
					   "weight=5,penwidth=1,color=darkolivegreen4,";
					            c = 1;
					            break;*/
					case EdgeType::DATA:
						fs << "weight=5,penwidth=1,color=orangered2,";
						c = 1;
						break;
					case EdgeType::AGGREGATE:
						fs << "weight=100,color=dodgerblue4,arrowhead=diamond,"
						      "penwidth=2,";
						c = edgeCount;
						break;
					default:
						fs << "weight=0,penwidth=0.5,";
						c = edgeCount;
						break;
				}
				edgeCount -= c;
				fs << "labeldistance=\"2\",headlabel=\"" << std::dec << c << "\"]" << std::endl;
			}
		}

		// Create edges for all events
		for (auto &d : eventData) {
			Managed *owner = getManaged(d.ownerUid);
			if (!owner) {
				continue;
			}
			uintptr_t pTar = reinterpret_cast<uintptr_t>(owner);
			fs << "\tn" << std::hex << std::noshowbase << p << ":ev_" << d.getEventTypeName() << " -> n"
			   << std::hex << std::noshowbase << pTar
			   << " [weight=0,penwidth=0.5,color=darkolivegreen4,style=dashed,arrowhead=vee]"
			   << std::endl;
		}

		// Display root edges
		if (objectDescr.rootRefCount > 0) {
			fs << "\tr" << std::hex << std::noshowbase << p
			   << " [shape=\"point\",width=0.1]" << std::endl;
			fs << "\tr" << std::hex << std::noshowbase << p << " -> n"
			   << std::hex << std::noshowbase << p << " [weight=1000,headlabel="
			                                          "\"" << std::dec
			   << objectDescr.rootRefCount << "\"]" << std::endl;
		}

		idx++;
	}

	fs << "}" << std::endl;
}
#endif
}

