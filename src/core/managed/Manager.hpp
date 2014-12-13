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
 * @file Manager.hpp
 *
 * Definition of the Manager class used for garbage collection.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MANAGER_HPP_
#define _OUSIA_MANAGER_HPP_

#include <cassert>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

namespace ousia {

// Forward declaration
class Managed;

class Manager {
public:
	/**
	 * Enum used to specify the direction of a object reference (inbound or
	 * outbound).
	 */
	enum class RefDir { IN, OUT };

	/**
	 * The ObjectDescriptor struct is used by the Manager for reference counting
	 * and garbage collection. It describes the reference multigraph with
	 * adjacency lists. Each ObjectDescriptor instance represents a single
	 * managed object and its assocition to and from other managed objects
	 * (nodes in the graph).
	 */
	struct ObjectDescriptor {
	public:
		/**
		 * Contains the number of references to rooted handles. A managed
		 * objects
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
		 * Returns true, if the ObjectDescriptor has at least one input
		 * reference.
		 */
		bool hasInRef() const;

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
	 * Returns the object ObjectDescriptor for the given object from the objects
	 * map.
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
}

#endif /* _OUSIA_MANAGER_HPP_ */

