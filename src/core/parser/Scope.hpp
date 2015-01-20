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

#ifndef _OUSIA_PARSER_SCOPE_H_
#define _OUSIA_PARSER_SCOPE_H_

#include <functional>
#include <list>
#include <vector>

#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Utils.hpp>
#include <core/model/Node.hpp>

/**
 * @file Scope.hpp
 *
 * Contains the Scope class used for resolving references based on the current
 * parser state.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

namespace ousia {
namespace parser {

// Forward declaration
class Scope;

/**
 * Callback function type used for creating a dummy object while no correct
 * object is available for resolution.
 */
using ResolutionImposterCallback = std::function<Rooted<Node>()>;

/**
 * Callback function type called whenever the result of a resolution is
 * available.
 */
using ResolutionResultCallback =
    std::function<void(Handle<Node>, Logger &logger)>;

/**
 * The GuardedScope class takes care of pushing a Node instance into the
 * name resolution stack of a Scope instance and poping this node once the
 * ScopedScope instance is deletes. This way you cannot forget to pop a Node
 * from a Scope instance as this operation is performed automatically.
 */
class GuardedScope {
private:
	/**
	 * Reference at the backing scope instance.
	 */
	Scope *scope;

public:
	/**
	 * Creates a new ScopedScope instance.
	 *
	 * @param scope is the backing Scope instance.
	 * @param node is the Node instance that should be pushed onto the stack of
	 * the Scope instance.
	 */
	GuardedScope(Scope *scope, Handle<Node> node);

	/**
	 * Pops the Node given in the constructor form the stack of the Scope
	 * instance.
	 */
	~GuardedScope();

	/**
	 * Move constructor of the ScopedScope class.
	 */
	GuardedScope(GuardedScope &&);

	// No copy construction
	GuardedScope(const GuardedScope &) = delete;

	/**
	 * Provides access at the underlying Scope instance.
	 */
	Scope *operator->() { return scope; }

	/**
	 * Provides access at the underlying Scope instance.
	 */
	Scope &operator*() { return *scope; }
};

/**
 * Base class for the
 */
class ScopeBase {
protected:
	/**
	 * List containing all nodes currently on the scope, with the newest nodes
	 * being pushed to the back of the list.
	 */
	NodeVector<Node> nodes;

public:
	/**
	 * Default constructor, creates an empty Scope instance.
	 */
	ScopeBase() {}

	/**
	 * Creates a new instance of the ScopeBase class, copying the the given
	 * nodes as initial start value of the node stack. This could for example
	 * be initialized with the path of a node.
	 *
	 * @param nodes is a node vector containing the current node stack.
	 */
	ScopeBase(const NodeVector<Node> &nodes) : nodes(nodes) {}

	/**
	 * Tries to resolve a node for the given type and path for all nodes that
	 * are currently in the stack, starting with the topmost node on the stack.
	 *
	 * @param path is the path for which a node should be resolved.
	 * @param type is the type of the node that should be resolved.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @return a reference at a resolved node or nullptr if no node could be
	 * found.
	 */
	Rooted<Node> resolve(const std::vector<std::string> &path,
	                     const Rtti &type, Logger &logger);
};

/**
 * Class used for representing a deferred resolution. A deferred resolution is
 * triggered whenever an object cannot be resolved, but there may be a chance
 * that it can be resolved in the future. This happens e.g. if a document is
 * just being parsed and the object that is being refered to has not been
 * reached yet.
 */
class DeferredResolution {
private:
	/**
	 * Copy of the scope at the time when the resolution was first triggered.
	 */
	ScopeBase scope;

	/**
	 * Callback function to be called when an element is successfully resolved.
	 */
	ResolutionResultCallback resultCallback;

public:
	/**
	 * Path queried for the resolution.
	 */
	std::vector<std::string> path;

	/**
	 * Reference at the type of the object that should be resolved.
	 */
	const Rtti &type;

	/**
	 * Position at which the resolution was triggered.
	 */
	const SourceLocation location;

