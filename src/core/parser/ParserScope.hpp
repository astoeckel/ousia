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

#ifndef _OUSIA_PARSER_SCOPE_HPP_
#define _OUSIA_PARSER_SCOPE_HPP_

#include <functional>
#include <list>
#include <unordered_set>
#include <vector>

#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Utils.hpp>
#include <core/model/Node.hpp>

/**
 * @file ParserScope.hpp
 *
 * Contains the ParserScope class used for resolving references based on the
 * current parser state.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

namespace ousia {

// Forward declaration
class CharReader;
class Logger;
class ParserScope;
class Type;
class Variant;

/**
 * Callback function type used for creating a dummy object while no correct
 * object is available for resolution.
 */
using ResolutionImposterCallback = std::function<Rooted<Node>()>;

/**
 * Callback function type called whenever the result of a resolution is
 * available.
 *
 * @param resolved is the new, resolved node.
 * @param owner is the node that was passed as "owner".
 * @param logger is the logger to which errors should be logged.
 */
using ResolutionResultCallback = std::function<
    void(Handle<Node> resolved, Handle<Node> owner, Logger &logger)>;

/**
 * Base class for the ParserScope, does not contain the mechanisms for deferred
 * lookup, only maintains the stack of nodes.
 */
class ParserScopeBase {
protected:
	/**
	 * List containing all nodes currently on the scope, with the newest nodes
	 * being pushed to the back of the list.
	 */
	ManagedVector<Node> nodes;

public:
	/**
	 * Default constructor, creates an empty ParserScope instance.
	 */
	ParserScopeBase();

	/**
	 * Creates a new instance of the ParserScopeBase class, copying the the
	 * given nodes as initial start value of the node stack. This could for
	 * example be initialized with the path of a node.
	 *
	 * @param nodes is a node vector containing the current node stack.
	 */
	ParserScopeBase(const ManagedVector<Node> &nodes);

	/**
	 * Tries to resolve a node for the given type and path for all nodes that
	 * are currently in the stack, starting with the topmost node on the stack.
	 *
	 * @param type is the type of the node that should be resolved.
	 * @param path is the path for which a node should be resolved.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @return a reference at a resolved node or nullptr if no node could be
	 * found.
	 */
	Rooted<Node> resolve(const Rtti *type, const std::vector<std::string> &path,
	                     Logger &logger);

	/**
	 * Returns true if the stack is empty.
	 *
	 * @return true if there is no element on the stack.
	 */
	bool isEmpty() const;

	/**
	 * Returns a reference at the internal node stack.
	 *
	 * @return a const reference at the internal node stack.
	 */
	const ManagedVector<Node> &getStack() const;

	/**
	 * Returns a list containing the Rtti type of each Node that is currently
	 * in the stack.
	 *
	 * @return the type signature of the current node stack.
	 */
	std::vector<Rtti const *> getStackTypeSignature() const;

	/**
	 * Returns the top-most Node instance in the ParserScopeBase hirarchy.
	 *
	 * @return a reference at the root node.
	 */
	Rooted<Node> getRoot() const;

	/**
	 * Returns the bottom-most Node instance in the ParserScopeBase hirarchy,
	 * e.g. the node that was pushed last onto the stack.
	 *
	 * @return a reference at the leaf node.
	 */
	Rooted<Node> getLeaf() const;

	/**
	 * Ascends in the stack starting with the leaf node, returns the first node
	 * that matches the type given in the RttiSet or nullptr if none matches.
	 *
	 * @param types is a set of Rtti types for which should be searched in the
	 * stack.
	 * @param maxDepth is the maximum number of stack entries the selection
	 * function may ascend. A negative value indicates no limitation.
	 * @return the matching node or nullptr if the node was not found.
	 */
	Rooted<Node> select(RttiSet types, int maxDepth = -1);

	/**
	 * Ascends in the stack starting with the leaf node, returns the first node
	 * that matches the given type or nullptr if none matches.
	 *
	 * @tparam T is the type that should be searched in the stack.
	 * @param maxDepth is the maximum number of stack entries the selection
	 * function may ascend. A negative value indicates no limitation.
	 * @return the matching node or nullptr if the node was not found.
	 */
	template <class T>
	Rooted<T> select(int maxDepth = -1)
	{
		return select(RttiSet{typeOf<T>()}, maxDepth).cast<T>();
	}

