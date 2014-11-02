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
	NodeDescriptor *getDescriptor(Node *n, bool create);

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
	void delNode(Node *n, NodeDescriptor *descr);

	/**
	 * Internal version of the delRef function with an additional "all"
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
	void delRef(Node *tar, Node *src, bool all);

public:
	NodeManager() : threshold(NODE_MANAGER_SWEEP_THRESHOLD) {}

	NodeManager(size_t threshold) : threshold(threshold) {}

	/**
	 * Deletes all nodes which are managed by this class.
	 */
	~NodeManager();

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
	void delRef(Node *tar, Node *src) { delRef(tar, src, false); }

	/**
	 * Performs garbage collection.
	 */
	void sweep();
};

template<class T>
class BaseHandle;

template<class T>
class RootedHandle;

template<class T>
class Handle;

/**
 * The Node class builds the main class in the DOM graph. The Node class
 * instances are managed by a NodeManager which performs garbage collection of
 * the Node instances. Do not pass raw Node pointers around, always wrap them
 * inside a RootedHandle or Handle class.
 */
class Node {
protected:
	NodeManager &mgr;

public:
	Node(NodeManager &mgr) : mgr(mgr){};

	virtual ~Node(){};

	NodeManager &getManager() { return mgr; }

	template <class T>
	Handle<T> acquire(const BaseHandle<T> &h) {
		return Handle<T>(h, this);
	}

	template <class T>
	Handle<T> acquire(BaseHandle<T> &&h) {
		return Handle<T>(h, this);
	}

	template <class T>
	Handle<T> acquire(T *t) {
		return Handle<T>(t, this);
	}

};

template <class T>
class BaseHandle {
protected:
	friend class RootedHandle<T>;
	friend class Handle<T>;

	static_assert(std::is_base_of<Node, T>::value, "T must be a Node");

	/**
	 * Reference to the represented node.
	 */
	T *ptr;

public:

	/**
	 * Constructor of the base handle class.
	 *
	 * @param ptr is the pointer to the node the handle should represent.
	 */
	BaseHandle(T *ptr) : ptr(ptr) {}

	/**
	 * Provides access to the underlying node.
	 */
	T *operator->() { return ptr; }

	/**
	 * Provides access to the underlying node.
	 */
	T &operator*() { return *ptr; }
};

/**
 * A RootedHandle represents a directed, garbage collected pointer at a Node
 * instance. The lifetime of the represented node is guaranteed to be at least
 * as long as the lifetime of the RootedHandle instance.
 */
template <class T>
class RootedHandle : public BaseHandle<T> {

private:
	void addRef()
	{
		if (BaseHandle<T>::ptr) {
			BaseHandle<T>::ptr->getManager().addRef(BaseHandle<T>::ptr,
			                                        nullptr);
		}
	}

	void delRef()
	{
		if (BaseHandle<T>::ptr) {
			BaseHandle<T>::ptr->getManager().delRef(BaseHandle<T>::ptr,
			                                        nullptr);
		}
	}

public:
	/**
	 * Creates an empty handle.
	 */
	RootedHandle() : BaseHandle<T>(nullptr){};

	/**
	 * Copies the given handle to this handle instance. Both handles are
	 * indistinguishable after the operation.
	 *
	 * @param h is the handle that should be asigned to this instance.
	 */
	RootedHandle(const RootedHandle<T> &h) : BaseHandle<T>(h.ptr) { addRef(); }

	/**
	 * Move constructor. Moves the given rvalue handle to this instance.
	 *
	 * @param h is the handle to be moved to this instance.
	 */
	RootedHandle(RootedHandle<T> &&h) : BaseHandle<T>(h.ptr)
	{
		h.ptr = nullptr;
	}

	/**
	 * Assignment operator. Assigns the given handle to this handle instance.
	 * Both handles are indistinguishable after the operation.
	 *
	 * @param h is the handle that should be asigned to this instance.
	 */
	RootedHandle<T> &operator=(const BaseHandle<T> &h)
	{
		delRef();
		this->ptr = h.ptr;
		addRef();
		return *this;
	}

	/**
	 * Move assignment operator. Moves the given rvalue handle into this
	 * instance.
	 *
	 * @param h is the handle to be moved to this instance.
	 */
	RootedHandle<T> &operator=(BaseHandle<T> &&h)
	{
		delRef();
		this->ptr = h.ptr;
		h.ptr = nullptr;
		return *this;
	}

