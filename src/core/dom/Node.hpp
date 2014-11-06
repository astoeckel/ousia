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

#ifndef _OUSIA_DOM_NODE_HPP_
#define _OUSIA_DOM_NODE_HPP_

#include <iostream>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace ousia {
namespace dom {

class Node;

template <class T>
class Handle;

template <class T>
class Rooted;

template <class T>
class Owned;

/**
 * Enum used for specifying the Reference direction.
 */
enum class RefDir { in, out };

/**
 * The NodeDescriptor class is used by the NodeManager for reference counting
 * and garbage collection. It describes the node reference multi graph with
 * adjacency lists.
 */
class NodeDescriptor {
public:
	/**
	 * Contains the number of references to rooted handles. A node whith at
	 * least one rooted reference is considered reachable.
	 */
	int rootRefCount;

	/**
	 * Map containing all references pointing at this node. The map key
	 * describes the node which points at this node, the map value contains the
	 * reference count from this node.
	 */
	std::map<Node *, int> refIn;

	/**
	 * Map containing all references pointing from this node at other nodes. The
	 * map key describes the target node and the map value the reference count.
	 */
	std::map<Node *, int> refOut;

	/**
	 * Default constructor of the NodeDescriptor class.
	 */
	NodeDescriptor() : rootRefCount(0){};

	/**
	 * Returns the total input degree of this node. The root references are also
	 * counted as incomming references and thus added to the result.
	 *
	 * @return the input degree of this node, including the root references.
	 */
	int refInCount() const;

	/**
	 * Returns the total output degree of this node.
	 *
	 * @return the output degree of this node.
	 */
	int refOutCount() const;

	/**
	 * Returns the input degree for the given node.
	 *
	 * @param n is the node for which the input degree should be returned,
	 * nullptr if the number of root references is returned.
	 * @return the input degree of the node or the rootRefCount if nullptr is
	 * given as node. If the node is not found, zero is returned.
	 */
	int refInCount(Node *n) const;

	/**
	 * Returns the output degree for the given node.
	 *
	 * @param n is the node for which the output degree should be returned.
	 * @return the output degree of the node. If the node is not found, zero is
	 * returned.
	 */
	int refOutCount(Node *n) const;

	/**
	 * Increments the input or output degree for the given node.
	 *
	 * @param dir is "in", increments the input degree, otherwise increments the
	 * output degree.
	 * @param n is the node for which the input or output degree should be
	 * incremented. If the given node is null, the rootRefCount is incremented,
	 * independent of the in parameter.
	 */
	void incrNodeDegree(RefDir dir, Node *n);

	/**
	 * Decrements the input or output degree for the given node.
	 *
	 * @param dir is "in", decrements the input degree, otherwise decrements the
	 * output degree.
	 * @param n is the node for which the input or output degree should be
	 * decremented. If the given node is null, the rootRefCount is decremented,
	 * independent of the in parameter.
	 * @param all specifies whether the node degree of the reference to this
	 * node should be set to zero, no matter what the actual degree is. This
	 * is set to true, when the given node is deleted and all references to it
	 * should be purged, no matter what.
	 * @return true if the node degree was sucessfully decremented.
	 */
	bool decrNodeDegree(RefDir dir, Node *n, bool all = false);
};

/**
 * Default sweep threshold used in the node manager. If the number of nodes
 * marked for sweeping reaches this threshold a garbage collection sweep is
 * performed.
 */
constexpr size_t NODE_MANAGER_SWEEP_THRESHOLD = 128;

class NodeManager {
protected:
	/**
	 * Threshold that defines the minimum number of entries in the "marked"
	 * set until "sweep" is called.
	 */
	const size_t threshold;

	/**
	 * Map used to store the node descriptors for all managed nodes. Every node
	 * that has at least one root, in or out reference has an entry in this map.
	 */
	std::unordered_map<Node *, NodeDescriptor> nodes;

	/**
	 * Set containing the nodes marked for sweeping.
	 */
	std::unordered_set<Node *> marked;

	/**
	 * Set containing nodes marked for deletion.
	 */
	std::unordered_set<Node *> deleted;