	/**
	 * Constructor of the DeferredResolutionScope class. Copies the given
	 * arguments.
	 *
	 * @param nodes is a reference at the current internal node stack of the
	 * Scope class.
	 * @param path is the path that was queried when the resolution failed the
	 * first time.
	 * @param type is the Rtti of the element that should be queried.
	 * @param resultCallback is the callback function that should be called if
	 * the desired element has indeed been found.
	 * @param location is the location at which the resolution was triggered.
	 */
	DeferredResolution(const NodeVector<Node> &nodes,
	                   const std::vector<std::string> &path,
	                   const Rtti &type,
	                   ResolutionResultCallback resultCallback,
	                   const SourceLocation &location = SourceLocation{});

	/**
	 * Performs the actual deferred resolution and calls the resultCallback
	 * callback function in case the resolution is sucessful. In this case
	 * returns true, false otherwise.
	 *
	 * @param logger is the logger instance to which error messages should be
	 * logged.
	 * @return true if the resolution was successful, false otherwise.
	 */
	bool resolve(Logger &logger);
};

/**
 * Provides an interface for document parsers to resolve references based on the
 * current position in the created document tree. The Scope class itself is
 * represented as a chain of Scope objects where each element has a reference to
 * a Node object attached to it. The descend method can be used to add a new
 * scope element to the chain.
 */
class Scope : public ScopeBase {
private:
	/**
	 * List containing all deferred resolution descriptors.
	 */
	std::list<DeferredResolution> deferred;

public:
	/**
	 * Default constructor of the Scope class, creates an empty Scope with no
	 * element on the internal stack.
	 */
	Scope() {}

	/**
	 * Pushes a new node onto the scope.
	 *
	 * @param node is the node that should be used for local lookup.
	 */
	void push(Handle<Node> node);

	/**
	 * Removes the last pushed node from the scope.
	 */
	void pop();

	/**
	 * Returns a ScopedScope instance, which automatically pushes the given node
	 * into the Scope stack and pops it once the ScopedScope is destroyed.
	 */
	GuardedScope descend(Handle<Node> node);

	/**
	 * Returns the top-most Node instance in the Scope hirarchy.
	 *
	 * @return a reference at the root node.
	 */
	Rooted<Node> getRoot() const;

