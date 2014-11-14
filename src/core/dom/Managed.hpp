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

#ifndef _OUSIA_DOM_MANAGED_HPP_
#define _OUSIA_DOM_MANAGED_HPP_

#include <iostream>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace ousia {
namespace dom {

// TODO: Implement clone, getReferenced and getReferencing

class Managed;

template <class T>
class Handle;

template <class T>
class Rooted;

template <class T>
class Owned;

/**
 * Enum used to specify the direction of a object reference (inbound or
 * outbound).
 */
enum class RefDir { in, out };

/**
 * The ObjectDescriptor struct is used by the Manager for reference counting and
 * garbage collection. It describes the reference multigraph with adjacency
 * lists. Each ObjectDescriptor instance represents a single managed object and
 * its assocition to and from other managed objects (nodes in the graph).
 */
struct ObjectDescriptor {
public:
	/**
	 * Contains the number of references to rooted handles. A managed objects
	 * whith at least one rooted reference is considered reachable.
	 */
	int rootRefCount;

	/**
	 * Map containing all references pointing at this managed object. The
	 * map key describes the object which points at this object, the map
	 * value contains the reference count from this object.
	 */
	std::map<Managed *, int> refIn;

	/**
	 * Map containing all references pointing from this managed object to
	 * other managed objects. The map key describes the target object and
	 * the map value the reference count.
	 */
	std::map<Managed *, int> refOut;

	/**
	 * Default constructor of the ObjectDescriptor class.
	 */
	ObjectDescriptor() : rootRefCount(0){};

	/**
	 * Returns the total input degree of this managed object. The root
	 * references are also counted as incomming references and thus added to
	 * the result.
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
	 * Returns the input degree for the given managed object.
	 *
	 * @param o is the node for which the input degree should be returned,
	 * nullptr if the number of root references is returned.
	 * @return the input degree of the node or the rootRefCount if nullptr
	 * is given as node. If the node is not found, zero is returned.
	 */
	int refInCount(Managed *o) const;

	/**
	 * Returns the output degree for the given node.
	 *
	 * @param o is the node for which the output degree should be returned.
	 * @return the output degree of the node. If the node is not found, zero
	 * is returned.
	 */
	int refOutCount(Managed *o) const;

	/**
	 * Increments the input or output degree for the represented managed
	 * object.
	 *
	 * @param dir describes the direction of the association. A value of
	 * "in", increments the input degree, otherwise increments the output
	 * degree.
	 * @param o is the managed object for which the input or output degree
	 * should be incremented. If the given object is null, the rootRefCount
	 * is incremented, independent of the dir parameter.
	 */
	void incrDegree(RefDir dir, Managed *o);

	/**
	 * Decrements the input or output degree for the given managed object.
	 *
	 * @param dir describes the direction of the association. A value of
	 * "in", increments the input degree, otherwise increments the output
	 * degree.
	 * @param o is the managed object for which the input or output degree
	 * should be incremented. If the given object is null, the rootRefCount
	 * is incremented, independent of the dir parameter.
	 * @param all specifies whether the degree of the reference to this
	 * object should be set to zero, no matter what the actual degree is.
	 * This parameter is used when the given object is deleted and all
	 * references to it should be purged, no matter what.
	 * @return true if the degree was sucessfully decremented.
	 */
	bool decrDegree(RefDir dir, Managed *o, bool all = false);
};

class Manager {
private:
	/**
	 * Default sweep threshold. If the number of managed objects marked for
	 * sweeping reaches this threshold a garbage collection sweep is performed.
	 */
	static constexpr size_t SWEEP_THRESHOLD = 128;

protected:
	/**
	 * Threshold that defines the minimum number of entries in the "marked"
	 * set until "sweep" is called.
	 */
	const size_t threshold;

	/**
	 * Map used to store the descriptors for all managed objects. Every object
	 * that has at least one root, in or out reference has an entry in this map.
	 */
	std::unordered_map<Managed *, ObjectDescriptor> objects;

	/**
	 * Set containing the objects marked for sweeping.
	 */
	std::unordered_set<Managed *> marked;

	/**
	 * Set containing objects marked for deletion.
	 */
	std::unordered_set<Managed *> deleted;

	/**
	 * Recursion depth while performing deletion. This variable is needed
	 * because the deletion of an object may cause further objects to be
	 * deleted. Yet the actual deletion should only be performed at the
	 * uppermost recursion level.
	 */
	int deletionRecursionDepth = 0;

	/**
	 * Returns the object ObjectDescriptor for the given object from the objects map.
	 */
	ObjectDescriptor *getDescriptor(Managed *o);

	/**
	 * Purges the objects in the "deleted" set.
	 */
	void purgeDeleted();

	/**
	 * Function used internally to delete a object and clean up all references
	 * in the object manager still pointing at it.
	 *
	 * @param o is the object that should be deleted.
	 * @param descr is a reference to the ObjectDescriptor of the given object.
	 */
	void deleteObject(Managed *o, ObjectDescriptor *descr);

	/**
	 * Internal version of the deleteRef function with an additional "all"
	 * parameter. Removes a reference to the given target object from the source
	 * object.
	 *
	 * @param tar is the target object for which the reference from the given
	 * source object should be removed.
	 * @param src is the source object from which the target object was
	 * referenced or nullptr if the target object is referenced from the local
	 * scope.
	 * @param all specifies whether all (src, tar) references should be deleted,
	 * independent of the actual cardinality. This is set to true, when the
	 * given object is deleted and all references to it should be purged, no
	 * matter what.
	 */
	void deleteRef(Managed *tar, Managed *src, bool all);

public:
	Manager() : threshold(SWEEP_THRESHOLD) {}

	Manager(size_t threshold) : threshold(threshold) {}

