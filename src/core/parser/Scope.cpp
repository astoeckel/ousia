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

#include <core/common/Utils.hpp>

#include "Scope.hpp"

namespace ousia {
namespace parser {

/* Class GuardedScope */

GuardedScope::GuardedScope(Scope *scope, Handle<Node> node) : scope(scope)
{
	scope->push(node);
}

GuardedScope::~GuardedScope()
{
	if (scope) {
		scope->pop();
	}
}

GuardedScope::GuardedScope(GuardedScope &&s)
{
	scope = s.scope;
	s.scope = nullptr;
}

/* Class ScopeBase */

Rooted<Node> ScopeBase::resolve(const std::vector<std::string> &path,
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
			logger.note("Referenced objects are:");
			for (const ResolutionResult &r : res) {
				logger.note(Utils::join(r.path(), "."));
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

/* Class Scope */

void Scope::push(Handle<Node> node) { nodes.push_back(node); }

void Scope::pop() { nodes.pop_back(); }

GuardedScope Scope::descend(Handle<Node> node)
{
	return GuardedScope{this, node};
}

Rooted<Node> Scope::getRoot() const { return nodes.front(); }

Rooted<Node> Scope::getLeaf() { return nodes.back(); }

bool Scope::resolve(const std::vector<std::string> &path, const Rtti &type,
                    Logger &logger, ResolutionImposterCallback imposterCallback,
                    ResolutionResultCallback resultCallback,
                    const SourceLocation &location)
{
	if (!resolve(path, type, logger, resultCallback, location)) {
		resultCallback(imposterCallback(), logger);
		return false;
	}
	return true;
}

bool Scope::resolve(const std::vector<std::string> &path, const Rtti &type,
                    Logger &logger, ResolutionResultCallback resultCallback,
                    const SourceLocation &location)
{
	Rooted<Node> res = ScopeBase::resolve(path, type, logger);
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

bool Scope::performDeferredResolution(Logger &logger)
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
		logger.error(std::string("Could not resolve ")  + failed.type.name + std::string(" \"") +
		                 Utils::join(failed.path, ".") +
		                 std::string("\""),
		             failed.location);
	}
	deferred.clear();
	return false;
}
}
}
