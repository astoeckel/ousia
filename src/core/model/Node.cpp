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

#include <iostream>

#include <functional>
#include <unordered_set>

#include <core/common/Exceptions.hpp>

#include "Node.hpp"

namespace ousia {

/* Class SharedResolutionState */

/**
 * Hash functional used to convert pairs of nodes and int to hashes which
 * can be used within a unordered_set.
 */
struct VisitorHash {
	size_t operator()(const std::pair<const Node *, int> &p) const
	{
		const std::hash<const Node *> nodeHash;
		const std::hash<int> intHash;
		return nodeHash(p.first) + 37 * intHash(p.second);
	}
};

/**
 * Alias for the VisitorSet class which represents all nodes which have been
 * visited in the name resolution process. The map stores pairs of node
 * pointers and integers, indicating for which path index the node has already
 * been visited.
 */
using VisitorSet =
    std::unordered_set<std::pair<const Node *, int>, VisitorHash>;

/**
 * The SharedResolutionState structure represents the state that is shared
 * between all resolution paths. A reference to a per-resolution-global
 * SharedResolutionState instance is stored in the ResolutionState class.
 */
class SharedResolutionState {
public:
	/**
	 * Actual path (name pattern) that was requested for resolution.
	 */
	const std::vector<std::string> &path;

	/**
	 * Type of the node that was requested for resolution.
	 */
	const RttiBase &type;

	/**
	 * Tracks all nodes that have already been visited.
	 */
	VisitorSet visited;

	/**
	 * Current resolution result.
	 */
	std::vector<ResolutionResult> result;

	static std::vector<int> buildPrefixTable(
	    const std::vector<std::string> &path);

	/**
	 * Constructor of the SharedResolutionState class.
	 *
	 * @param path is a const reference to the actual path that should be
	 * resolved.
	 * @param type is the type of the node that should be resolved.
	 */
	SharedResolutionState(const std::vector<std::string> &path,
	                      const RttiBase &type)
	    : path(path), type(type)
	{
	}
};

/* Class ResolutionState */

/**
 * The ResolutionState class represents a single resolution path used when
 * resolving Node instances by name.
 */
class ResolutionState {
private:
	/**
	 * Constructor of the ResolutionState class.
	 *
	 * @param shared is the shared, path independent state.
	 * @param resolutionRoot is the current resolution root node.
	 */
	ResolutionState(SharedResolutionState &shared, int idx,
	                Node *resolutionRoot)
	    : shared(shared), idx(idx), resolutionRoot(resolutionRoot)
	{
	}

public:
	/**
	 * Constructor of the ResolutionState class.
	 *
	 * @param shared is the shared, path independent state.
	 * @param resolutionRoot is the current resolution root node.
	 */
	ResolutionState(SharedResolutionState &shared,
	                Node *resolutionRoot = nullptr, bool atStartNode = true)
	    : shared(shared),
	      idx(0),
	      resolutionRoot(resolutionRoot),
	      atStartNode(atStartNode)
	{
	}

	/**
	 * Reference at the resolution state that is shared between the various
	 * resolution paths.
	 */
	SharedResolutionState &shared;

	/**
	 * Current index within the given path.
	 */
	int idx;

	/**
	 * Current resolution root node or nullptr if no resolution root node has
	 * been set yet.
	 */
	Node *resolutionRoot;

	/**
	 * Set to true if the resolution currently is at the node at which the
	 * resolution process was started.
	 */
	bool atStartNode;

	/**
	 * Adds a node to the result.
	 *
	 * @param node is the node that has been found.
	 */
	void addToResult(Node *node)
	{
		shared.result.emplace_back(ResolutionResult{node, resolutionRoot});
	}

	/**
	 * Marks the given node as visited. Returns false if the given node has
	 * already been visited.
	 *
	 * @param node is the node that should be marked as visited.
	 */
	bool markVisited(Node *node)
	{
		if (shared.visited.count(std::make_pair(node, idx)) > 0) {
			return false;
		}
		shared.visited.insert(std::make_pair(node, idx));
		return true;
	}