	/**
	 * Deletes all objects managed by this class.
	 */
	~Manager();

	/**
	 * Registers an object for being managed by the Manager. The Manager now has
	 * the sole responsibility for freeing the managed object. Under no
	 * circumstances free the object manually, this will result in double frees.
	 *
	 * @param o is the object which is registered for being used with the
	 * Manager.
	 */
	void manage(Managed *o);

	/**
	 * Stores a reference to the given target object from the given source
	 * object. If the source pointer is set to nullptr, this means that the
	 * target object is rooted (semantic: it is reachable from the current
	 * scope) and should not be collected.
	 *
	 * @param tar is the target object to which the reference from src should be
	 * stored.
	 * @param src is the source object from which the target object is
	 * referenced or nullptr if the target object is referenced from the local
	 * scope.
	 */
	void addRef(Managed *tar, Managed *src);

	/**
	 * Removes a reference to the given target object from the source object.
	 *
	 * @param tar is the target object for which the reference from the given
	 * source object should be removed.
	 * @param src is the source object from which the target object was
	 * referenced or nullptr if the target object is referenced from the local
	 * scope.
	 */
	void deleteRef(Managed *tar, Managed *src) { deleteRef(tar, src, false); }

	/**
	 * Performs garbage collection.
	 */
	void sweep();
};

/**
 * The Managed class represents a garbage collected object. Instances of the
 * Managed class are managed (e.g. freed) by an instance of the Manager class.
 * Never free instances of this class yourself (even by playing an instance of
 * this class on the steck). Create any new instance of any managed object with
 * the makeRooted and makeOwned functions.
 */
class Managed {
protected:
	Manager &mgr;

public:
	Managed(Manager &mgr) : mgr(mgr) { mgr.manage(this); };

	virtual ~Managed(){};

	Manager &getManager() { return mgr; }

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

/**
 * The Handle class is the base class for handles pointing at managed objects.
 * It implements methods for comparing handles to each other and to pointers
 * of the represented managed object type. Furthermore all other handle types
 * and pointers can be conveniently converted to a Handle instance. However,
 * the Handle class does not qualify the represented pointer for garbage
 * collection. Thus the Handle class should only be used as type for input
 * parameters in methods/functions and at no other ocasion. Use the Rooted or
 * the Owned class if the represented object should actually be garbage
 * collected.
 */
template <class T>
class Handle {
protected:
	friend class Rooted<T>;
	friend class Owned<T>;

	static_assert(std::is_convertible<T*, Managed*>::value, "T must be a Managed");

	/**
	 * Reference to the represented managed object.
	 */
	T *ptr;

public:
	/**
	 * Constructor of the base Handle class.
	 *
	 * @param ptr is the pointer to the managed object the Handle should
	 * represent.
	 */
	Handle(T *ptr) : ptr(ptr) {}

	/**
	 * Copies the given Handle to this Handle instance.
	 *
	 * @param h is the Handle that should be asigned to this instance.
	 */
	Handle(const Handle<T> &h) : ptr(h.get()) {}

	/**
	 * Copies the given Handle for a managed object of a derived class to this
	 * Handle instance.
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
	 * Provides access to the underlying managed object.
	 */
	T *operator->() { return ptr; }

	/**
	 * Provides access to the underlying managed object for immutable handles.
	 */
	const T *operator->() const { return ptr; }

	/**
	 * Provides access to the underlying managed object.
	 */
	T &operator*() { return *ptr; }

	/**
	 * Provides access to the underlying managed object for immutable handles.
	 */
	const T &operator*() const { return *ptr; }

	/**
	 * Comparison operator between base Owned and base Owned.
	 */
	template<class T2>
	bool operator==(const Handle<T2> &h) const { return ptr == h.get(); }

	/**
	 * Comparison operator between base Owned and pointer.
	 */
	friend bool operator==(const Handle<T> &h, const Managed *o)
	{
		return h.get() == o;
	}

	/**
	 * Comparison operator between base Owned and pointer.
	 */
	friend bool operator==(const Managed *o, const Handle<T> &h)
	{
		return o == h.get();
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
 * A Rooted represents a directed, garbage collected pointer at a managed
 * object. The lifetime of the represented managed object is guaranteed to be at
 * least as long as the lifetime of the Rooted handle instance.
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
	 * Constructor of the Rooted class.
	 *
	 * @param ptr is the managed object the Rooted handle should represent.
	 */
	Rooted(T *ptr) : Handle<T>(ptr) { addRef(); }

	/**
	 * Constructor of the Rooted class.
	 *
	 * @param h is another Rooted whose managed object should be used.
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
 * The Owned class represents a directed, garbage collected pointer at a managed
 * instance. The lifetime of the represented managed object is guaranteed to be
 * at last as long as the lifetime of the Managed instance which owns this
 * reference.
 */
template <class T>
class Owned : public Handle<T> {
private:
	Managed *owner;

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
	 * @param ptr is a pointer at the managed object the Owned handle should
	 * represent.
	 * @param owner is the managed object which owns this Owned handle instance.
	 * The managed object pointed to in the handle is guaranteed to live at
	 * least as long as the owner.
	 */
	Owned(T *ptr, Managed *owner) : Handle<T>(ptr), owner(owner) { addRef(); }

	/**
	 * Constructor of the Owned class.
	 *
	 * @param h is another Owned whose managed object should be used.
	 * @param owner is the managed object which owns this Owned handle instance.
	 * The managed object pointed to in the handle is guaranteed to live at
	 * least as long as the owner.
	 */
	template <class T2>
	Owned(const Handle<T2> &h, Managed *owner)
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
	Managed *getOwner() const { return owner; }
};

}
}

#endif /* _OUSIA_DOM_MANAGED_HPP_ */

