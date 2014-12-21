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
 * @file Managed.hpp
 *
 * Describes the garbage collectable Managed class and Handle types pointing at
 * instances of this class.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MANAGED_HPP_
#define _OUSIA_MANAGED_HPP_

#include "Manager.hpp"

namespace ousia {

template <class T>
class Handle;

template <class T>
class Rooted;

template <class T>
class Owned;

template <class T>
class DefaultListener;

class RttiBase;

// TODO: Implement clone, getReferenced and getReferencing

/**
 * The Managed class represents a garbage collected object. Instances of the
 * Managed class are managed (e.g. freed) by an instance of the Manager class.
 * Never free instances of this class yourself (even by playing an instance of
 * this class on the steck). Create any new instance of any managed object with
 * the makeRooted and makeOwned functions.
 *
 * Managed additionally offer the ability to attach arbitrary data to them (with
 * no overhead for objects which do not use this ability). RTTI type information 
 * about the actual Managed object type can be retrieved using the type() and
 * isa() functions. The acquire() function allows to convinently convert an
 * Handle to another object to an Owned instance, owned by this Managed
 * instance.
 */
class Managed {
protected:
	/**
	 * mgr is the reference to the managed object manager which owns this
	 * managed object.
	 */
	Manager &mgr;

public:
	/**
	 * Constructor of the Managed class. Associates the new instance with the
	 * given Manager, which is now in charge for managing this instance. Never
	 * manually free instances of this class (even by using stack instances).
	 * Always use the Rooted and Owned smart pointer classes when refering to
	 * types derived from Managed.
	 *
	 * @param mgr is the Manager which should take ownership of this instance.
	 */
	Managed(Manager &mgr) : mgr(mgr) { mgr.manage(this); };

	/**
	 * Virtual destuctor which may be overwritten by child classes.
	 */
	virtual ~Managed(){};

	/**
	 * Returns a reference ot the manager instance which owns this managed
	 * object.
	 *
	 * @return a reference at the underlying Manager object which manages this
	 * particular Managed instance.
	 */
	Manager &getManager() { return mgr; }

	/**
	 * Acquires a reference to the object wraped in the given handle -- creates
	 * a new Owned handle instance with this Managed instance as owner and the
	 * given object handle as the referenced object.
	 *
	 * @param h is a Handle pointing at the object that should be acquired.
	 * @return a Owned handle with this Managed instance as owner and the given
	 * object as reference.
	 */
	template <class T>
	Owned<T> acquire(const Handle<T> &h)
	{
		return Owned<T>{h, this};
	}

	/**
	 * Acquires a reference to the given pointer to a Managed object -- creates
	 * a new Owned handle instance with this Managed instance as owner and the
	 * given object as the referenced object.
	 *
	 * @param h is a Handle pointing at the object that should be acquired.
	 * @return a Owned handle with this Managed instance as owner and the given
	 * object pointer as reference.
	 */
	template <class T>
	Owned<T> acquire(T *t)
	{
		return Owned<T>{t, this};
	}

	/* Data store methods */

	void storeData(const std::string &key, Handle<Managed> h);

	bool hasDataKey(const std::string &key);

	Rooted<Managed> readData(const std::string &key);

	std::map<std::string, Rooted<Managed>> readData();

	bool deleteData(const std::string &key);

	/* RTTI methods */

	/**
	 * Returns the RttiBase instance registered for instances of the type of
	 * this Managed instance.
	 *
	 * @return a reference to the registered RttiBase for this particular
	 * Managed class.
	 */
	const RttiBase &type() const;

	/**
	 * Returns true if this Managed instance is of the given RttiBase.
	 *
	 * @param true if the RttiBase registered for this particular Managed
	 * class is of the given type or one of the registered parent types is of
	 * the given type.
	 */
	bool isa(const RttiBase &t) const;
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
	template <class T2>
	bool operator==(const Handle<T2> &h) const
	{
		return ptr == h.get();
	}

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

	/**
	 * Statically casts the handle to a handle of the given type.
	 */
	template <class T2>
	Handle<T2> cast()
	{
		return Handle<T2>{static_cast<T2 *>(ptr)};
	}
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

#endif /* _OUSIA_MANAGED_HPP_ */

