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
 * @file ManagedContainer.hpp
 *
 * Container classes for conviniently storing managed instances.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MANAGED_CONTAINER_H_
#define _OUSIA_MANAGED_CONTAINER_H_

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>
#include <type_traits>

#include "Manager.hpp"
#include "Managed.hpp"

namespace ousia {

template <class T>
class Handle;

/**
 * Default accessor class for accessing the managed element within a list value
 * type.
 *
 * @tparam ValueType is the list value type that should be accessed.
 */
template <class ValueType>
struct ListAccessor {
	Managed *getManaged(const ValueType &val) const { return val.get(); }
};

/**
 * Default accessor class for accessing the managed element within a map value
 * type.
 *
 * @tparam ValueType is the map value type that should be accessed.
 */
template <class ValueType>
struct MapAccessor {
	Managed *getManaged(const ValueType &val) const
	{
		return val.second.get();
	}
};

/**
 * Default implementation of a Listener class. With empty functions.
 */
template <class ValueType>
struct DefaultListener {
	void addElement(const ValueType &val, Managed *owner) {}
	void deleteElement(const ValueType &val, Managed *owner) {}
};

/**
 * Template class which can be used to collect refrences to a certain type of
 * managed objects. Do not use this class directly, use ManagedMap or
 * ManagedVector instead. This class only provides functionality which is common
 * to list and map containers (iterators and state).
 *
 * @tparam T is the type of the Managed object that should be managed.
 * @tparam Collection is the underlying STL collection and should contain
 * pointers of type T as value.
 * @tparam Accessor is a type that allows to resolve the STL value type to the
 * actual, underlying pointer to T -- this is important to generically support
 * maps, where the value type is a pair of key type and the actual value.
 * @tparam Listener is an optional type that allows to execute arbitrary code
 * whenever data is read or written to the collection.
 */
template <class T, class Collection, class Accessor, class Listener>
class ManagedContainer {
public:
	using own_type = ManagedContainer<T, Collection, Accessor, Listener>;
	using value_type = typename Collection::value_type;
	using reference = typename Collection::reference;
	using const_reference = typename Collection::const_reference;
	using iterator = typename Collection::iterator;
	using const_iterator = typename Collection::const_iterator;
	using size_type = typename Collection::size_type;

private:
	/**
	 * Handle containing a reference to the owner of the collection.
	 */
	Managed *owner;

	/**
	 * Accessor used to access the managed instance inside a "reference type".
	 */
	Accessor accessor;

	/**
	 * Listener which is notified whenever an element is added to or removed
	 * from the list.
	 */
	Listener listener;

	/**
	 * Calls the "addElement" function of each element and thus initializes
	 * the references from the owner to the elements.
	 */
	void initialize()
	{
		for (const auto &elem : *this) {
			addElement(elem);
		}
	}

	/**
	 * Calls the "deleteElement" function of each element and thus finalizes
	 * the references from the owner to the elements.
	 */
	void finalize()
	{
		if (owner) {
			for (const auto &elem : *this) {
				deleteElement(elem);
			}
		}
	}

protected:
	/**
	 * Underlying STL collection.
	 */
	Collection c;

	/**
	 * Function to be called whenever an element is added to the collection.
	 * This function makes sure that the association from the owner to the added
	 * element is established.
	 *
	 * @param managed is the managed instance that is being added to the
	 * container.
	 * @param elem is a reference to the actual element that is being added to
	 * the underlying container.
	 */
	void addElement(const value_type &elem)
	{
		Managed* managed = accessor.getManaged(elem);
		Manager &manager = owner ? owner->getManager() : managed->getManager();

		manager.addRef(managed, owner);
		listener.addElement(elem, owner);
	}

	/**
	 * Function to be called whenever an element is removed from the collection.
	 * This function makes sure that the association from the owner to the added
	 * element is removed.
	 *
	 * @param elem is a reference to the actual element that is being removed
	 * from the underlying container.
	 */
	void deleteElement(const value_type &elem)
	{
		Managed* managed = accessor.getManaged(elem);
		Manager &manager = owner ? owner->getManager() : managed->getManager();

		manager.deleteRef(managed, owner);
		listener.deleteElement(elem, owner);
	}

public:

	/**
	 * Constructor of the ManagedContainer class.
	 *
	 * @param owner is the managed object which owns the collection and all
	 * handles to other managed objects stored within.
	 */
	ManagedContainer(Handle<Managed> owner) : owner(owner.get()) {};

	/**
	 * Copy constructor. Creates a copy of the given container with the same
	 * owner as the given container.
	 *
	 * @param other is the other coontainer that should be copied.
	 */
	ManagedContainer(const own_type &other) : owner(other.owner), c(other.c)
	{
		initialize();
	}