	/**
	 * Constructor of the handle class.
	 *
	 * @param ptr is the node the handle should represent.
	 */
	RootedHandle(T *ptr) : BaseHandle<T>(ptr) { addRef(); }

	/**
	 * Constructor of the handle class.
	 *
	 * @param h is another handle whose Node should be used.
	 */
	RootedHandle(BaseHandle<T> h) : BaseHandle<T>(h.ptr) { addRef(); }

	/**
	 * Destructor of the RootedHandle class, deletes all refrences the class is
	 * still holding.
	 */
	~RootedHandle() { delRef(); }
};

/**
 * The handle class represents a directed, garbage collected pointer at a Node
 * instance. The lifetime of the represented node is guaranteed to be at last
 * as long as the lifetime of the Node instance which owns this reference.
 */
template <class T>
class Handle : public BaseHandle<T> {
private:
	Node *owner;

	void addRef()
	{
		if (BaseHandle<T>::ptr && owner) {
			owner->getManager().addRef(BaseHandle<T>::ptr, owner);
		}
	}

	void delRef()
	{
		if (BaseHandle<T>::ptr && owner) {
			owner->getManager().delRef(BaseHandle<T>::ptr, owner);
		}
	}

public:
	/**
	 * Creates an empty handle.
	 */
	Handle() : BaseHandle<T>(nullptr), owner(nullptr){};

	/**
	 * Copies the given handle to this handle instance. Both handles are
	 * indistinguishable after the operation. Note that especially the handle
	 * owner is copied.
	 *
	 * @param h is the handle that should be asigned to this instance.
	 */
	Handle(const Handle<T> &h) : BaseHandle<T>(h.ptr), owner(h.owner)
	{
		addRef();
	}

	/**
	 * Move constructor. Moves the given rvalue handle to this instance.
	 *
	 * @param h is the handle to be moved to this instance.
	 */
	Handle(Handle<T> &&h) : BaseHandle<T>(h.ptr), owner(h.owner)
	{
		h.ptr = nullptr;
	}

	/**
	 * Assignment operator. Assigns the given handle to this handle instance.
	 * Both handles are indistinguishable after the operation. Note that
	 * especially the handle owner is copied.
	 *
	 * @param h is the handle that should be asigned to this instance.
	 */
	Handle<T> &operator=(const Handle<T> &h)
	{
		delRef();
		this->ptr = h.ptr;
		this->owner = h.owner;
		addRef();
		return *this;
	}

	/**
	 * Move assignment operator. Moves the given rvalue handle into this
	 * instance.
	 *
	 * @param h is the handle to be moved to this instance.
	 */
	Handle<T> &operator=(Handle<T> &&h)
	{
		delRef();
		this->ptr = h.ptr;
		this->owner = h.owner;
		h.ptr = nullptr;
		return *this;
	}

	/**
	 * Constructor of the handle class.
	 *
	 * @param ptr is the node the handle should represent.
	 * @param owner is the node which owns this handle instance. The ptr node
	 * is guaranteed to live at least as long as the owner.
	 */
	Handle(T *ptr, Node *owner) : BaseHandle<T>(ptr), owner(owner) { addRef(); }

	/**
	 * Constructor of the handle class.
	 *
	 * @param h is another handle whose Node should be used.
	 * @param owner is the node which owns this handle instance. The ptr node
	 * is guaranteed to live at least as long as the owner.
	 */
	Handle(const BaseHandle<T> &h, Node *owner)
	    : BaseHandle<T>(h.ptr), owner(owner)
	{
		addRef();
	}

	/**
	 * Constructor of the handle class.
	 *
	 * @param h is another handle whose Node should be used.
	 * @param owner is the node which owns this handle instance. The ptr node
	 * is guaranteed to live at least as long as the owner.
	 */
	Handle(BaseHandle<T> &&h, Node *owner)
	    : BaseHandle<T>(h.ptr), owner(owner)
	{
		h.ptr = nullptr;
	}

	/**
	 * Destructor of the Handle class, deletes all refrences the class is still
	 * holding.
	 */
	~Handle() { delRef(); }
};

}
}

#endif /* _OUSIA_DOM_NODE_HPP_ */