	/**
	 * Recursion depth while performing deletion. This variable is needed
	 * because the deletion of a node may cause further nodes to be deleted.
	 * Yet the actual deletion should only be performed at the uppermost
	 * recursion level.
	 */
	int deletionRecursionDepth = 0;

	/**
	 * Returns the node descriptor for the given node from the nodes map.
	 * Creates it if it does not exist and the "create" parameter is set to
	 * true.
	 */
	NodeDescriptor *getDescriptor(Node *n);

	/**
	 * Purges the nodes in the "deleted" set.
	 */
	void purgeDeleted();

	/**
	 * Function used internally to delete a node and clean up all references in
	 * the node manager still pointing at it.
	 *
	 * @param n is the node that should be deleted.
	 * @param
	 */
	void deleteNode(Node *n, NodeDescriptor *descr);

	/**
	 * Internal version of the deleteRef function with an additional "all"
	 * parameter. Removes a reference to the given target node from the source
	 * node.
	 *
	 * @param tar is the target node for which the reference from the given
	 * source node should be removed.
	 * @param src is the source node from which the target node was referenced
	 * or nullptr if the target node is referenced from the local scope.
	 * @param all specifies whether all (src, tar) references should be deleted,
	 * independent of the actual cardinality. This is set to true, when the
	 * given node is deleted and all references to it should be purged, no
	 * matter what.
	 */
	void deleteRef(Node *tar, Node *src, bool all);

public:
	NodeManager() : threshold(NODE_MANAGER_SWEEP_THRESHOLD) {}

	NodeManager(size_t threshold) : threshold(threshold) {}

	/**
	 * Deletes all nodes which are managed by this class.
	 */
	~NodeManager();

	/**
	 * Registers a node for being used with the NodeManager.
	 *
	 * @param n is the node which is registered for being used with the
	 * NodeManager.
	 */
	void registerNode(Node *n);

	/**
	 * Stores a reference to the given target node from the given source node.
	 * If the source pointer is set to nullptr, this means that the target node
	 * is rooted (semantic: it is reachable from the current scope) and should
	 * not be collected.
	 *
	 * @param tar is the target node to which the reference from src should be
	 * stored.
	 * @param src is the source node from which the target node is referenced or
	 * nullptr if the target node is referenced from the local scope.
	 */
	void addRef(Node *tar, Node *src);

	/**
	 * Removes a reference to the given target node from the source node.
	 *
	 * @param tar is the target node for which the reference from the given
	 * source node should be removed.
	 * @param src is the source node from which the target node was referenced
	 * or nullptr if the target node is referenced from the local scope.
	 */
	void deleteRef(Node *tar, Node *src) { deleteRef(tar, src, false); }

	/**
	 * Performs garbage collection.
	 */
	void sweep();
};

/**
 * The Node class builds the main class in the DOM graph. The Node class
 * instances are managed by a NodeManager which performs garbage collection of
 * the Node instances. Do not pass raw Node pointers around, always wrap them
 * inside a Rooted or Owned class.
 */
class Node {
protected:
	NodeManager &mgr;

public:
	Node(NodeManager &mgr) : mgr(mgr) { mgr.registerNode(this); };

	virtual ~Node(){};

	NodeManager &getManager() { return mgr; }

	template <class T>
	Owned<T> acquire(const Handle<T> &h)
	{
		return Owned<T>{h, this};
	}

	template <class T>
	Owned<T> acquire(Handle<T> &&h)
	{
		return Owned<T>{h, this};
	}

	template <class T>
	Owned<T> acquire(T *t)
	{
		return Owned<T>{t, this};
	}
};

template <class T>
class Handle {
protected:
	friend class Rooted<T>;
	friend class Owned<T>;

	static_assert(std::is_base_of<Node, T>::value, "T must be a Node");

	/**
	 * Reference to the represented node.
	 */
	T *ptr;

public:
	/**
	 * Constructor of the base Owned class.
	 *
	 * @param ptr is the pointer to the node the Owned should represent.
	 */
	Handle(T *ptr) : ptr(ptr) {}

	/**
	 * Copies the given Handle to this Handle instance.
	 *
	 * @param h is the Handle that should be asigned to this instance.
	 */
	Handle(const Handle<T> &h) : ptr(h.get()) {}

