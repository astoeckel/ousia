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
#include <vector>

#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Utils.hpp>
#include <core/model/Node.hpp>

/**
 * @file ParserScope.hpp
 *
 * Contains the ParserScope class used for resolving references based on the
 *current
 * parser state.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

namespace ousia {

// Forward declaration
class CharReader;
class Logger;
class ParserScope;

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
 * Base class for the
 */
class ParserScopeBase {
protected:
	/**
	 * List containing all nodes currently on the scope, with the newest nodes
	 * being pushed to the back of the list.
	 */
	NodeVector<Node> nodes;

public:
	/**
	 * Default constructor, creates an empty ParserScope instance.
	 */
	ParserScopeBase();

	/**
	 * Creates a new instance of the ParserScopeBase class, copying the the
	 *given
	 * nodes as initial start value of the node stack. This could for example
	 * be initialized with the path of a node.
	 *
	 * @param nodes is a node vector containing the current node stack.
	 */
	ParserScopeBase(const NodeVector<Node> &nodes);

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
	Rooted<Node> resolve(const std::vector<std::string> &path, const Rtti &type,
	                     Logger &logger);
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
	 * ParserScope class.
	 * @param path is the path that was queried when the resolution failed the
	 * first time.
	 * @param type is the Rtti of the element that should be queried.
	 * @param resultCallback is the callback function that should be called if
	 * the desired element has indeed been found.
	 * @param location is the location at which the resolution was triggered.
	 */
	DeferredResolution(const NodeVector<Node> &nodes,
	                   const std::vector<std::string> &path, const Rtti &type,
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
 * Enum containing all possible parser flags that can be used by parsers to
 * signal states that cannot be (explicitly or implicitly) stored in the node
 * graph itself.
 */
enum class ParserFlag {
	/**
     * Set to the boolean value "true" if the head section of a file has passed.
     * This happens once the first non-import tag is reached.
     */
	POST_HEAD
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
	NodeVector<Node> topLevelNodes;

	/**
	 * Private constructor used to create a ParserScope fork.
	 */
	ParserScope(const NodeVector<Node> &nodes,
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
	 * Removes the last pushed node from the scope.
	 */
	void pop();

	/**
	 * Returns the top-level nodes. These are the nodes that are pushed onto the
	 * scope instance while the node stack has the depth it had during the
	 * creation of this ParserScope instance.
	 *
	 * @return a node vector containing the top-level nodes.
	 */
	NodeVector<Node> getTopLevelNodes() const;

	/**
	 * Returns the top-most Node instance in the ParserScope hirarchy.
	 *
	 * @return a reference at the root node.
	 */
	Rooted<Node> getRoot() const;

	/**
	 * Returns the bottom-most Node instance in the ParserScope hirarchy, e.g.
	 * the node that was pushed last onto the stack.
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
	 */
	Rooted<Node> select(RttiSet types, int maxDepth = -1);

	/**
	 * Ascends in the stack starting with the leaf node, returns the first node
	 * that matches the given type or nullptr if none matches.
	 *
	 * @tparam T is the type that should be searched in the stack.
	 * @param maxDepth is the maximum number of stack entries the selection
	 * function may ascend. A negative value indicates no limitation.
	 */
	template<class T>
	Rooted<T> select(int maxDepth = -1) {
		return select(RttiSet{&typeOf<T>()}, maxDepth).cast<T>();
	}

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
	 * @param path is the path for which a node should be resolved.
	 * @param type is the type of the node that should be resolved.
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
	 * @param location is the location in the current source file in which
	 * the resolution was triggered.
	 * @return true if the resolution was immediately successful. This does
	 * not mean, that the resolved object does not exist, as it may be resolved
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

#endif /* _OUSIA_PARSER_SCOPE_HPP_ */

