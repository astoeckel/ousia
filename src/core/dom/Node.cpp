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

#include "Node.hpp"

namespace ousia {
namespace dom {

/* Class NodeDescriptor */

int NodeDescriptor::refInCount() const
{
	int res = 0;
	for (const auto &e : refIn) {
		res += e.second;
	}
	return res + rootRefCount;
}

int NodeDescriptor::refOutCount() const
{
	int res = 0;
	for (const auto &e : refOut) {
		res += e.second;
	}
	return res;
}

int NodeDescriptor::refInCount(Node *n) const
{
	if (n == nullptr) {
		return rootRefCount;
	}

	const auto it = refIn.find(n);
	if (it != refIn.cend()) {
		return it->second;
	}
	return 0;
}

int NodeDescriptor::refOutCount(Node *n) const
{
	const auto it = refOut.find(n);
	if (it != refOut.cend()) {
		return it->second;
	}
	return 0;
}

void NodeDescriptor::incrNodeDegree(RefDir dir, Node *n)
{
	// If the given node is null it refers to an input rooted reference
	if (n == nullptr) {
		rootRefCount++;
		return;
	}

	// Fetch a reference to either the input or the output reference map
	auto &m = dir == RefDir::in ? refIn : refOut;

	// Insert a new entry or increment the corresponding reference counter
	auto it = m.find(n);
	if (it == m.end()) {
		m.emplace(std::make_pair(n, 1));
	} else {
		it->second++;
	}
}

bool NodeDescriptor::decrNodeDegree(RefDir dir, Node *n, bool all)
{
	// If the given node is null it refers to an input rooted reference
	if (n == nullptr) {
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
	auto it = m.find(n);
	if (it != m.end()) {
		it->second--;
		if (it->second == 0 || all) {
			m.erase(it);
		}
		return true;
	}
	return false;
}

/* Class NodeManager */

/**
 * The ScopedIncrement class is used by the NodeManager to safely increment a
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

NodeManager::~NodeManager()
{
	// Perform a final sweep
	sweep();

	// All nodes should have been deleted!
	assert(nodes.empty());

	// Free all nodes managed by the node manager (we'll get here if assertions
	// are disabled)
	if (!nodes.empty()) {
		ScopedIncrement incr{deletionRecursionDepth};
		for (auto &e : nodes) {
			delete e.first;
		}
	}
}

NodeDescriptor *NodeManager::getDescriptor(Node *n)
{
	if (n) {
		auto it = nodes.find(n);
		if (it != nodes.end()) {
			return &(it->second);
		}
	}
	return nullptr;
}

void NodeManager::registerNode(Node *n)
{
	nodes.emplace(std::make_pair(n, NodeDescriptor{}));
}

void NodeManager::addRef(Node *tar, Node *src)
{
	// Fetch the node descriptors for the two nodes
	NodeDescriptor *dTar = getDescriptor(tar);
	NodeDescriptor *dSrc = getDescriptor(src);

	// Store the tar <- src reference
	assert(dTar);
	dTar->incrNodeDegree(RefDir::in, src);
	if (src) {
		// Store the src -> tar reference
		assert(dSrc);
		dSrc->incrNodeDegree(RefDir::out, tar);
	} else {
		// We have just added a root reference, remove the element from the
		// list of marked nodes
		marked.erase(tar);
	}
}

void NodeManager::deleteRef(Node *tar, Node *src, bool all)
{
	// Fetch the node descriptors for the two nodes
	NodeDescriptor *dTar = getDescriptor(tar);
	NodeDescriptor *dSrc = getDescriptor(src);

	// Decrement the output degree of the source node first
	if (dSrc) {
		dSrc->decrNodeDegree(RefDir::out, tar, all);
	}

	// Decrement the input degree of the input node
	if (dTar && dTar->decrNodeDegree(RefDir::in, src, all)) {
		// If the node has a zero in degree, it can be safely deleted, otherwise
		// if it has no root reference, add it to the "marked" set which is
		// subject to tracing garbage collection
		if (dTar->refInCount() == 0) {
			deleteNode(tar, dTar);
		} else if (dTar->rootRefCount == 0) {
			// Call the tracing garbage collector if the number of marked nodes
			// is larger than the threshold value and this function was not
			// called from inside the deleteNode function
			marked.insert(tar);
		}
	}

	// Call the garbage collector if the marked size is larger than the actual
	// value
	if (marked.size() >= threshold) {
		sweep();
	}
}

void NodeManager::deleteNode(Node *n, NodeDescriptor *descr)
{
	// Abort if the node already is on the "deleted" list
	if (deleted.find(n) != deleted.end()) {
		return;
	}

	// Increment the recursion depth counter. The "deleteRef" function called
	// below
	// may descend further into this function and the actual deletion should be
	// done in a single step.
	{
		ScopedIncrement incr{deletionRecursionDepth};

		// Add the node to the "deleted" set
		deleted.insert(n);

		// Remove all output references of this node
		while (!descr->refOut.empty()) {
			deleteRef(descr->refOut.begin()->first, n, true);
		}

		// Remove the node from the "marked" set
		marked.erase(n);
	}

	purgeDeleted();
}

void NodeManager::purgeDeleted()
{
	// Perform the actual deletion if the recursion level is zero
	if (deletionRecursionDepth == 0 && !deleted.empty()) {
		// Increment the recursion depth so this function does not get called
		// again while deleting nodes
		ScopedIncrement incr{deletionRecursionDepth};

		// Deleting nodes might add new nodes to the deleted list, thus the
		// iterator would get invalid and we have to use this awkward
		// construction
		while (!deleted.empty()) {
			auto it = deleted.begin();
			Node *n = *it;
			deleted.erase(it);
			marked.erase(n);
			nodes.erase(n);
			delete n;
		}
	}
}

void NodeManager::sweep()
{
	// Only execute sweep on the highest recursion level
	if (deletionRecursionDepth > 0) {
		return;
	}

	// Set containing nodes which are reachable from a rooted node
	std::unordered_set<Node *> reachable;

	// Deletion of nodes may cause other nodes to be added to the "marked" list
	// so we repeat this process until nodes are no longer deleted
	while (!marked.empty()) {
		// Repeat until all nodes in the "marked" list have been visited
		while (!marked.empty()) {
			// Increment the deletionRecursionDepth counter to prevent deletion
			// of nodes while sweep is running
			ScopedIncrement incr{deletionRecursionDepth};

			// Fetch the next node in the "marked" list and remove it
			Node *curNode = *(marked.begin());

			// Perform a breadth-first search starting from the current node
			bool isReachable = false;
			std::unordered_set<Node *> visited{{curNode}};
			std::queue<Node *> queue{{curNode}};
			while (!queue.empty() && !isReachable) {
				// Pop the next element from the queue, remove the element from
				// the marked list as we obviously have evaluated it
				curNode = queue.front();
				queue.pop();
				marked.erase(curNode);

				// Fetch the node descriptor
				NodeDescriptor *descr = getDescriptor(curNode);
				if (!descr) {
					continue;
				}

				// If this node is rooted, the complete visited subgraph is
				// rooted
				if (descr->rootRefCount > 0) {
					isReachable = true;
					break;
				}

				// Iterate over all nodes leading to the current one
				for (auto &src : descr->refIn) {
					Node *srcNode = src.first;

					// Abort if the node already in the reachable list,
					// otherwise add the node to the queue if it was not visited
					if (reachable.find(srcNode) != reachable.end()) {
						isReachable = true;
						break;
					} else if (visited.find(srcNode) == visited.end()) {
						visited.insert(srcNode);
						queue.push(srcNode);
					}
				}
			}

			// Insert the nodes into the list of to be deleted nodes or
			// reachable nodes depending on the "isReachable" flag
			if (isReachable) {
				for (auto n : visited) {
					reachable.insert(n);
				}
			} else {
				for (auto n : visited) {
					deleteNode(n, getDescriptor(n));
				}
			}
		}

		// Now purge all nodes marked for deletion
		purgeDeleted();
	}
}
}
}

