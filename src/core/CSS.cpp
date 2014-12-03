/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

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

#include "CSS.hpp"

namespace ousia {

/*
 * different versions of "getChildren".
 */

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator *op, const std::string *className,
    const PseudoSelector *select)
{
	std::vector<Rooted<SelectorNode>> out;
	for (auto &e : edges) {
		if (op && e->getSelectionOperator() != *op) {
			continue;
		}
		if (className && e->getTarget()->getName() != *className) {
			continue;
		}
		if (select && e->getTarget()->getPseudoSelector() != *select) {
			continue;
		}
		out.push_back(e->getTarget());
	}
	return out;
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator &op, const std::string &className,
    const PseudoSelector &select)
{
	return getChildren(&op, &className, &select);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const std::string &className, const PseudoSelector &select)
{
	return getChildren(NULL, &className, &select);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator &op, const PseudoSelector &select)
{
	return getChildren(&op, NULL, &select);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator &op, const std::string &className)
{
	return getChildren(&op, &className, NULL);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator &op)
{
	return getChildren(&op, NULL, NULL);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const std::string &className)
{
	return getChildren(NULL, &className, NULL);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const PseudoSelector &select)
{
	return getChildren(NULL, NULL, &select);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren()
{
	return getChildren(NULL, NULL, NULL);
}

/*
 * append
 */

std::vector<Rooted<SelectorNode>> SelectorNode::append(
    Rooted<SelectorEdge> edge)
{
	std::vector<Rooted<SelectorNode>> out;
	// look if we already have a child in an equivalent edge.
	std::vector<Rooted<SelectorNode>> children =
	    getChildren(edge->getSelectionOperator(), edge->getTarget()->getName(),
	                edge->getTarget()->getPseudoSelector());
	// note that this can only be one child or no child.
	if (children.size() == 0) {
		// if there is no child the appending process is trivial: We can just
		// add the whole subtree represented by the other node as child here.
		edges.push_back(edge);
	} else {
		// otherwise we start the appending process recursively on the child
		// level.
		// TODO: RuleSet merging
		if (edge->getTarget()->getEdges().size() == 0) {
			// if there are no more subsequent edges this is a leafe we could
			// not merge, because it is already present in the Tree.
			out.push_back(edge->getTarget());
		} else {
			// otherwise we go into recursion.
			for (auto &e : edge->getTarget()->getEdges()) {
				std::vector<Rooted<SelectorNode>> childLeafs =
				    children[0]->append(e);
				out.insert(out.end(), childLeafs.begin(), childLeafs.end());
			}
		}
	}
	return out;
}
}