	/**
	 * Copy constructor. Creates a copy of the given container with another
	 * owner.
	 *
	 * @param other is the other container that should be copied.
	 */
	ManagedContainer(Handle<Managed> owner, const own_type &other)
	    : owner(owner.get()), c(other.c)
	{
		initialize();
	}

	/**
	 * Copy constructor. Creates a copy of the given container and takes over
	 * ownership.
	 */
	ManagedContainer(Handle<Managed> owner, const Collection &collection)
	    : owner(owner.get()), c(collection)
	{
		initialize();
	}

	/**
	 * Move constructor. Moves the other instance to this instance.
	 *
	 * @param other is the other container that should be moved.
	 */
	ManagedContainer(own_type &&other)
	    : owner(other.owner), c(std::move(other.c))
	{
		other.owner = nullptr;
	}

	/**
	 * Copy constructor. Creates a copy of the given container with another
	 * owner.
	 *
	 * @param other is the other container that should be moved.
	 */
	ManagedContainer(Handle<Managed> owner, own_type &&other)
	    : owner(owner.get()), c(std::move(other.c))
	{
		other.finalize();
		other.owner = nullptr;
		initialize();
	}

	/**
	 * Copy constructor. Initialize with an iterator from another collection.
	 *
	 * @param owner is the managed object which owns the collection and all
	 * handles to other managed objects stored within.
	 * @param first is an iterator pointing at the first element to be copied
	 * from some other collection.
	 * @param last is an iterator pointing at the last element to be copied
	 * from some other collection.
	 */
	template <class InputIterator>
	ManagedContainer(Handle<Managed> owner, InputIterator first,
	                 InputIterator last)
	    : owner(owner.get())
	{
		auto it = end();
		for (auto it2 = first; it2 != last; it2++) {
			it = insert(it, *it2);
			it++;
		}
	}

	/**
	 * Destructor of the ManagedContainer class. Calls the "deleteElement"
	 * function for each element in the container.
	 */
	~ManagedContainer() { finalize(); };

	/**
	 * Copy assignment operator.
	 *
	 * @param other is the collection instance that should be copied.
	 * @return this instance.
	 */
	own_type &operator=(const own_type &other)
	{
		finalize();
		owner = other.owner;
		c = other.c;
		initialize();
		return *this;
	}

	/**
	 * Move assignment operator.
	 *
	 * @param other is the collection instance that should be moved;
	 * @return this instance.
	 */
	own_type &operator=(own_type &&other)
	{
		finalize();
		owner = other.owner;
		c = std::move(other.c);
		other.owner = nullptr;
		return *this;
	}

	/**
	 * Equality operator.
	 */
	bool operator==(const own_type &other)
	{
		return (owner == other.owner) && (c == other.c);
	}

	/**
	 * Inequality operator.
	 */
	bool operator!=(const own_type &other)
	{
		return (owner != other.owner) || (c != other.c);
	}

	/**
	 * Returns the owner of the ManagedContainer instance.
	 */
	Managed *getOwner() const { return owner; }

	/* State functions */

	/**
	 * Returns the size of the container.
	 *
	 * @return the number of elements in the container.
	 */
	size_type size() const noexcept { return c.size(); }

	/**
	 * Returns whether the container is empty.
	 *
	 * @return true if the container is empty, false otherwise.
	 */
	bool empty() const noexcept { return c.empty(); }

	/* Iterators */
	iterator begin() { return c.begin(); }
	iterator end() { return c.end(); }

	iterator rbegin() { return c.rbegin(); }
	iterator rend() { return c.rend(); }

	const_iterator begin() const { return c.cbegin(); }
	const_iterator end() const { return c.cend(); }

	const_iterator cbegin() const { return c.cbegin(); }
	const_iterator cend() const { return c.cend(); }

	const_iterator rbegin() const { return c.crbegin(); }
	const_iterator rend() const { return c.crend(); }

	const_iterator crbegin() const { return c.crbegin(); }
	const_iterator crend() const { return c.crend(); }

	/**
	 * Removes all elements from the container.
	 */
	void clear() noexcept
	{
		finalize();
		c.clear();
	}

	/**
	 * Generic insert operation.
	 */
	iterator insert(iterator position, value_type val)
	{
		addElement(val);
		return c.insert(position, val);
	}

	/**
	 * Erases the element at the given position.
	 *
	 * @param position is the position at which the element should be removed.
	 * @return an iterator to the element after the deleted element.
	 */
	iterator erase(iterator position)
	{
		deleteElement(*position);
		return c.erase(position);
	}

