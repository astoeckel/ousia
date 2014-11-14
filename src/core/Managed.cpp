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
#include <queue>

#include "Managed.hpp"

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

/* Class ObjectDescriptor */

int ObjectDescriptor::refInCount() const
{
	int res = 0;
	for (const auto &e : refIn) {
		res += e.second;
	}
	return res + rootRefCount;
}

int ObjectDescriptor::refOutCount() const
{
	int res = 0;
	for (const auto &e : refOut) {
		res += e.second;
	}
	return res;
}

int ObjectDescriptor::refInCount(Managed *o) const
{
	if (o == nullptr) {
		return rootRefCount;
	}

	const auto it = refIn.find(o);
	if (it != refIn.cend()) {
		return it->second;
	}
	return 0;
}

int ObjectDescriptor::refOutCount(Managed *o) const
{
	const auto it = refOut.find(o);
	if (it != refOut.cend()) {
		return it->second;
	}
	return 0;
}

void ObjectDescriptor::incrDegree(RefDir dir, Managed *o)
{
	// If the given Managed is null it refers to an input rooted reference
	if (o == nullptr) {
		rootRefCount++;
		return;
	}

	// Fetch a reference to either the input or the output reference map
	auto &m = dir == RefDir::in ? refIn : refOut;

	// Insert a new entry or increment the corresponding reference counter
	auto it = m.find(o);
	if (it == m.end()) {
		m.emplace(std::make_pair(o, 1));
	} else {
		it->second++;
	}
}

bool ObjectDescriptor::decrDegree(RefDir dir, Managed *o, bool all)
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
	auto &m = dir == RefDir::in ? refIn : refOut;

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

	// Free all objects managed by the Managed manager (we'll get here if assertions
	// are disabled)
	if (!objects.empty()) {
		ScopedIncrement incr{deletionRecursionDepth};
		for (auto &e : objects) {
			delete e.first;
		}
	}
}

ObjectDescriptor *Manager::getDescriptor(Managed *o)
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
	objects.emplace(std::make_pair(o, ObjectDescriptor{}));
}

void Manager::addRef(Managed *tar, Managed *src)
{
	// Fetch the Managed descriptors for the two objects
	ObjectDescriptor *dTar = getDescriptor(tar);
	ObjectDescriptor *dSrc = getDescriptor(src);

	// Store the tar <- src reference
	assert(dTar);
	dTar->incrDegree(RefDir::in, src);
	if (src) {
		// Store the src -> tar reference
		assert(dSrc);
		dSrc->incrDegree(RefDir::out, tar);
	} else {
		// We have just added a root reference, remove the element from the
		// list of marked objects
		marked.erase(tar);
	}
}

void Manager::deleteRef(Managed *tar, Managed *src, bool all)
{
	// Fetch the Managed descriptors for the two objects
	ObjectDescriptor *dTar = getDescriptor(tar);
	ObjectDescriptor *dSrc = getDescriptor(src);

	// Decrement the output degree of the source Managed first
	if (dSrc) {
		dSrc->decrDegree(RefDir::out, tar, all);
	}

	// Decrement the input degree of the input Managed
	if (dTar && dTar->decrDegree(RefDir::in, src, all)) {
		// If the Managed has a zero in degree, it can be safely deleted, otherwise
		// if it has no root reference, add it to the "marked" set which is
		// subject to tracing garbage collection
		if (dTar->refInCount() == 0) {
			deleteObject(tar, dTar);
		} else if (dTar->rootRefCount == 0) {
			// Insert the Managed into the list of objects to be inspected by garbage
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
	// Abort if the Managed already is on the "deleted" list
	if (deleted.find(o) != deleted.end()) {
		return;
	}

	// Increment the recursion depth counter. The "deleteRef" function called
	// below
	// may descend further into this function and the actual deletion should be
	// done in a single step.
	{
		ScopedIncrement incr{deletionRecursionDepth};

		// Add the Managed to the "deleted" set
		deleted.insert(o);

		// Remove all output references of this Managed
		while (!descr->refOut.empty()) {
			deleteRef(descr->refOut.begin()->first, o, true);
		}

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

		// Deleting objects might add new objects to the deleted list, thus the
		// iterator would get invalid and we have to use this awkward
		// construction
		while (!deleted.empty()) {
			auto it = deleted.begin();
			Managed *o = *it;
			deleted.erase(it);
			marked.erase(o);
			objects.erase(o);
			delete o;
		}
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

	// Deletion of objects may cause other objects to be added to the "marked" list
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
					// otherwise add the Managed to the queue if it was not visited
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
}
