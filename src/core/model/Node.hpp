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
 * @file Node.hpp
 *
 * Contains the definition of the Node class, the base class used by all model
 * classes.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_NODE_HPP_
#define _OUSIA_NODE_HPP_

#include <cstdint>
#include <map>
#include <set>
#include <unordered_set>
#include <string>
#include <vector>

#include <core/managed/Managed.hpp>
#include <core/managed/ManagedContainer.hpp>

#include "Index.hpp"

namespace ousia {

// Forward declarations
class Logger;
class RttiType;
template <class T>
class Rtti;

/**
 * Describes the validity of a node structure.
 */
enum class ValidationState : uint8_t {
	/**
     * The validity is set to UNKNOWN if the Node has not yet been validated or
     * the validation state has been reset (because the node was changed).
     */
	UNKNOWN,

	/**
     * The validity is set to validating if the Node has not yet been validated,
     * but its validation is currently running. This flag is used to prevent
     * recursion.
     */
	VALIDATING,

	/**
     * The validity is set to VALID if the Node has been validaten and is known
     * to be valid.
     */
	VALID,

	/**
     * The validity is set to INVALID if the Node has been validated and is
     * known to be invalid.
     */
	INVALID
};

/**
 * Structure describing a single result obtained from the resolution function.
 */
struct ResolutionResult {
	/**
	 * The actual node that was resolved.
	 */
	Rooted<Node> node;

	/**
	 * Root node of the subtree in which the node was found. This e.g. points to
	 * the Domain in which a Structure was defined or the Typesystem in which a
	 * Type was defined. May be nullptr.
	 */
	Rooted<Node> resolutionRoot;

	/**
	 * Default constructor of the ResolutionResult class.
	 *
	 * @param node is a reference pointing at the actually found node.
	 * @param resolutionRoot is a reference to the root node of the subtree in
	 * which the node was found.
	 */
	ResolutionResult(Handle<Node> node, Handle<Node> resolutionRoot)
	    : node(node), resolutionRoot(resolutionRoot)
	{
	}

	/**
	 * Returns a canonical path leading to the node. The path is relative to the
	 * resolutionRoot (the root node of the subgraph the node was defined in).
	 *
	 * @return a canonical path leading to the node.
	 */
	std::vector<std::string> path() const;
};

// Forward declaration
class ResolutionState;

/**
 * The Node class builds the base class for any Node within the DOM graph. A
 * node may either be a descriptive node (such as a domain description etc.)
 * or a document element. Each node is identified by acharacteristic name and
 * a parent element. Note that the node name is not required to be unique. Nodes
 * without parent are considered root nodes.
 */
class Node : public Managed {
private:
	/**
	 * Name of the node. As names are always looked up relative to a node,
	 * names are not required to be unique.
	 */
	std::string name;

	/**
	 * Reference to a parent node instace.
	 */
	Owned<Node> parent;

	/**
	 * A "dirty" flag that signifies if this Node has been already validated
	 * or not.
	 */
	mutable ValidationState validationState;

	/**
	 * Private version of the "path" function used to construct the path. Calls
	 * the path function of the parent node and adds the own name to the given
	 * vector.
	 *
	 * @param p is the list the path should be constructed in.
	 */
	void path(std::vector<std::string> &p, Handle<Node> root) const;

	/**
	 * Returns true if the resolution process is just at the beginning (no part
	 * of the path has been matched yet).
	 *
	 * @param state is used internally to manage the resolution process.
	 * @return true if the resolution is at the beginning, false otherwise.
	 */
	static bool canFollowComposita(ResolutionState &state);

	/**
	 * Returns true if following references is currently allowed in the
	 * resolution process. This is the case if the resolution is currently at
	 * the beginning (no part of the path has been matched yet) and this node
	 * is the current resolution root node.
	 *
	 * @param state is used internally to manage the resolution process.
	 * @return true if references can be followed, false otherwise.
	 */
	static bool canFollowReferences(ResolutionState &state);

