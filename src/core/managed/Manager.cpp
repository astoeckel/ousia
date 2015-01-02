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

//#define DEBUG_MANAGER

#ifdef DEBUG_MANAGER
#include <iostream>
#endif

#include <cassert>

#include "Managed.hpp"
#include "Manager.hpp"

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
#ifdef DEBUG_MANAGER
	std::cout << "manage " << o << std::endl;
#endif
	objects.emplace(std::make_pair(o, ObjectDescriptor{}));
}

void Manager::addRef(Managed *tar, Managed *src)
{
#ifdef DEBUG_MANAGER
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
#ifdef DEBUG_MANAGER
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
#ifdef DEBUG_MANAGER
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

		// Remove the data store and the event store entry
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
			std::unordered_set<Managed *> visited{{curManaged}};
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
                               EventHandler handler, Managed *owner)
{
	// Add a reference from the reference object to the owner object
	if (owner) {
		addRef(owner, ref);
	}

	// Create a event handler descriptor and store it along with the
	auto &vec = events.emplace(ref, std::vector<EventHandlerDescriptor>{})
	                .first->second;
	const EventHandlerDescriptor descr(type, handler, owner);
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
			// Delete the reference from the reference object to the handler
			EventHandlerDescriptor &descr = vec[id];
			if (descr.owner) {
				deleteRef(descr.owner, ref);
			}

			// Remove the handler from the list by resetting handler and owner
			// to nullptr
			descr.handler = nullptr;
			descr.owner = nullptr;
			return true;
		}
	}
	return false;
}

bool Manager::triggerEvent(Managed *ref, Event &data)
{
	bool hasHandler = false;
	auto eventsIt = events.find(ref);
	if (eventsIt != events.end()) {
		for (EventHandlerDescriptor &descr : eventsIt->second) {
			if (descr.type == data.type && descr.handler) {
				descr.handler(data, descr.owner);
				hasHandler = true;
			}
		}
	}
	return hasHandler;
}
}