	/**
	 * Erases a range of elements from the collection.
	 *
	 * @param position is the position at which the element should be removed.
	 * @return an iterator to the element after the deleted element.
	 */
	iterator erase(iterator first, iterator last)
	{
		for (const_iterator it = first; it != last; it++) {
			this->deleteElement(*it);
		}
		return c.erase(first, last);
	}
};

/**
 * Template class which can be used to collect "Owned" refrences to a certain
 * type of managed object. This class should be used in favour of other
 * collections of handles, it takes care of acquiring an owned handle from the
 * owner of this collection whenever a new element is added.
 *
 * @param T is the type of the Managed object that should be managed.
 * @param Collection should be a STL list container of T*
 */
template <class T, class Collection, class Accessor, class Listener>
class ManagedGenericList
    : public ManagedContainer<T, Collection, Accessor, Listener> {
private:
	using Base = ManagedContainer<T, Collection, Accessor, Listener>;

public:
	using value_type = typename Base::value_type;
	using reference = typename Base::reference;
	using const_reference = typename Base::const_reference;
	using iterator = typename Base::iterator;
	using const_iterator = typename Base::const_iterator;
	using size_type = typename Base::size_type;

	using Base::ManagedContainer;
	using Base::insert;

	/* Front and back */
	reference front() { return Base::c.front(); }
	const_reference front() const { return Base::c.front(); }
	reference back() { return Base::c.back(); }
	const_reference back() const { return Base::c.back(); }

	/* Insert and delete operations */

	template <class InputIterator>
	iterator insert(const_iterator position, InputIterator first,
	                InputIterator last)
	{
		bool atStart = true;
		const_iterator pos = position;
		for (InputIterator it = first; it != last; it++) {
			if (atStart) {
				atStart = false;
			} else {
				pos++;
			}
			pos = insert(pos, *it);
		}
		return pos;
	}

	iterator find(const value_type &val)
	{
		for (iterator it = Base::begin(); it != Base::end(); it++) {
			if (*it == val) {
				return it;
			}
		}
		return Base::end();
	}

	const_iterator find(const value_type &val) const
	{
		for (const_iterator it = Base::cbegin(); it != Base::cend(); it++) {
			if (*it == val) {
				return it;
			}
		}
		return Base::cend();
	}

	void push_back(const value_type &val)
	{
		this->addElement(val);
		Base::c.push_back(val);
	}

	void pop_back()
	{
		if (!Base::empty()) {
			this->deleteElement(back());
		}
		Base::c.pop_back();
	}
};

/**
 * Special type of ManagedContainer based on an STL map.
 */
template <class K, class T, class Collection, class Accessor, class Listener>
class ManagedGenericMap
    : public ManagedContainer<T, Collection, Accessor, Listener> {
private:
	using Base = ManagedContainer<T, Collection, Accessor, Listener>;

public:
	using value_type = typename Base::value_type;
	using key_type = typename Collection::key_type;
	using reference = typename Base::reference;
	using const_reference = typename Base::const_reference;
	using iterator = typename Base::iterator;
	using const_iterator = typename Base::const_iterator;
	using size_type = typename Base::size_type;

	using Base::ManagedContainer;
	using Base::erase;
	using Base::insert;

	std::pair<iterator, bool> insert(value_type val)
	{
		this->addElement(val);
		return Base::c.insert(val);
	}

	iterator insert(const_iterator position, value_type val)
	{
		this->addElement(val);
		return Base::c.insert(position, val);
	}

	template <class InputIterator>
	void insert(InputIterator first, InputIterator last)
	{
		for (auto it = first; it != last; it++) {
			insert(*it);
		}
	}

	size_t erase(const key_type &k)
	{
		iterator pos = find(k);
		if (pos != Base::end()) {
			erase(pos);
			return 1;
		}
		return 0;
	}

	iterator find(const key_type &k) { return Base::c.find(k); }
	const_iterator find(const key_type &k) const { return Base::c.find(k); }
};

/**
 * Special type of ManagedGenericList based on an STL vector.
 */
template <class T, class Listener = DefaultListener<Handle<T>>>
class ManagedVector
    : public ManagedGenericList<T, std::vector<Handle<T>>,
                                ListAccessor<Handle<T>>, Listener> {
public:
	using Base = ManagedGenericList<T, std::vector<Handle<T>>,
	                                ListAccessor<Handle<T>>, Listener>;
	using Base::ManagedGenericList;
};

/**
 * Special type of ManagedGenericMap based on an STL map.
 */
template <class K, class T,
          class Listener = DefaultListener<std::pair<K, Handle<T>>>>
class ManagedMap
    : public ManagedGenericMap<K, T, std::map<K, Handle<T>>,
                               MapAccessor<std::pair<K, Handle<T>>>, Listener> {
public:
	using Base =
	    ManagedGenericMap<K, T, std::map<K, Handle<T>>,
	                      MapAccessor<std::pair<K, Handle<T>>>, Listener>;
	using Base::ManagedGenericMap;
};
}

#endif /* _OUSIA_MANAGED_CONTAINER_H_ */