	/**
	 * Method used internally for resolving nodes with a certain name and type
	 * within the object graph.
	 *
	 * @param state is used internally to manage the resolution process.
	 * @return true if an matching element was found within this subtree, false
	 * otherwise.
	 * @return true if at least one new node has been found that matches the
	 * criteria given for the resolution.
	 */
	bool resolve(ResolutionState &state);

	/**
	 * Method used internally to check whether the given index has an entry
	 * which matches the one currently needed to continue the path.
	 *
	 * @param index is a reference to the index from which the currently active
	 * path element should be looked up.
	 * @param state is used internally to manage the resolution process.
	 * @return true if at least one new node has been found that matches the
	 * criteria given for the resolution.
	 */
	bool continueResolveIndex(const Index &index, ResolutionState &state);

	/**
	 * Checks whether the name of the given node is already stored in the given
	 * set, if yes, logs a corresponding error message.
	 *
	 * @param node is the node of which the name should be checked.
	 * @param names is a set in which all encountered names are stored.
	 * @param logger is the logger instance to which error messages are written.
	 * @return true if the given node has a unique name, false otherwise.
	 */
	bool checkDuplicate(Handle<Node> node,
	                    std::unordered_set<std::string> &names,
	                    Logger &logger) const;

protected:
	/**
	 * Function which should be overwritten by derived classes in order to
	 * resolve node names to a list of possible nodes. The implementations of
	 * this function do not need to do anything but call the
	 * continueResolveComposita() and/or continueResolveReferences() methods on
	 * any index or list of references and pass the resolution state to these
	 * methods.
	 *
	 * @param state is used internally to manage the resolution process.
	 */
	virtual void doResolve(ResolutionState &state);

	/**
	 * Tries to advance the resolution process with the compositum pointed at
	 * by h. If a part of the resolution path has already been matched, only
	 * decends into the given node if the path can be continued. Otherwise
	 * always decends into the node to search for potential beginnings of the
	 * path.
	 *
	 * @param h is a handle at a compositum (a node the current node consists of
	 * or explicitly defines).
	 * @param state is used internally to manage the resolution process.
	 * @return true if at least one new node has been found that matches the
	 * criteria given for the resolution.
	 */
	bool continueResolveCompositum(Handle<Node> h, ResolutionState &state);

	/**
	 * Calls continueResolveCompositum() for each element in the given
	 * container.
	 *
	 * @param container is a container containing compositum nodes for which the
	 * continueResolveCompositum() method should be called.
	 * @param state is used internally to manage the resolution process.
	 * @return true if at least one new node has been found that matches the
	 * criteria given for the resolution.
	 */
	template <class T>
	bool continueResolveComposita(T &container, ResolutionState &state)
	{
		bool res = false;
		for (auto elem : container) {
			res = continueResolveCompositum(elem, state) | res;
		}
		return res;
	}

	/**
	 * Calls continueResolveCompositum() for each element in the given
	 * container. Uses the given index to speed up the resolution process.
	 *
	 * @param container is a container containing compositum nodes for which the
	 * continueResolveCompositum() method should be called.
	 * @param index is the Index instance of the given container and is used to
	 * speed up the resolution process.
	 * @param state is used internally to manage the resolution process.
	 * @return true if at least one new node has been found that matches the
	 * criteria given for the resolution.
	 */
	template <class T>
	bool continueResolveComposita(T &container, const Index &index,
	                              ResolutionState &state)
	{
		if (continueResolveIndex(index, state)) {
			return true;
		}
		if (canFollowComposita(state)) {
			return continueResolveComposita(container, state);
		}
		return false;
	}

	/**
	 * Tries to search for the requested node in another subtree to which a
	 * reference exists from this node.
	 *
	 * @param h is a handle pointing at the node in the subtree.
	 * @param state is used internally to manage the resolution process.
	 * @return true if at least one new node has been found that matches the
	 * criteria given for the resolution.
	 */
	bool continueResolveReference(Handle<Node> h, ResolutionState &state);

	/**
	 * Tries to search for the requested node in another subtree to which a
	 * reference exists from this node.
	 *
	 * @param h is a handle pointing at the node in the subtree.
	 * @param state is used internally to manage the resolution process.
	 * @return true if at least one new node has been found that matches the
	 * criteria given for the resolution.
	 */
	template <class T>
	bool continueResolveReferences(T &container, ResolutionState &state)
	{
		if (canFollowReferences(state)) {
			bool res = false;
			for (auto elem : container) {
				res = continueResolveReference(elem, state) | res;
			}
			return res;
		}
		return false;
	}

