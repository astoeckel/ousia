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

#ifndef _OUSIA_NODE_HPP_
#define _OUSIA_NODE_HPP_

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <unordered_set>

#include <core/managed/Managed.hpp>
#include <core/managed/ManagedContainer.hpp>

namespace ousia {

/* Forward declarations */
class Node;
class Event;

/**
 * EventType is an enum containing all possible node events. New event types
 * should be added here.
 */
enum class EventType : int {
	/**
     * Generic update event which may be triggered if some important property
     * of the node is changed.
     */
	UPDATE,

	/**
     * The NAME_CHANGE event is used to inform listeners that the name of the
     * node has changed.
     */
	NAME_CHANGE,

	/**
     * The ADD_CHILD event is used to inform listeners that the node got a new
     * child in any of its child node lists.
     */
	ADD_CHILD,

	/**
     * The DELETE_CHILD event is used to inform listeners that the node got a
     * new child in any of its child node lists.
     */
	DELETE_CHILD
};

/**
 * Definition of the EventHandler function.
 *
 * @param event is a reference to the object holding the event data.
 * @param owner is a reference to the managed object that was given in the
 * registerEventHandler function.
 */
using EventHandler = void (*)(const Event &event, Handle<Managed> owner);

/**
 * The Event class and its child classes are responsible for containing the
 * actual event data which further describes the event to the event handlers.
 * Instances of this class and its children must be declared on the stack or as
 * a temporary.
 */
class Event {
private:
	/**
	 * True as long as the event can bubble up the node hirarchy.
	 */
	mutable bool bubble;

public:
	/**
	 * Contains the actual event type of this class.
	 */
	const EventType type;

	/**
	 * Node on which the event was triggered.
	 */
	Rooted<Node> sender;

	/**
	 * Constructor of the Event class.
	 *
	 * @param type is an element from the EventType enum.
	 * @param bubble if set to true, the event can bubble up the node hirarchy.
	 */
	Event(EventType type, bool bubble = true) : bubble(bubble), type(type){};

	/**
	 * Delete the copy constructor.
	 */
	Event(const Event &) = delete;

	/**
	 * Delete the assignment operator.
	 */
	Event &operator=(const Event &) = delete;

	/**
	 * Stops the propagation of this event to the parent element.
	 */
	void stopPropagation() const { bubble = false; }

	/**
	 * Returns true if the event can still bubble.
	 */
	bool canBubble() const { return bubble; }
};

/**
 * Event used when the name of a node has changed.
 */
class NameChangeEvent : public Event {
public:
	/**
	 * Reference to a string containing the old name of the node.
	 */
	const std::string &oldName;

	/**
	 * Reference to a string containing the new name of the node.
	 */
	const std::string &newName;

	/**
	 * Constructor of the NameChangeEvent class.
	 *
	 * @param oldName is a reference to a string containing the old name of the
	 * node.
	 * @param newName is a reference to a string containing the new name of the
	 * node.
	 * @param bubble if set to true, the event can bubble up the node hirarchy.
	 */
	NameChangeEvent(const std::string &oldName, const std::string &newName,
	                bool bubble = true)
	    : Event(EventType::NAME_CHANGE, bubble),
	      oldName(oldName),
	      newName(newName)
	{
	}
};

/**
 * Struct containing the data which describes a single registered event handler.
 * Note that the event type (e.g. which type of event this element was
 * registered for) is stored outside the EventHandlerDescriptor (in the map
 * storing the registered event handlers).
 */
struct EventHandlerDescriptor {
	/**
	 * Unique id of the event handler.
	 */
	const int id;

	/**
	 * Reference to the event handler containing the events.
	 */
	const EventHandler handler;

	/**
	 * Reference to the managed element which owns the event handler. The object
	 * which owns the Owned handler is given in the constructor.
	 */
	const Owned<Managed> owner;

	/**
	 * Set to true, if this event handler listens to bubbled events comming from
	 * child nodes.
	 */
	const bool includeChildren;