	/**
	 * Returns true if the search reached the end of the given path.
	 *
	 * @return true if the end of the path was reached, false otherwise.
	 */
	bool atEndOfPath() { return idx == static_cast<int>(shared.path.size()); }

	/**
	 * Returns true if the given type matches the type given in the query.
	 *
	 * @return true if the type matches, false otherwise.
	 */
	bool typeMatches(const RttiBase &type) { return type.isa(shared.type); }

	const std::string &currentName() { return shared.path[idx]; }

	ResolutionState advance()
	{
		return ResolutionState{shared, idx + 1, resolutionRoot};
	}

	ResolutionState fork(Node *newResolutionRoot)
	{
		return ResolutionState{shared, newResolutionRoot, false};
	}

	bool canFollowReferences() { return idx == 0 && atStartNode; }

	bool canFollowComposita() { return idx == 0; }
};

/* Class Node */

void Node::setName(std::string name)
{
	// Call the name change event and (afterwards!) set the new name
	NameChangeEvent ev{this->name, name};
	triggerEvent(ev);
	this->name = std::move(name);
}

void Node::path(std::vector<std::string> &p) const
{
	if (!isRoot()) {
		parent->path(p);
	}
	p.push_back(name);
}

std::vector<std::string> Node::path() const
{
	std::vector<std::string> res;
	path(res);
	return res;
}

bool Node::canFollowComposita(ResolutionState &state)
{
	return state.canFollowComposita();
}

bool Node::canFollowReferences(ResolutionState &state)
{
	return state.canFollowReferences();
}

bool Node::resolve(ResolutionState &state)
{
	// Try to mark this note as visited, do nothing if already has been visited
	if (state.markVisited(this)) {
		std::cout << "visiting " << name << std::endl;

		// Add this node to the result if it matches the current description
		if (state.atEndOfPath()) {
			if (state.typeMatches(type())) {
				std::cout << "found match!" << std::endl;
				state.addToResult(this);
				return true;
			}
		} else {
			continueResolve(state);
		}
	}
	return false;
}

void Node::continueResolve(ResolutionState &state)
{
	// Do nothing in the default implementation
}

bool Node::continueResolveIndex(const Index &index, ResolutionState &state)
{
	Rooted<Node> h = index.resolve(state.currentName());
	if (h != nullptr) {
		ResolutionState advancedState = state.advance();
		return h->resolve(advancedState);
	}
	return false;
}

bool Node::continueResolveCompositum(Handle<Node> h, ResolutionState &state)
{
	// If the name of the compositum explicitly matches the current name in the
	// path, advance the search and try to resolve from this position
	if (h->getName() == state.currentName()) {
		ResolutionState advancedState = state.advance();
		if (h->resolve(advancedState)) {
			return true;
		}
	}

	// If the name did not match, but we are at the beginning of the path,
	// descend without advancing the state
	if (canFollowComposita(state)) {
		return h->resolve(state);
	}
	return false;
}

bool Node::continueResolveReference(Handle<Node> h, ResolutionState &state)
{
	// We can only follow references if we currently are at the beginning of the
	// path and this node is the root node
	if (canFollowReferences(state)) {
		std::cout << "following reference to " << h->getName() << std::endl;
		ResolutionState forkedState = state.fork(this);
		return continueResolveCompositum(h, forkedState);
	}
	return false;
}

std::vector<ResolutionResult> Node::resolve(
    const std::vector<std::string> &path, const RttiBase &type)
{
	// Create the state variables
	SharedResolutionState sharedState(path, type);
	ResolutionState state(sharedState, this);

	// Kickstart the resolution process by treating this very node as compositum
	std::cout << "------------" << std::endl;
	std::cout << "resolving: ";
	for (auto s: path) {
		std::cout << s << " ";
	}
	std::cout << " of type " << type.name << std::endl;

	if (path.size() > 0) {
		continueResolveCompositum(this, state);
	}

	// Return the results
	return sharedState.result;
}

std::vector<ResolutionResult> Node::resolve(const std::string &name,
                                            const RttiBase &type)
{
	// Place the name in a vector and call the corresponding resolve function
	return resolve(std::vector<std::string>{name}, type);
}

/* RTTI type registrations */

const Rtti<Node> RttiTypes::Node{"Node"};
}