	/**
	 * This method should be called if the internal state of this Node is
	 * changed such that a new validation run has to be made. Also informs the
	 * parent node about the invalidation.
	 */
	void invalidate();

	/**
	 * This method should be called if a Node finds itself in an invalid state.
	 */
	void markInvalid();

	/**
	 * The convention for this function is as follows:
	 * 1.) The child should validate itself and return false, if constraints are
	 *     not met. Errors should be logged if and only if false is returned.
	 * 3.) It should call validate on all children.
	 *     If some child returns false this method should return false as well.
	 * 4.) If all children could be validated this method should return true.
	 *
	 * The default and trivial behaviour of this function is to return true.
	 *
	 * @param logger is a logger for error messages if false is returned.
	 * @return true if this is a valid node and false if it is not.
	 */
	virtual bool doValidate(Logger &logger) const;

	/**
	 * Makes sure the name of this node is a valid identifier and loggs a
	 * corresponding error message.
	 *
	 * @param logger is the logger to which the error message is logged.
	 * @return true if the name is valid, false otherwise.
	 */
	bool validateName(Logger &logger) const;

	/**
	 * Helper function that can be used to forward the validation process to
	 * child nodes.
	 *
	 * @tparam T is the type of the list that should be handled.
	 * @param list is a list of arbitrary kind. The "validate" function is
	 * called for all elementsd of the list.
	 * @param logger is the logger to which any errors should be reported.
	 */
	template <class T>
	bool continueValidation(const T &list, Logger &logger) const
	{
		bool res = true;
		for (auto elem : list) {
			res = elem->validate(logger) & res;
		}
		return res;
	}

	/**
	 * Helper function that can be used to forward the validation process to
	 * child nodes while at the same time checking that the children have no
	 * duplicated names.
	 *
	 * @tparam T is the type of the list that should be handled.
	 * @param list is a list of arbitrary kind. The "validate" function is
	 * called for all elementsd of the list.
	 * @param logger is the logger to which any errors should be reported.
	 */
	template <class T>
	bool continueValidationCheckDuplicates(const T &list, Logger &logger) const
	{
		bool res = true;
		std::unordered_set<std::string> names;
		for (auto elem : list) {
			res = elem->validate(logger) & checkDuplicate(elem, names, logger) &
			      res;
		}
		return res;
	}

public:
	/**
	 * Initializes the node with empty name and parent.
	 *
	 * @param mgr is a reference to the Manager instace the node belongs to.
	 */
	Node(Manager &mgr, Handle<Node> parent = nullptr)
	    : Managed(mgr),
	      parent(acquire(parent)),
	      validationState(ValidationState::UNKNOWN)
	{
	}

	/**
	 * Constructs a new node with the given name and the given parent element.
	 *
	 * @param mgr is a reference to the Manager instace the node belongs to.
	 * @param name is the name of the Node.
	 * @param parent is a handle pointing at the parent node.
	 */
	Node(Manager &mgr, std::string name, Handle<Node> parent = nullptr)
	    : Managed(mgr),
	      name(name),
	      parent(acquire(parent)),
	      validationState(ValidationState::UNKNOWN)
	{
	}

	/**
	 * Sets the name of the node to the given name. Note: The name set here may
	 * be invalid (contain spaces, colons or other special characters). However,
	 * in this case the node will not be reachable as reference from a input
	 * document. This behaviour allows for gracefully degradation in error
	 * cases.
	 *
	 * @param name is the name that should be assigned to the node.
	 */
	void setName(std::string name);

	/**
	 * Returns the name of the node.
	 */
	const std::string &getName() const { return name; }

	/**
	 * Specifies whether the node has a name, e.g. whether the current name is
	 * not empty.
	 *
	 * @return true if the name of this node is not empty, false otherwise.
	 */
	bool hasName() const { return !name.empty(); }