	/**
	 * Ascends in the stack starting with the leaf node, returns the first node
	 * that matches the type given in the RttiSet. Throws an exception if no
	 * node matches.
	 *
	 * @param types is a set of Rtti types for which should be searched in the
	 * stack.
	 * @param maxDepth is the maximum number of stack entries the selection
	 * function may ascend. A negative value indicates no limitation.
	 * @return the matching node.
	 */
	Rooted<Node> selectOrThrow(RttiSet types, int maxDepth = -1);

	/**
	 * Ascends in the stack starting with the leaf node, returns the first node
	 * that matches the given type. Throws an exception if no node matches.
	 *
	 * @tparam T is the type that should be searched in the stack.
	 * @param maxDepth is the maximum number of stack entries the selection
	 * function may ascend. A negative value indicates no limitation.
	 * @return the matching node.
	 */
	template <class T>
	Rooted<T> selectOrThrow(int maxDepth = -1)
	{
		return selectOrThrow(RttiSet{typeOf<T>()}, maxDepth).cast<T>();
	}
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
	ParserScopeBase scope;

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
	const Rtti *type;

	/**
	 * Node for which the resolution is taking place.
	 */
	Rooted<Node> owner;

	/**
	 * Constructor of the DeferredResolutionScope class. Copies the given
	 * arguments.
	 *
	 * @param nodes is a reference at the current internal node stack of the
	 * ParserScope class.
	 * @param path is the path that was queried when the resolution failed the
	 * first time.
	 * @param type is the Rtti of the element that should be queried.
	 * @param resultCallback is the callback function that should be called if
	 * the desired element has indeed been found.
	 * @param owner is the node for which the resolution takes place.
	 */
	DeferredResolution(const ManagedVector<Node> &nodes,
	                   const std::vector<std::string> &path, const Rtti *type,
	                   ResolutionResultCallback resultCallback,
	                   Handle<Node> owner);

	/**
	 * Performs the actual deferred resolution and calls the resultCallback
	 * callback function in case the resolution is sucessful. In this case
	 * returns true, false otherwise.
	 *
	 * @param ignore is a set of nodes that should be ignored if returned as
	 * resolution result as they are
	 * @param logger is the logger instance to which error messages should be
	 * logged.
	 * @return true if the resolution was successful, false otherwise.
	 */
	bool resolve(const std::unordered_multiset<const Node *> &ignore,
	             Logger &logger);

	/**
	 * Inform the callee about the failure by calling the callback function with
	 * "nullptr" as resolved element.
	 *
	 * @param logger is the logger instance to which error messages should be
	 * logged.
	 */
	void fail(Logger &logger);
};

/**
 * Enum containing all possible parser flags that can be used by parsers to
 * signal states that cannot be (explicitly or implicitly) stored in the node
 * graph itself.
 */
enum class ParserFlag {
	/**
     * Set to the boolean value "true" if the head section of a file has passed.
     * This happens once the first non-import tag is reached.
     */
	POST_HEAD,

	/**
	 * Set to the boolean value "true" if explicit fields may no longer be
	 * defined inside a structure element.
	 */
	POST_EXPLICIT_FIELDS,

	/**
	 * Set to true if all user defined tokens have been registered.
	 */
	POST_USER_DEFINED_TOKEN_REGISTRATION
};

/**
 * Provides an interface for document parsers to resolve references based on the
 * current position in the created document tree. The ParserScope class itself
 * is represented as a chain of ParserScope objects where each element has a
 * reference to a Node object attached to it.
 */
class ParserScope : public ParserScopeBase {
public:
	/**
	 * Struct describing a set parser flag.
	 */
	struct ParserFlagDescriptor {
		/**
		 * Stack depth at which the flag has been set.
		 */
		size_t depth;

		/**
		 * Flag that has been set.
		 */
		ParserFlag flag;

		/**
		 * Value of that flag.
		 */
		bool value;

