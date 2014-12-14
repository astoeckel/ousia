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

#ifndef _OUSIA_MANAGED_CONTAINER_H_
#define _OUSIA_MANAGED_CONTAINER_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <map>
#include <type_traits>

#include "Managed.hpp"

namespace ousia {

/**
 * Template class which can be used to collect refrences to a certain type of
 * managed objects. Do not use this class directly, use ManagedMap or
 * ManagedVector instead. This class only provides functionality which is common
 * to list and map containers (iterators and state).
 *
 * @param T is the type of the Managed object that should be managed.
 * @param Collection should be a STL container of Owned<T>
 */
template <class T, class Collection>
class ManagedContainer : Managed {
public:
	using collection_type = Collection;
	using value_type = typename collection_type::value_type;
	using reference = typename collection_type::reference;
	using const_reference = typename collection_type::const_reference;
	using iterator = typename collection_type::iterator;
	using const_iterator = typename collection_type::const_iterator;
	using size_type = typename collection_type::size_type;

protected:
	/**
	 * Handle containing a reference to the owner of the collection.
	 */
	Handle<Managed> owner;

	/**
	 * Underlying STL collection.
	 */
	collection_type c;

protected:
	/**
	 * Function which can be overridden by child classes to execute special code
	 * whenever a new element is added to the collection.
	 */
	virtual void addElement(value_type h) {}

	/**
	 * Function which can be overriden by child classes to execute special code
	 * whenever an element is removed from the collection.
	 */
	virtual void deleteElement(value_type h) {}

public:
	/**
	 * Constructor of the ManagedContainer class.
	 *
	 * @param owner is the managed object which owns the collection and all
	 * handles to other managed objects stored within.
	 */
	ManagedContainer(Handle<Managed> owner) : Managed(owner->getManager), owner(owner){};

	/**
	 * Destructor of the ManagedContainer class.
	 */
	virtual ~ManagedContainer() {};

	/* State functions */
	size_type size() const noexcept { return c.size(); }
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

	/* Clear function */
	void clear() noexcept
	{
		for (const_iterator it = cbegin(); it != cend(); it++) {
			deleteElement(*it);
		}
		c.clear();
	}
};

/**
 * Template class which can be used to collect "Owned" refrences to a certain
 * type of managed object. This class should be used in favour of other
 * collections of handles, it takes care of acquiring an owned handle from the
 * owner of this collection whenever a new element is added.
 *
 * @param T is the type of the Managed object that should be managed.
 * @param Collection should be a STL list container of Owned<T>
 */
template <class T, class Collection>
class ManagedGenericList : public ManagedContainer<T, Collection> {
private:
	using Base = ManagedContainer<T, Collection>;

public:
	using Base::ManagedContainer;
	using collection_type = typename Base::collection_type;
	using value_type = typename Base::value_type;
	using reference = typename Base::reference;
	using const_reference = typename Base::const_reference;
	using iterator = typename Base::iterator;
	using const_iterator = typename Base::const_iterator;
	using size_type = typename Base::size_type;

	/**
	 * Initialize with an iterator from another collection.
	 *
	 * @param owner is the managed object which owns the collection and all
	 * handles to other managed objects stored within.
	 * @param first is an iterator pointing at the first element to be copied
	 * from some other collection.
	 * @param last is an iterator pointing at the last element to be copied
	 * from some other collection.
	 */
	template <class InputIterator>
	ManagedGenericList(Handle<Managed> owner, InputIterator first,
	                   InputIterator last)
	    : ManagedContainer<T, Collection>(owner)
	{
		insert(Base::c.begin(), first, last);
	}

	/**
	 * Initialize with another collection.
	 *
	 * @param owner is the managed object which owns the collection and all
	 * handles to other managed objects stored within.
	 * @param in is a reference at some other collection with content that
	 * should be copied.
	 */
	template <class InputCollection>
	ManagedGenericList(Handle<Managed> owner, const InputCollection &in)
	    : ManagedContainer<T, Collection>(owner)
	{
		for (const auto &e : in) {
			push_back(e);
		}
	}

	/* Front and back */
	reference front() { return Base::c.front(); }
	const_reference front() const { return Base::c.front(); }
	reference back() { return Base::c.back(); }
	const_reference back() const { return Base::c.back(); }

	/* Insert and delete operations */

	iterator insert(const_iterator position, Handle<T> h)
	{
		value_type v = Base::owner->acquire(h);
		addElement(v);
		return Base::c.insert(position, v);
	}

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