	/**
	 * Copies the given Handle for a derived class to this Handle instance.
	 *
	 * @param h is the Handle that should be asigned to this instance.
	 */
	template <class T2>
	Handle(const Handle<T2> &h)
	    : ptr(h.get())
	{
	}

	/**
	 * Returns the underlying pointer.
	 */
	T *get() const { return ptr; }

	/**
	 * Provides access to the underlying node.
	 */
	T *operator->() { return ptr; }

	/**
	 * Provides access to the underlying node.
	 */
	T &operator*() { return *ptr; }

	/**
	 * Comparison operator between base Owned and base Owned.
	 */
	bool operator==(const Handle &h) const { return ptr == h.ptr; }

	/**
	 * Comparison operator between base Owned and pointer.
	 */
	friend bool operator==(const Handle &h, const Node *n)
	{
		return h.ptr == n;
	}

	/**
	 * Comparison operator between base Owned and pointer.
	 */
	friend bool operator==(const Node *n, const Handle &h)
	{
		return h.ptr == n;
	}

	/**
	 * Returns true if the handle is the null pointer.
	 */
	bool isNull() const { return ptr == nullptr; }

	/**
	 * Returns true if the handle is the null pointer.
	 */
	bool operator!() const { return isNull(); }
};

/**
 * Null represents a null handle.
 */
static const Handle<Node> Null{nullptr};

/**
 * A Rooted represents a directed, garbage collected pointer at a Node
 * instance. The lifetime of the represented node is guaranteed to be at least
 * as long as the lifetime of the Rooted instance.
 */
template <class T>
class Rooted : public Handle<T> {
private:
	void addRef()
	{
		if (Handle<T>::ptr) {
			Handle<T>::ptr->getManager().addRef(Handle<T>::ptr, nullptr);
		}
	}

	void deleteRef()
	{
		if (Handle<T>::ptr) {
			Handle<T>::ptr->getManager().deleteRef(Handle<T>::ptr, nullptr);
		}
	}

public:
	/**
	 * Creates an empty Owned.
	 */
	Rooted() : Handle<T>(nullptr){};

	/**
	 * Copies the given Rooted to this Rooted instance. Both handles
	 * are indistinguishable after the operation.
	 *
	 * @param h is the Owned that should be asigned to this instance.
	 */
	Rooted(const Rooted<T> &h) : Handle<T>(h.ptr) { addRef(); }

	/**
	 * Move constructor. Moves the given rvalue Rooted to this instance.
	 *
	 * @param h is the Rooted to be moved to this instance.
	 */
	Rooted(Rooted<T> &&h) : Handle<T>(h.ptr) { h.ptr = nullptr; }

	/**
	 * Constructor of the Owned class.
	 *
	 * @param ptr is the node the Owned should represent.
	 */
	Rooted(T *ptr) : Handle<T>(ptr) { addRef(); }

	/**
	 * Constructor of the Owned class.
	 *
	 * @param h is another Owned whose Node should be used.
	 */
	template <class T2>
	Rooted(const Handle<T2> &h)
	    : Handle<T>(h.get())
	{
		addRef();
	}

	/**
	 * Assignment operator. Assigns the given Owned to this Owned instance.
	 * Both handles are indistinguishable after the operation.
	 *
	 * @param h is the Owned that should be asigned to this instance.
	 */
	Rooted<T> &operator=(const Rooted<T> &h)
	{
		deleteRef();
		this->ptr = h.ptr;
		addRef();
		return *this;
	}

	/**
	 * Move assignment operator. Moves the given rvalue Owned into this
	 * instance.
	 *
	 * @param h is the Owned to be moved to this instance.
	 */
	Rooted<T> &operator=(Rooted<T> &&h)
	{
		deleteRef();
		this->ptr = h.ptr;
		h.ptr = nullptr;
		return *this;
	}

	/**
	 * Assignment operator. Assigns the given Owned to this Owned instance.
	 * Both handles are indistinguishable after the operation.
	 *
	 * @param h is the Owned that should be asigned to this instance.
	 */
	template <class T2>
	Rooted<T> &operator=(const Handle<T2> &h)
	{
		deleteRef();
		this->ptr = h.get();
		addRef();
		return *this;
	}

