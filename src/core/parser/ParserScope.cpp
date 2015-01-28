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

#include <core/common/Exceptions.hpp>
#include <core/common/Utils.hpp>

#include "ParserScope.hpp"

namespace ousia {

/* Class ParserScopeBase */

ParserScopeBase::ParserScopeBase() {}

ParserScopeBase::ParserScopeBase(const NodeVector<Node> &nodes) : nodes(nodes)
{
}

Rooted<Node> ParserScopeBase::resolve(const std::vector<std::string> &path,
                                      const Rtti &type, Logger &logger)
{
	// Go up the stack and try to resolve the
	for (auto it = nodes.rbegin(); it != nodes.rend(); it++) {
		std::vector<ResolutionResult> res = (*it)->resolve(path, type);

		// Abort if the object could not be resolved
		if (res.empty()) {
			continue;
		}

		// Log an error if the object is not unique
		if (res.size() > 1) {
			logger.error(std::string("The reference \"") +
			             Utils::join(path, ".") + ("\" is ambigous!"));
			logger.note("Referenced objects are:", SourceLocation{},
			            MessageMode::NO_CONTEXT);
			for (const ResolutionResult &r : res) {
				logger.note(Utils::join(r.path(), "."), *(r.node));
			}
		}
		return res[0].node;
	}
	return nullptr;
}

/* Class DeferredResolution */

DeferredResolution::DeferredResolution(const NodeVector<Node> &nodes,
                                       const std::vector<std::string> &path,
                                       const Rtti &type,
                                       ResolutionResultCallback resultCallback,
                                       const SourceLocation &location)
    : scope(nodes),
      resultCallback(resultCallback),
      path(path),
      type(type),
      location(location)
{
}

bool DeferredResolution::resolve(Logger &logger)
{
	Rooted<Node> res = scope.resolve(path, type, logger);
	if (res != nullptr) {
		try {
			resultCallback(res, logger);
		}
		catch (LoggableException ex) {
			logger.log(ex);
		}
		return true;
	}
	return false;
}

/* Class ParserScope */

ParserScope::ParserScope(const NodeVector<Node> &nodes,
                         const std::vector<ParserFlagDescriptor> &flags)
    : ParserScopeBase(nodes), flags(flags), topLevelDepth(nodes.size())
{
}

ParserScope::ParserScope() : topLevelDepth(0) {}

bool ParserScope::checkUnwound(Logger &logger) const
{
	if (nodes.size() != topLevelDepth) {
		logger.error("Not all open elements have been closed!",
		             SourceLocation{}, MessageMode::NO_CONTEXT);
		logger.note("Still open elements are: ", SourceLocation{},
		            MessageMode::NO_CONTEXT);
		for (size_t i = topLevelDepth + 1; i < nodes.size(); i++) {
			logger.note(std::string("Element of interal type ") +
			                nodes[i]->type().name +
			                std::string(" defined here:"),
			            nodes[i]->getLocation());
		}
		return false;
	}
	return true;
}

ParserScope ParserScope::fork() { return ParserScope{nodes, flags}; }

bool ParserScope::join(const ParserScope &fork, Logger &logger)
{
	// Make sure the fork has been unwound
	if (!fork.checkUnwound(logger)) {
		return false;
	}

	// Insert the deferred resolutions of the fork into our own deferred
	// resolution list
	deferred.insert(deferred.end(), fork.deferred.begin(), fork.deferred.end());
	return true;
}

void ParserScope::push(Handle<Node> node)
{
	const size_t currentDepth = nodes.size();
	if (currentDepth == topLevelDepth) {
		topLevelNodes.push_back(node);
	}
	nodes.push_back(node);
}

void ParserScope::pop()
{
	// Make sure pop is not called without an element on the stack
	const size_t currentDepth = nodes.size();
	if (currentDepth == topLevelDepth) {
		throw LoggableException{"No element here to end!"};
	}

	// Remove all flags from the stack that were set for higher stack depths.
	size_t newLen = 0;
	for (ssize_t i = flags.size() - 1; i >= 0; i--) {
		if (flags[i].depth < currentDepth) {
			newLen = i + 1;
			break;
		}
	}
	flags.resize(newLen);

	// Remove the element from the stack
	nodes.pop_back();
}

NodeVector<Node> ParserScope::getTopLevelNodes() const { return topLevelNodes; }

Rooted<Node> ParserScope::getRoot() const { return nodes.front(); }

Rooted<Node> ParserScope::getLeaf() const { return nodes.back(); }

Rooted<Node> ParserScope::select(RttiSet types, int maxDepth)
{
	ssize_t minDepth = 0;
	if (maxDepth >= 0) {
		minDepth = static_cast<ssize_t>(nodes.size()) - (maxDepth + 1);
	}
	for (ssize_t i = nodes.size() - 1; i >= minDepth; i--) {
		if (nodes[i]->type().isOneOf(types)) {
			return nodes[i];
		}
	}
	return nullptr;
}

void ParserScope::setFlag(ParserFlag flag, bool value)
{
	// Fetch the current stack depth
	const size_t currentDepth = nodes.size();

	// Try to change the value of the flag if it was already set on the same
	// stack depth
	for (auto it = flags.rbegin(); it != flags.rend(); it++) {
		if (it->depth == currentDepth) {
			if (it->flag == flag) {
				it->value = value;
				return;
			}
		} else {
			break;
		}
	}

	// Insert a new element into the flags list
	flags.emplace_back(currentDepth, flag, value);
}

bool ParserScope::getFlag(ParserFlag flag)
{
	for (auto it = flags.crbegin(); it != flags.crend(); it++) {
		if (it->flag == flag) {
			return it->value;
		}
	}
	return false;
}

bool ParserScope::resolve(const std::vector<std::string> &path,
                          const Rtti &type, Logger &logger,
                          ResolutionImposterCallback imposterCallback,
                          ResolutionResultCallback resultCallback,
                          const SourceLocation &location)
{
	if (!resolve(path, type, logger, resultCallback, location)) {
		resultCallback(imposterCallback(), logger);
		return false;
	}
	return true;
}

bool ParserScope::resolve(const std::vector<std::string> &path,
                          const Rtti &type, Logger &logger,
                          ResolutionResultCallback resultCallback,
                          const SourceLocation &location)
{
	Rooted<Node> res = ParserScopeBase::resolve(path, type, logger);
	if (res != nullptr) {
		try {
			resultCallback(res, logger);
		}
		catch (LoggableException ex) {
			logger.log(ex, location);
		}
		return true;
	}
	deferred.emplace_back(nodes, path, type, resultCallback, location);
	return false;
}

bool ParserScope::performDeferredResolution(Logger &logger)
{
	// Repeat the resolution process as long as something has changed in the
	// last iteration (resolving a node may cause other nodes to be resolvable).
	while (true) {
		// Iterate over all deferred resolution processes,
		bool hasChange = false;
		for (auto it = deferred.begin(); it != deferred.end();) {
			if (it->resolve(logger)) {
				it = deferred.erase(it);
				hasChange = true;
			} else {
				it++;
			}
		}

		// Abort if nothing has changed in the last iteration
		if (!hasChange) {
			break;
		}
	}

	// We were successful if there are no more deferred resolutions
	if (deferred.empty()) {
		return true;
	}

	// Output error messages for all elements for which resolution did not
	// succeed.
	for (const auto &failed : deferred) {
		logger.error(std::string("Could not resolve ") + failed.type.name +
		                 std::string(" \"") + Utils::join(failed.path, ".") +
		                 std::string("\""),
		             failed.location);
	}
	deferred.clear();
	return false;
}
}