	iterator find(const Handle<T> h)
	{
		for (iterator it = Base::begin(); it != Base::end(); it++) {
			if (*it == h) {
				return it;
			}
		}
		return Base::end();
	}

	const_iterator find(const Handle<T> h) const
	{
		for (const_iterator it = Base::cbegin(); it != Base::cend(); it++) {
			if (*it == h) {
				return it;
			}
		}
		return Base::cend();
	}

	void push_back(Handle<T> h)
	{
		value_type v = Base::owner->acquire(h);
		this->addElement(v);
		Base::c.push_back(v);
	}

	void pop_back()
	{
		if (!Base::empty()) {
			this->deleteElement(back());
		}
		Base::c.pop_back();
	}

	iterator erase(iterator position)
	{
		this->deleteElement(*position);
		return Base::c.erase(position);
	}

	iterator erase(iterator first, iterator last)
	{
		for (const_iterator it = first; it != last; it++) {
			this->deleteElement(*it);
		}
		return Base::c.erase(first, last);
	}
};

/**
 * Special type of ManagedContainer based on an STL map.
 */
template <class K, class T, class Collection>
class ManagedGenericMap : public ManagedContainer<T, Collection> {
private:
	using Base = ManagedContainer<T, Collection>;

public:
	using Base::ManagedContainer;
	using collection_type = typename Base::collection_type;
	using value_type = typename Base::value_type;
	using key_type = typename Collection::key_type;
	using reference = typename Base::reference;
	using const_reference = typename Base::const_reference;
	using iterator = typename Base::iterator;
	using const_iterator = typename Base::const_iterator;
	using size_type = typename Base::size_type;

private:
	value_type acquirePair(std::pair<K, Handle<T>> val)
	{
		return value_type{val.first, Base::owner->acquire(val.second)};
	}

public:
	/**
	 * Initialize with an iterator from another collection.
	 *
	 * @param owner is the managed object which owns the collection and all
	 * handles to other managed objects stored within.
	 * @param first is an iterator pointing at the first element to be copied
	 * from some other collection.
	 * @param last is an iterator pointing at the last element to be copied
	 * from some other collection.
	 */
	template <class InputIterator>
	ManagedGenericMap(Handle<Managed> owner, InputIterator first,
	                  InputIterator last)
	    : ManagedContainer<T, Collection>(owner)
	{
		insert(first, last);
	}

	/**
	 * Initialize with another collection.
	 *
	 * @param owner is the managed object which owns the collection and all
	 * handles to other managed objects stored within.
	 * @param in is a reference at some other collection with content that
	 * should be copied.
	 */
	template <class InputCollection>
	ManagedGenericMap(Handle<Managed> owner, const InputCollection &in)
	    : ManagedContainer<T, Collection>(owner)
	{
		for (const auto &e : in) {
			insert(*in);
		}
	}

	std::pair<iterator, bool> insert(std::pair<K, Handle<T>> val)
	{
		value_type v = acquirePair(val);
		this->addElement(v);
		return Base::c.insert(v);
	}

	iterator insert(const_iterator position, std::pair<K, Handle<T>> val)
	{
		value_type v = acquirePair(val);
		this->addElement(v);
		return Base::c.insert(position, v);
	}

	template <class InputIterator>
	void insert(InputIterator first, InputIterator last)
	{
		for (auto it = first; it != last; it++) {
			insert(acquirePair);
		}
	}

	iterator erase(const_iterator position)
	{
		this->deleteElement(*position);
		return Base::c.erase(position);
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

	iterator erase(const_iterator first, const_iterator last)
	{
		for (const_iterator it = first; it != last; it++) {
			this->deleteElement(*it);
		}
		return Base::c.erase(first, last);
	}

	iterator find(const key_type &k) { return Base::c.find(k); }
	const_iterator find(const key_type &k) const { return Base::c.find(k); }
};

/**
 * Special type of ManagedGenericList based on an STL vector.
 */
template <class T>
class ManagedVector : public ManagedGenericList<T, std::vector<Owned<T>>> {
public:
	using ManagedGenericList<T, std::vector<Owned<T>>>::ManagedGenericList;
};

/**
 * Special type of ManagedGenericMap based on an STL map.
 */
template <class K, class T>
class ManagedMap : public ManagedGenericMap<K, T, std::map<K, Owned<T>>> {
public:
	using ManagedGenericMap<K, T, std::map<K, Owned<T>>>::ManagedGenericMap;
};
}

#endif /* _OUSIA_MANAGED_CONTAINER_H_ */