	/**
	 * Move assignment operator. Moves the given rvalue Owned into this
	 * instance.
	 *
	 * @param h is the Owned to be moved to this instance.
	 */
	Rooted<T> &operator=(Handle<T> &&h)
	{
		deleteRef();
		this->ptr = h.ptr;
		h.ptr = nullptr;
		return *this;
	}

	/**
	 * Destructor of the Rooted class, deletes all refrences the class is
	 * still holding.
	 */
	~Rooted() { deleteRef(); }
};

/**
 * The Owned class represents a directed, garbage collected pointer at a Node
 * instance. The lifetime of the represented node is guaranteed to be at last
 * as long as the lifetime of the Node instance which owns this reference.
 */
template <class T>
class Owned : public Handle<T> {
private:
	Node *owner;

	void addRef()
	{
		if (Handle<T>::ptr && owner) {
			owner->getManager().addRef(Handle<T>::ptr, owner);
		}
	}

	void deleteRef()
	{
		if (Handle<T>::ptr && owner) {
			owner->getManager().deleteRef(Handle<T>::ptr, owner);
		}
	}

public:
	/**
	 * Creates an empty Owned.
	 */
	Owned() : Handle<T>(nullptr), owner(nullptr){};

	/**
	 * Copies the given Owned to this Owned instance. Both handles are
	 * indistinguishable after the operation. Note that especially the Owned
	 * owner is copied.
	 *
	 * @param h is the Owned that should be asigned to this instance.
	 */
	Owned(const Owned<T> &h) : Handle<T>(h.get()), owner(h.getOwner())
	{
		addRef();
	}

	/**
	 * Copies the given Owned of another derived type to this Owned instance.
	 * Both handles are indistinguishable after the operation (except for the
	 * type). Note that especially the Owned owner is copied.
	 *
	 * @param h is the Owned that should be asigned to this instance.
	 */
	template <class T2>
	Owned(const Owned<T2> &h)
	    : Handle<T>(h.get()), owner(h.getOwner())
	{
		addRef();
	}

	/**
	 * Move constructor. Moves the given rvalue Owned to this instance.
	 *
	 * @param h is the Owned to be moved to this instance.
	 */
	Owned(Owned<T> &&h) : Handle<T>(h.get()), owner(h.getOwner())
	{
		h.ptr = nullptr;
	}

	/**
	 * Assignment operator. Assigns the given Owned to this Owned instance.
	 * Both handles are indistinguishable after the operation. Note that
	 * especially the Owned owner is copied.
	 *
	 * @param h is the Owned that should be asigned to this instance.
	 */
	Owned<T> &operator=(const Owned<T> &h)
	{
		deleteRef();
		this->ptr = h.ptr;
		this->owner = h.getOwner();
		addRef();
		return *this;
	}

	/**
	 * Move assignment operator. Moves the given rvalue Owned into this
	 * instance.
	 *
	 * @param h is the Owned to be moved to this instance.
	 */
	Owned<T> &operator=(Owned<T> &&h)
	{
		deleteRef();
		this->ptr = h.ptr;
		this->owner = h.getOwner();
		h.ptr = nullptr;
		return *this;
	}

	/**
	 * Constructor of the Owned class.
	 *
	 * @param ptr is the node the Owned should represent.
	 * @param owner is the node which owns this Owned instance. The ptr node
	 * is guaranteed to live at least as long as the owner.
	 */
	Owned(T *ptr, Node *owner) : Handle<T>(ptr), owner(owner) { addRef(); }

	/**
	 * Constructor of the Owned class.
	 *
	 * @param h is another Owned whose Node should be used.
	 * @param owner is the node which owns this Owned instance. The ptr node
	 * is guaranteed to live at least as long as the owner.
	 */
	template <class T2>
	Owned(const Handle<T2> &h, Node *owner)
	    : Handle<T>(h.get()), owner(owner)
	{
		addRef();
	}

	/**
	 * Destructor of the Owned class, deletes all refrences the class is still
	 * holding.
	 */
	~Owned() { deleteRef(); }

	/**
	 * Returns the reference to the owner of the Owned.
	 *
	 * @return the Owned owner.
	 */
	Node *getOwner() const { return owner; }
};
}
}

#endif /* _OUSIA_DOM_NODE_HPP_ */