	/**
	 * Constructor of the EventHandlerDescriptor struct.
	 *
	 * @param id is the node-unique id of the EventHandlerDescriptor.
	 * @param handler is the function pointer which is going to be called once
	 * the associated event handler has fired.
	 * @param owner is a user-specified object which owns the method that is
	 * going to be called. This can be used to make sure that the method which
	 * handles the events has access to its owned object as long as the event
	 * handler lives.
	 * @param parent is the parent element this descriptor belongs to. The
	 * a handle to the "owner" object will be created on behalf of the parent.
	 * @param includeChildren is set to true if the event handler should handle
	 * events comming from child elements.
	 */
	EventHandlerDescriptor(int id, EventHandler handler, Handle<Managed> owner,
	                       Managed *parent, bool includeChildren)
	    : id(id),
	      handler(handler),
	      owner(owner, parent),
	      includeChildren(includeChildren)
	{
	}
};

/**
 * The Node class builds the base class for any Node within the DOM graph. A
 * node may either be a descriptive node (such as a domain description etc.)
 * or a document element. Each node is identified by acharacteristic name and
 * a parent element. Note that the node name is not required to be unique. Nodes
 * without parent are considered root nodes.
 */
class Node : public Managed {
public:
	/**
	 * The Filter function is used when resolving names to Node instances. The
	 * filter tests whether the given node meets the requirements for inclusion
	 * in the result list.
	 *
	 * @param managed is the managed which should be tested.
	 * @param data is user-defined data passed to the filter.
	 * @return true if the node should be included in the result set, false
	 * otherwise.
	 */
	using Filter = bool (*)(Handle<Managed> managed, void *data);

	/**
	 * Hash functional used to convert pairs of nodes and int to hashes which
	 * can be used within a unordered_set.
	 */
	struct VisitorHash {
		size_t operator()(const std::pair<const Node *, int> &p) const
		{
			const std::hash<const Node *> nodeHash;
			const std::hash<int> intHash;
			return nodeHash(p.first) + 37 * intHash(p.second);
		}
	};

	/**
	 * Alias for the VisitorSet class which represents all nodes which have been
	 * visited in the name resolution process. The map stores pairs of node
	 * pointers and integers, indicating for which path start id the node has
	 * already been visited.
	 */
	using VisitorSet =
	    std::unordered_set<std::pair<const Node *, int>, VisitorHash>;

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
	 * Current id counter. The id counter may be used to create ids which are
	 * unique inside the realm of this manager instance.
	 */
	int handlerIdCounter = 0;

	/**
	 * Multimap containing all registered event handlers for this node.
	 */
	std::multimap<EventType, EventHandlerDescriptor> handlers;

	/**
	 * Private version of the "path" function used to construct the path. Calls
	 * the path function of the parent node and adds the own name to the given
	 * vector.
	 *
	 * @param p is the list the path should be constructed in.
	 */
	void path(std::vector<std::string> &p) const;

protected:
	/**
	 * Function which should be overwritten by derived classes in order to
	 * resolve node names to a list of possible nodes. The implementations of
	 * this function do not need to do anything but call the "resovle" function
	 * of any child instance of NamedNode.
	 *
	 * @param res is the result list containing all possible nodes matching the
	 * name specified in the path.
	 * @param path is a list specifying a path of node names which is meant to
	 * specify a certain named node.
	 * @param idx is the current index in the path.
	 * @param visited is a map which is used to prevent unwanted recursion.
	 * @param filter is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The filter function can be used to restrict the
	 * type of matched functions.
	 * @param filterData is user-defined data that should be passed to the
	 * filter.
	 */
	virtual void doResolve(std::vector<Rooted<Managed>> &res,
	                       const std::vector<std::string> &path, Filter filter,
	                       void *filterData, unsigned idx, VisitorSet &visited);

public:
	/**
	 * Initializes the node with empty name and parent.
	 *
	 * @param mgr is a reference to the Manager instace the node belongs to.
	 */
	Node(Manager &mgr, Handle<Node> parent = nullptr)
	    : Managed(mgr), parent(acquire(parent))
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
	    : Managed(mgr), name(name), parent(acquire(parent))
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
	std::string getName() const { return name; }

	/**
	 * Specifies whether the node has a name, e.g. whether the current name is
	 * not empty.
	 *
	 * @return true if the name of this node is not empty, false otherwise.
	 */
	bool hasName() const { return !name.empty(); }

	/**
	 * Sets the parent node.
	 *
	 * @param parent is a Handle to the parent node.
	 */
	void setParent(Handle<Node> parent) { this->parent = acquire(parent); }

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
	 * @return a vector containing the path (starting with the root node) to
	 * this node as a list of names.
	 */
	std::vector<std::string> path() const;