	/**
	 * Returns the bottom-most Node instance in the Scope hirarchy, e.g. the
	 * node that was pushed last onto the stack.
	 *
	 * @return a reference at the leaf node.
	 */
	Rooted<Node> getLeaf();

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * Calls the "imposterCallback" function for obtaining a temporary result if
	 * a node cannot be resolved right now. The "resultCallback" is at most
	 * called twice: Once when this method is called (probably with the
	 * temporary) and another time if the resolution turned out to be successful
	 * at a later point in time.
	 *
	 * @param path is the path for which a node should be resolved.
	 * @param type is the type of the node that should be resolved.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param imposterCallback is the callback function that is called if
	 * the node cannot be resolved at this moment. It gives the caller the
	 * possibility to create an imposter (a temporary object) that may be used
	 * later in the resolution process.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called at least once
	 * either with the imposter (if the resolution was not successful) or the
	 * resolved object directly when this function is called. If the resolution
	 * was not successful the first time, it may be called another time later
	 * in the context of the "performDeferredResolution" function.
	 * @param location is the location in the current source file in which the
	 * resolution was triggered.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolve(const std::vector<std::string> &path, const Rtti &type,
	             Logger &logger, ResolutionImposterCallback imposterCallback,
	             ResolutionResultCallback resultCallback,
	             const SourceLocation &location = SourceLocation{});

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * The "resultCallback" is called when the resolution was successful, which
	 * may be at a later point in time.
	 *
	 * @param path is the path for which a node should be resolved.
	 * @param type is the type of the node that should be resolved.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @param location is the location in the current source file in which the
	 * resolution was triggered.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolve(const std::vector<std::string> &path, const Rtti &type,
	             Logger &logger, ResolutionResultCallback resultCallback,
	             const SourceLocation &location = SourceLocation{});

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * Calls the "imposterCallback" function for obtaining a temporary result if
	 * a node cannot be resolved right now. The "resultCallback" is at most
	 * called twice: Once when this method is called (probably with the
	 * temporary) and another time if the resolution turned out to because
	 * successful at a later point in time.
	 *
	 * @tparam T is the type of the node that should be resolved.
	 * @param path is the path for which a node should be resolved.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param imposterCallback is the callback function that is called if
	 * the node cannot be resolved at this moment. It gives the caller the
	 * possibility to create an imposter (a temporary object) that may be used
	 * later in the resolution process.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called at least once
	 * either with the imposter (if the resolution was not successful) or the
	 * resolved object directly when this function is called. If the resolution
	 * was not successful the first time, it may be called another time later
	 * in the context of the "performDeferredResolution" function.
	 * @param location is the location in the current source file in which the
	 * resolution was triggered.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	template <class T>
	bool resolve(const std::vector<std::string> &path, Logger &logger,
	             std::function<Rooted<T>()> imposterCallback,
	             std::function<void(Handle<T>, Logger &)> resultCallback,
	             const SourceLocation &location = SourceLocation{})
	{
		return resolve(
		    path, typeOf<T>(), logger,
		    [imposterCallback]() -> Rooted<Node> { return imposterCallback(); },
		    [resultCallback](Handle<Node> node, Logger &logger) {
			    resultCallback(node.cast<T>(), logger);
			},
		    location);
	}

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * The "resultCallback" is called when the resolution was successful, which
	 * may be at a later point in time.
	 *
	 * @tparam T is the type of the node that should be resolved.
	 * @param path is the path for which a node should be resolved.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @param location is the location in the current source file in which the
	 * resolution was triggered.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	template <class T>
	bool resolve(const std::vector<std::string> &path, Logger &logger,
	             std::function<void(Handle<T>, Logger &)> resultCallback,
	             const SourceLocation &location = SourceLocation{})
	{
		return resolve(path, typeOf<T>(), logger,
		               [resultCallback](Handle<Node> node, Logger &logger) {
			               resultCallback(node.cast<T>(), logger);
			           },
		               location);
	}

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * Calls the "imposterCallback" function for obtaining a temporary result if
	 * a node cannot be resolved right now. The "resultCallback" is at most
	 * called twice: Once when this method is called (probably with the
	 * temporary) and another time if the resolution turned out to because
	 * successful at a later point in time.
	 *
	 * @tparam T is the type of the node that should be resolved.
	 * @param name is the path for which a node should be resolved. The name is
	 * split at '.' to form a path.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param imposterCallback is the callback function that is called if
	 * the node cannot be resolved at this moment. It gives the caller the
	 * possibility to create an imposter (a temporary object) that may be used
	 * later in the resolution process.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called at least once
	 * either with the imposter (if the resolution was not successful) or the
	 * resolved object directly when this function is called. If the resolution
	 * was not successful the first time, it may be called another time later
	 * in the context of the "performDeferredResolution" function.
	 * @param location is the location in the current source file in which the
	 * resolution was triggered.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	template <class T>
	bool resolve(const std::string &name, Logger &logger,
	             std::function<Rooted<T>()> imposterCallback,
	             std::function<void(Handle<T>, Logger &)> resultCallback,
	             const SourceLocation &location = SourceLocation{})
	{
		return resolve<T>(Utils::split(name, '.'), logger, imposterCallback,
		                  resultCallback, location);
	}

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * The "resultCallback" is called when the resolution was successful, which
	 * may be at a later point in time.
	 *
	 * @tparam T is the type of the node that should be resolved.
	 * @param name is the path for which a node should be resolved. The name is
	 * split at '.' to form a path.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @param location is the location in the current source file in which the
	 * resolution was triggered.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	template <class T>
	bool resolve(const std::string &name, Logger &logger,
	             std::function<void(Handle<T>, Logger &)> resultCallback,
	             const SourceLocation &location = SourceLocation{})
	{
		return resolve<T>(Utils::split(name, '.'), logger, resultCallback,
		                  location);
	}

	/**
	 * Tries to resolve all currently deferred resolution steps. The list of
	 * pending deferred resolutions is cleared after this function has run.
	 *
	 * @param logger is the logger instance into which errors should be logged.
	 */
	bool performDeferredResolution(Logger &logger);
};
}
}

#endif /* _OUSIA_PARSER_SCOPE_H_ */