		/**
		 * Default constructor.
		 */
		ParserFlagDescriptor() {}

		/**
		 * Constructor of the parser flag descriptor class.
		 *
		 * @param depth is the depth at which the flag was set.
		 * @param flag is the flag that has been set.
		 * @param value is the value that has been set for that flag.
		 */
		ParserFlagDescriptor(size_t depth, ParserFlag flag, bool value)
		    : depth(depth), flag(flag), value(value)
		{
		}
	};

private:
	/**
	 * List containing all deferred resolution descriptors.
	 */
	std::list<DeferredResolution> deferred;

	/**
	 * Multiset storing the Nodes that are currently awaiting resolution. This
	 * list has the purpose of forcing nodes to be resolved in the correct order
	 * -- first nodes need to be returned as resolution result, that do
	 * themselves not depend on other resolutions. However, if no further
	 * resolutions are possible, this rule is ignored and all resolutions are
	 * performed.
	 */
	std::unordered_multiset<const Node *> awaitingResolution;

	/**
	 * Vector containing all set flags. The vector contains triples of the
	 * depth at which the flag was set, the flag itself and the value.
	 */
	std::vector<ParserFlagDescriptor> flags;

	/**
	 * Depth of the "nodes" list when the ParserScope was created.
	 */
	size_t topLevelDepth;

	/**
	 * List of a all nodes that have been pushed onto the scope at the top level
	 * depth.
	 */
	ManagedVector<Node> topLevelNodes;

	/**
	 * Private constructor used to create a ParserScope fork.
	 */
	ParserScope(const ManagedVector<Node> &nodes,
	            const std::vector<ParserFlagDescriptor> &flags);

public:
	/**
	 * Default constructor of the ParserScope class, creates an empty
	 * ParserScope with no element on the internal stack.
	 */
	ParserScope();

	/**
	 * Makes sure all elements on the scope have been unwound. Loggs an error
	 * message if this is not the case and returns false.
	 *
	 * @param logger is the Logger instance to which information in case of
	 * failure should be written.
	 * @return true if the stack is unwound, false otherwise.
	 */
	bool checkUnwound(Logger &logger) const;

	/**
	 * Returns a new ParserScope instance with a copy of the current node stack
	 * but empty deferred resolutions list and empty topLevelNodes.
	 *
	 * @return a forked ParserScope instance, which starts with a copy of the
	 * node stack.
	 */
	ParserScope fork();

	/**
	 * Joins a previously forked ParserScope instance with this ParserScope.
	 * Copies all pending deferred resolutions from this ParserScope instance.
	 * Joining only works if the node stack of the given ParserScope has the
	 * same depth as the node stack of this ParserScope instance (has been
	 * unwound). This is assured by calling the "checkUnwound" function of
	 * the fork.
	 *
	 * @param fork is the ParserScope fork that should be joined with this
	 * ParserScope instance.
	 * @param logger is the Logger instance to which information in case of
	 * failure should be written.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool join(const ParserScope &fork, Logger &logger);

	/**
	 * Pushes a new node onto the scope.
	 *
	 * @param node is the node that should be used for local lookup.
	 */
	void push(Handle<Node> node);

	/**
	 * Removes the last pushed node from the scope. If the node that is popped
	 * from the internal stack is a RootNode, pending resolutions are performed
	 * and the RootNode is validated.
	 *
	 * @param logger is the Logger instance to which error messages should be
	 * logged.
	 */
	void pop(Logger &logger);

	/**
	 * Returns the top-level nodes. These are the nodes that are pushed onto the
	 * scope instance while the node stack has the depth it had during the
	 * creation of this ParserScope instance.
	 *
	 * @return a node vector containing the top-level nodes.
	 */
	ManagedVector<Node> getTopLevelNodes() const;

	/**
	 * Sets a parser flag for the current stack depth.
	 *
	 * @param flag is the flag that should be set.
	 * @param value is the value to which the flag should be set.
	 */
	void setFlag(ParserFlag flag, bool value);

