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

#include <functional>
#include <unordered_set>

#include <core/common/Exceptions.hpp>

#include "Node.hpp"

namespace ousia {

/* Class SharedResolutionState */

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
	std::unordered_set<Node *> visited;

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
	ResolutionState(SharedResolutionState &shared,
					int idx,
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
	ResolutionState(SharedResolutionState &shared, Node *resolutionRoot = nullptr)
	    : shared(shared), idx(0), resolutionRoot(resolutionRoot)
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
		if (shared.visited.count(node) > 0) {
			return false;
		}
		shared.visited.insert(node);
		return true;
	}

	/**
	 * Returns true if the current node matches the search criteria.
	 *
	 * @param type is the type of the current node.
	 * @return true if the state indicates that the path has been completely
	 * matched and that the given type matches the queried type.
	 */
	bool matches(const RttiBase &type)
	{
		return idx == static_cast<int>(shared.path.size()) &&
		       type.isa(shared.type);
	}

	const std::string &currentName() {
		return shared.path[idx];
	}

	ResolutionState advance() {
		return ResolutionState{shared, idx + 1, resolutionRoot};
	}

	ResolutionState fork(Node *newResolutionRoot) {
		return ResolutionState{shared, newResolutionRoot};
	}
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

bool Node::resolutionAtBeginning(ResolutionState &state)
{
	return state.idx == 0;
}

bool Node::resolve(ResolutionState &state)
{
	// Try to mark this note as visited, do nothing if already has been visited
	if (state.markVisited(this)) {
		// Add this node to the result if it matches the current description
		if (state.matches(type())) {
			state.addToResult(this);
			return true;
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

bool Node::continueResolveIndex(const Index &index, ResolutionState &state) {
	Rooted<Node> h = index.resolve(state.currentName());
	if (h != nullptr) {
		ResolutionState advancedState = state.advance();
		return h->resolve(advancedState);
	}
	return false;
}

bool Node::continueResolveCompositum(Handle<Node> h, ResolutionState &state)
{
	if (h->getName() == state.currentName()) {
		ResolutionState advancedState = state.advance();
		return h->resolve(advancedState);
	} else if (resolutionAtBeginning(state)) {
		return h->resolve(state);
	}
	return false;
}

bool Node::continueResolveReference(Handle<Node> h, ResolutionState &state)
{
	if (resolutionAtBeginning(state)) {
		ResolutionState forkedState = state.fork(this);
		if (h->getName() == state.currentName()) {
			ResolutionState advancedState = forkedState.advance();
			return h->resolve(advancedState);
		}
		return h->resolve(forkedState);
	}
	return false;
}

std::vector<ResolutionResult> Node::resolve(
    const std::vector<std::string> &path, const RttiBase &type)
{
	// Create the state variables
	SharedResolutionState sharedState(path, type);
	ResolutionState state(sharedState, this);

	// Call the internal resolve function, make sure the length of the given
	// path is valid
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