	/**
	 * Function which resolves a name path to a list of possible nodes.
	 *
	 * @param res is the result list containing all possible nodes matching the
	 * name specified in the path.
	 * @param path is a list specifying a path of node names which is meant to
	 * specify a certain named node.
	 * @param filter is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The filter function can be used to restrict the
	 * type of matched functions.
	 * @param filterData is user-defined data that should be passed to the
	 * filter.
	 * @param idx is the current index in the path.
	 * @param visited is a map which is used to prevent unwanted recursion.
	 * @param alias is a pointer at a string which contains an alternative name
	 * for this node. If nullptr is given, not such alternative name is
	 * provided.
	 * @return the number of elements in the result list.
	 */
	int resolve(std::vector<Rooted<Managed>> &res,
	            const std::vector<std::string> &path, Filter filter,
	            void *filterData, unsigned idx, VisitorSet &visited,
	            const std::string *alias);

	/**
	 * Function which resolves a name path to a list of possible nodes starting
	 * from this node.
	 *
	 * @param path is a list specifying a path of node names meant to specify a
	 * certain named node.
	 * @param filter is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The filter function can e.g. be used to restrict
	 * the type of matched functions.
	 * @param filterData is user-defined data that should be passed to the
	 * filter.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Managed>> resolve(const std::vector<std::string> &path,
	                                  Filter filter, void *filterData);

	/**
	 * Function which resolves a name path to a list of possible nodes starting
	 * from this node.
	 *
	 * @param path is a list specifying a path of node names meant to specify a
	 * certain named node.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Managed>> resolve(const std::vector<std::string> &path)
	{
		return resolve(path, nullptr, nullptr);
	}

	/**
	 * Function which resolves a single name to a list of possible nodes
	 * starting from this node.
	 *
	 * @param name is the name which should be resolved.
	 * @param filter is a callback function which may check whether a certain
	 * node should be in the result set. If nullptr is given, all nodes matching
	 * the path are included. The filter function can e.g. be used to restrict
	 * the type of matched functions.
	 * @param filterData is user-defined data that should be passed to the
	 * filter.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Managed>> resolve(const char *, Filter filter,
	                                  void *filterData)
	{
		return resolve(std::vector<std::string>{name}, filter, filterData);
	}

	/**
	 * Function which resolves a single name to a list of possible nodes
	 * starting from this node.
	 *
	 * @param name is the name which should be resolved.
	 * @return a vector containing all found node references.
	 */
	std::vector<Rooted<Managed>> resolve(const std::string &name)
	{
		return resolve(std::vector<std::string>{name}, nullptr, nullptr);
	}

	/**
	 * Registers a new event handler for listening to the given event type.
	 *
	 * @param type is the event type the handler should listen to.
	 * @param handler is the handler that should be called.
	 * @param owner is an object the handler belongs to. May be nullptr.
	 * @param includeChildren if set to true, the event handler is also called
	 * if the same event is triggered on one of the child nodes.
	 * @return a unique event handler.
	 */
	int registerEventHandler(EventType type, EventHandler handler,
	                         Handle<Managed> owner = nullptr,
	                         bool includeChildren = false);

	/**
	 * Unregisters the given event handler from the node. Note that removing an
	 * event handler has linear time.
	 *
	 * @param id is the unique event handler id.
	 * @return true if the given event handler was successfully unregistered.
	 */
	bool unregisterEventHandler(int id);

	/**
	 * Triggers an event on this node.
	 *
	 * @param event is a pointer at the event that should be triggered. The
	 * calling function has ownership over the given event.
	 * @param fromChild is set to true if the triggerEvent function is called
	 * from a child node.
	 * @return true if any event handler was found.
	 */
	bool triggerEvent(Event &event, bool fromChild = false);
};

// TODO: Use a different listener here for updating name maps

template <class T, class Listener = DefaultListener<Handle<T>>>
class NodeVector
    : public ManagedGenericList<T, std::vector<Handle<T>>,
                                ListAccessor<Handle<T>>, Listener> {
public:
	using Base = ManagedGenericList<T, std::vector<Handle<T>>,
	                                ListAccessor<Handle<T>>, Listener>;
	using Base::ManagedGenericList;
};

template <class K, class T,
          class Listener = DefaultListener<std::pair<K, Handle<T>>>>
class NodeMap
    : public ManagedGenericMap<K, T, std::map<K, Handle<T>>,
                               MapAccessor<std::pair<K, Handle<T>>>, Listener> {
public:
	using Base =
	    ManagedGenericMap<K, T, std::map<K, Handle<T>>,
	                      MapAccessor<std::pair<K, Handle<T>>>, Listener>;
	using Base::ManagedGenericMap;
};


}

#endif /* _OUSIA_NODE_HPP_ */

