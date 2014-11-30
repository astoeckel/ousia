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

#include <core/Node.hpp>

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

class Scope;

/**
 * The ScopedScope class takes care of pushing a Node instance into the
 * name resolution stack of a Scope instance and poping this node once the
 * ScopedScope instance is deletes. This way you cannot forget to pop a Node
 * from a Scope instance as this operation is performed automatically.
 */
class ScopedScope {
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
	 * @param node is the Node instance that should be poped onto the stack of
	 * the Scope instance.
	 */
	ScopedScope(Scope *scope, Handle<Node> node);

	/**
	 * Pops the Node given in the constructor form the stack of the Scope
	 * instance.
	 */
	~ScopedScope();

	/**
	 * Copying a ScopedScope is invalid.
	 */
	ScopedScope(const ScopedScope &) = delete;

	/**
	 * Move constructor of the ScopedScope class.
	 */
	ScopedScope(ScopedScope &&);

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
 * Provides an interface for document parsers to resolve references based on the
 * current position in the created document tree. The Scope class itself is
 * represented as a chain of Scope objects where each element has a reference to
 * a Node object attached to it. The descend method can be used to add a new
 * scope element to the chain.
 */
class Scope {
private:
	std::deque<Rooted<Node>> nodes;

public:
	/**
	 * Constructor of the Scope class.
	 *
	 * @param rootNode is the top-most Node from which elements can be looked
	 * up.
	 */
	Scope(Handle<Node> rootNode) { nodes.push_back(rootNode); }

	/**
	 * Returns a reference at the Manager instance all nodes belong to.
	 */
	Manager &getManager() { return getRoot()->getManager(); }

	/**
	 * Pushes a new node onto the scope.
	 *
	 * @param node is the node that should be used for local lookup.
	 */
	void push(Handle<Node> node) { nodes.push_back(node); }

	/**
	 * Removes the last pushed node from the scope.
	 */
	void pop() { nodes.pop_back(); }

	/**
	 * Returns a ScopedScope instance, which automatically pushes the given node
	 * into the Scope stack and pops it once the ScopedScope is destroyed.
	 */
	ScopedScope descend(Handle<Node> node) { return ScopedScope{this, node}; }

	/**
	 * Returns the top-most Node instance in the Scope hirarchy.
	 *
	 * @return a reference at the root node.
	 */
	Rooted<Node> getRoot() { return nodes.front(); }

	/**
	 * Returns the bottom-most Node instance in the Scope hirarchy, e.g. the
	 * node that was pushed last onto the stack.
	 *
	 * @return a reference at the leaf node.
	 */
	Rooted<Node> getLeaf() { return nodes.back(); }
};

/* Class ScopedScope -- inline declaration of some methods */

inline ScopedScope::ScopedScope(Scope *scope, Handle<Node> node) : scope(scope)
{
	scope->push(node);
}

inline ScopedScope::~ScopedScope()
{
	if (scope) {
		scope->pop();
	}
}

inline ScopedScope::ScopedScope(ScopedScope &&s)
{
	scope = s.scope;
	s.scope = nullptr;
}
}
}

#endif /* _OUSIA_PARSER_SCOPE_H_ */