	/**
	 * Gets the parser flag for the current stack depth, ascends the stack until
	 * a set for this flag is found. Returns false if the flag is not set.
	 *
	 * @param flag is the flag for which the value should be returned.
	 * @return the value that was previously set by setParserFlag or false if no
	 * value has been set.
	 */
	bool getFlag(ParserFlag flag);

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * Calls the "imposterCallback" function for obtaining a temporary
	 * result if a node cannot be resolved right now. The "resultCallback" is
	 * at most called twice: Once when this method is called (probably with the
	 * temporary) and another time if the resolution turned out to be
	 * successful at a later point in time.
	 *
	 * @param type is the type of the node that should be resolved.
	 * @param path is the path for which a node should be resolved.
	 * @param owner is the node for which the resolution takes place.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param imposterCallback is the callback function that is called if
	 * the node cannot be resolved at this moment. It gives the caller the
	 * possibility to create an imposter (a temporary object) that may be
	 * used later in the resolution process.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called at least
	 * once either with the imposter (if the resolution was not successful) or
	 * the resolved object directly when this function is called. If the
	 * resolution was not successful the first time, it may be called another
	 * time later in the context of the "performDeferredResolution" function.
	 * @return true if the resolution was immediately successful. This does
	 * not mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolve(const Rtti *type, const std::vector<std::string> &path,
	             Handle<Node> owner, Logger &logger,
	             ResolutionImposterCallback imposterCallback,
	             ResolutionResultCallback resultCallback);

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * The "resultCallback" is called when the resolution was successful, which
	 * may be at a later point in time.
	 *
	 * @param type is the type of the node that should be resolved.
	 * @param path is the path for which a node should be resolved.
	 * @param owner is the node for which the resolution takes place.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolve(const Rtti *type, const std::vector<std::string> &path,
	             Handle<Node> owner, Logger &logger,
	             ResolutionResultCallback resultCallback);

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
	 * @param owner is the node for which the resolution takes place.
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
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	template <class T>
	bool resolve(const std::vector<std::string> &path, Handle<Node> owner,
	             Logger &logger, ResolutionImposterCallback imposterCallback,
	             ResolutionResultCallback resultCallback)
	{
		return resolve(typeOf<T>(), path, owner, logger, imposterCallback,
		               resultCallback);
	}

	/**
	 * Tries to resolve a node for the given type and path for all nodes
	 * currently on the stack, starting with the topmost node on the stack.
	 * The "resultCallback" is called when the resolution was successful, which
	 * may be at a later point in time.
	 *
	 * @tparam T is the type of the node that should be resolved.
	 * @param path is the path for which a node should be resolved.
	 * @param owner is the node for which the resolution takes place.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	template <class T>
	bool resolve(const std::vector<std::string> &path, Handle<Node> owner,
	             Logger &logger, ResolutionResultCallback resultCallback)
	{
		return resolve(typeOf<T>(), path, owner, logger, resultCallback);
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
	 * @param owner is the node for which the resolution takes place.
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
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	template <class T>
	bool resolve(const std::string &name, Handle<Node> owner, Logger &logger,
	             ResolutionImposterCallback imposterCallback,
	             ResolutionResultCallback resultCallback)
	{
		return resolve<T>(Utils::split(name, '.'), owner, logger,
		                  imposterCallback, resultCallback);
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
	 * @param owner is the node for which the resolution takes place.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	template <class T>
	bool resolve(const std::string &name, Handle<Node> owner, Logger &logger,
	             ResolutionResultCallback resultCallback)
	{
		return resolve<T>(Utils::split(name, '.'), owner, logger,
		                  resultCallback);
	}

	/**
	 * Tries to resolve a node for the given type and path for all nodes that
	 * are currently in the stack, starting with the topmost node on the stack.
	 *
	 * @tparam T is the type of the node that should be resolved.
	 * @param path is the path for which a node should be resolved.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @return a reference at a resolved node or nullptr if no node could be
	 * found.
	 */
	template <class T>
	Rooted<T> resolve(const std::vector<std::string> &path, Logger &logger)
	{
		// TODO: Rooted<Node> res = resolve(typeOf<T>(), path,
		// logger).cast<T>();
		// does not work. Why? Bother stackoverflow with this.
		Rooted<Node> res = ParserScopeBase::resolve(typeOf<T>(), path, logger);
		return res.cast<T>();
	}