	/**
	 * Returns a handle to the parent node of the Node instance.
	 *
	 * @return a handle to the root node.
	 */
	Rooted<Managed> getParent() const { return parent; }

	/**
	 * Returns true, if the node does not have a parent. Root nodes may either
	 * be the root element of the complete DOM tree
	 *
	 * @return true if the node is a root node (has no parent) or false if the
	 * node is no root node (has a parent).
	 */
	bool isRoot() const { return parent.isNull(); }

	/**
	 * Returns the vector containing the complete path to this node (including
	 * the name of the parent nodes).
	 *
	 * @param root is the node up to which the path should be returned. Ignored
	 * if set to nullptr.
	 * @return a vector containing the path (starting with the root node) to
	 * this node as a list of names.
	 */
	std::vector<std::string> path(Handle<Node> root = nullptr) const;

	/**
	 * Function which resolves a name path to a list of possible nodes starting
	 * from this node.
	 *
	 * @param path is a list specifying a path of node names meant to specify a
	 * certain named node.
	 * @param type specifies the type of the node that should be located.
	 * @return a vector containing ResolutionResult structures which describe
	 * the resolved elements.
	 */
	std::vector<ResolutionResult> resolve(const std::vector<std::string> &path,
	                                      const RttiType &type);

	/**
	 * Function which resolves a single name to a list of possible nodes
	 * starting from this node.
	 *
	 * @param name is the name which should be resolved.
	 * @param type specifies the type of the node that should be located.
	 * @return a vector containing ResolutionResult structures which describe
	 * the resolved elements.
	 */
	std::vector<ResolutionResult> resolve(const std::string &name,
	                                      const RttiType &type);

	/**
	 * Checks whether this node is valid and returns true if it is and false
	 * if it is not. If the node is invalid further information will be appended
	 * to the logger.
	 *
	 * @param logger is a logger where errors will be logged if this Node is
	 * invalid.
	 * @return true if this Node is valid.
	 */
	bool validate(Logger &logger) const;
};

/**
 * The NodeVector class is a vector capable of automatically maintaining an
 * index used for the resolution of node names.
 *
 * @tparam T is the type that should be stored in the NodeVector. Must be a
 * descendant of the Node class.
 * @tparam Listener is the listener class that should be used to build the
 * internal index. Should either be Index or a reference to (&Index) in case a
 * shared index is used.
 */
template <class T, class Listener = Index>
class NodeVector
    : public ManagedGenericList<T, std::vector<Handle<T>>,
                                ListAccessor<Handle<T>>, Listener> {
public:
	using Base = ManagedGenericList<T, std::vector<Handle<T>>, ListAccessor<Handle<T>>,
	                         Listener>;
	using Base::Base;

	/**
	 * Returns the reference to the internal index.
	 */
	const Index &getIndex() const { return this->listener; }

	/**
	 * Returns the reference to the internal index.
	 */
	Index &getIndex() { return this->listener; }
};

/**
 * The NodeMap class is a map class capable of automatically maintaining an
 * index used for the resolution of node names.
 *
 * @tparam K is the key type that should be stored in the NodeMap.
 * @tparam T is the value type that should be stored in the NodeMap. Must be a
 * descendant of the Node class.
 * @tparam Listener is the listener class that should be used to build the
 * internal index. Should either be Index or a reference to (&Index) in case a
 * shared index is used.
 */
template <class K, class T, class Listener = Index>
class NodeMap
    : public ManagedGenericMap<K, T, std::map<K, Handle<T>>,
                               MapAccessor<std::pair<K, Handle<T>>>, Listener> {
public:
	using Base = ManagedGenericMap<K, T, std::map<K, Handle<T>>,
	                        MapAccessor<std::pair<K, Handle<T>>>,
	                        Listener>;
	using Base::Base;

	/**
	 * Returns the reference to the internal index.
	 */
	const Index &getIndex() const { return this->listener; }

	/**
	 * Returns the reference to the internal index.
	 */
	Index &getIndex() { return this->listener; }
};

namespace RttiTypes {
/**
 * Typeinformation for the base "Node" class.
 */
extern const Rtti<Node> Node;
}
}

#endif /* _OUSIA_NODE_HPP_ */

