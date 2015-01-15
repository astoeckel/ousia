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
#include <core/common/Rtti.hpp>
#include <core/common/TypedRttiBuilder.hpp>

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
		std::hash<const Node *> nodeHash;
		std::hash<int> intHash;
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
	const RttiType &type;

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
	                      const RttiType &type)
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
public:
	/**
	 * Reference at the resolution state that is shared between the various
	 * resolution paths.
	 */
	SharedResolutionState &shared;

	/**
	 * Current resolution root node or nullptr if no resolution root node has
	 * been set yet.
	 */
	Node *resolutionRoot;

	/**
	 * Current index within the given path.
	 */
	int idx;

	/**
	 * Set to true if the resolution currently is in the subtree in which the
	 * node resolution process was started (no reference boundary has been
	 * passed yet).
	 */
	bool inStartTree;

	/**
	 * Set to true, once a compositum has been found.
	 */
	bool foundCompositum;

	/**
	 * Constructor of the ResolutionState class.
	 *
	 * @param shared is the shared, path independent state.
	 * @param resolutionRoot is the current resolution root node.
	 */
	ResolutionState(SharedResolutionState &shared,
	                Node *resolutionRoot = nullptr, int idx = 0,
	                bool inStartTree = true)
	    : shared(shared),
	      resolutionRoot(resolutionRoot),
	      idx(idx),
	      inStartTree(inStartTree),
	      foundCompositum(false)
	{
	}

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
	bool typeMatches(const RttiType &type) { return type.isa(shared.type); }

	bool canContainType(const RttiType &type)
	{
		return type.composedOf(shared.type);
	}

	const std::string &currentName() { return shared.path[idx]; }

	ResolutionState advance()
	{
		return ResolutionState{shared, resolutionRoot, idx + 1, false};
	}

	ResolutionState fork(Node *newResolutionRoot)
	{
		return ResolutionState{shared, newResolutionRoot, 0, false};
	}

	bool canFollowReferences()
	{
		return idx == 0 && inStartTree && !foundCompositum;
	}

	bool canFollowComposita() { return idx == 0; }

	size_t resultCount() { return shared.result.size(); }
};

/* Class ResolutionResult */

std::vector<std::string> ResolutionResult::path() const
{
	return node->path(resolutionRoot);
}

/* Class Node */

void Node::setName(std::string name)
{
	// Call the name change event and (afterwards!) set the new name
	NameChangeEvent ev{this->name, name};
	triggerEvent(ev);
	this->name = std::move(name);
}

void Node::path(std::vector<std::string> &p, Handle<Node> root) const
{
	if (!isRoot()) {
		parent->path(p, root);
	}
	if (this != root) {
		p.push_back(name);
	}
}

std::vector<std::string> Node::path(Handle<Node> root) const
{
	std::vector<std::string> res;
	path(res, root);
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
		// Add this node to the result if it matches the current description
		if (state.atEndOfPath()) {
			if (state.typeMatches(type())) {
				state.addToResult(this);
				return true;
			}
		} else {
			size_t resCount = state.resultCount();
			continueResolve(state);
			return state.resultCount() > resCount;
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
		if (h->resolve(advancedState)) {
			state.foundCompositum = true;
			return true;
		}
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
			state.foundCompositum = true;
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
	// path and this node is the root node. Additionally only follow a reference
	// if the node the reference points to is known to contain the type that is
	// currently asked for in the resolution process
	if (canFollowReferences(state) && state.canContainType(h->type())) {
		ResolutionState forkedState = state.fork(this);
		return continueResolveCompositum(h, forkedState);
	}
	return false;
}

std::vector<ResolutionResult> Node::resolve(
    const std::vector<std::string> &path, const RttiType &type)
{
	// Create the state variables
	SharedResolutionState sharedState(path, type);
	ResolutionState state(sharedState, this);

	// Kickstart the resolution process by treating this very node as compositum
	if (path.size() > 0) {
		continueResolveCompositum(this, state);
	}

	// Return the results
	return sharedState.result;
}

std::vector<ResolutionResult> Node::resolve(const std::string &name,
                                            const RttiType &type)
{
	// Place the name in a vector and call the corresponding resolve function
	return resolve(std::vector<std::string>{name}, type);
}

/* Reflection Methods */

static Variant getNodeName(const Node *obj)
{
	return Variant::fromString(obj->getName());
}

static void setNodeName(const Variant &value, Node *obj)
{
	obj->setName(value.asString());
}

static Variant getNodeParent(const Node *obj)
{
	return Variant::fromObject(obj->getParent());
}

/* RTTI type registrations */
namespace RttiTypes {
const Rtti<ousia::Node> Node =
    TypedRttiBuilder<ousia::Node>("Node")
        .property("name", {RttiTypes::String, getNodeName, setNodeName})
        .property("parent", {Node, getNodeParent});
}
}