	/**
	 * Resolves a typesystem type. Makes sure an array type is returned if an
	 * array type is requested.
	 *
	 * @param path is the path for which a node should be resolved.
	 * @param owner is the node for which the resolution takes place.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolveType(const std::vector<std::string> &path, Handle<Node> owner,
	                 Logger &logger, ResolutionResultCallback resultCallback);

	/**
	 * Resolves a typesystem type. Makes sure an array type is returned if an
	 * array type is requested.
	 *
	 * @param name is the path for which a node should be resolved. The name is
	 * split at '.' to form a path.
	 * @param owner is the node for which the resolution takes place.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolveType(const std::string &name, Handle<Node> owner,
	                 Logger &logger, ResolutionResultCallback resultCallback);

	/**
	 * Build and resolves a (possibly) magic value with the given typesystem
	 * type. This function does not perform any deferred lookups.
	 *
	 * @param data is a reference at a variant that may contain magic values
	 * (even in inner structures). The data will be passed to the "build"
	 * function of the given type.
	 * @param type is the Typesystem type the data should be interpreted with.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @return true if the value was successfully built.
	 */
	bool resolveValue(Variant &data, Handle<Type> type, Logger &logger);

	/**
	 * Resolves a type and makes sure the corresponding value is of the correct
	 * type.
	 *
	 * @param path is the path for which a node should be resolved.
	 * @param owner is the node for which the resolution takes place.
	 * @param value is a reference at the Variant that represents the value for
	 * which the type should be looked up. The value must be valid as long as
	 * the owner node is valid (so it should be a part of the owner).
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolveTypeWithValue(const std::vector<std::string> &path,
	                          Handle<Node> owner, Variant &value,
	                          Logger &logger,
	                          ResolutionResultCallback resultCallback);

	/**
	 * Resolves a type and makes sure the corresponding value is of the correct
	 * type.
	 *
	 * @param name is the path for which a node should be resolved. The name is
	 * split at '.' to form a path.
	 * @param owner is the node for which the resolution takes place.
	 * @param value is a reference at the Variant that represents the value for
	 * which the type should be looked up. The value must be valid as long as
	 * the owner node is valid (so it should be a part of the owner).
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolveTypeWithValue(const std::string &name, Handle<Node> owner,
	                          Variant &value, Logger &logger,
	                          ResolutionResultCallback resultCallback);

	/**
	 * Resolves a FieldDescriptor. Makes sure that the default field can be
	 * handled.
	 *
	 * @param path is the path for which a node should be resolved.
	 * @param owner is the node for which the resolution takes place.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolveFieldDescriptor(const std::vector<std::string> &path,
	                            Handle<Node> owner, Logger &logger,
	                            ResolutionResultCallback resultCallback);

	/**
	 * Resolves a FieldDescriptor. Makes sure that the default field can be
	 * handled.
	 *
	 * @param name is the path for which a node should be resolved. The name is
	 * split at '.' to form a path.
	 * @param owner is the node for which the resolution takes place.
	 * @param logger is the logger instance into which resolution problems
	 * should be logged.
	 * @param resultCallback is the callback function to which the result of
	 * the resolution process is passed. This function is called once the
	 * resolution was successful.
	 * @return true if the resolution was immediately successful. This does not
	 * mean, that the resolved object does not exist, as it may be resolved
	 * later.
	 */
	bool resolveFieldDescriptor(const std::string &name, Handle<Node> owner,
	                            Logger &logger,
	                            ResolutionResultCallback resultCallback);

	/**
	 * Tries to resolve all currently deferred resolution steps. The list of
	 * pending deferred resolutions is cleared after this function has run.
	 *
	 * @param logger is the logger instance into which errors should be logged.
	 * @param postpone if set to true, postpones issuing any error messages and
	 * waits for node resolution.
	 */
	bool performDeferredResolution(Logger &logger, bool postpone = false);
};
}

#endif /* _OUSIA_PARSER_SCOPE_HPP_ */

