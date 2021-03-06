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

#include "Style.hpp"

namespace ousia {
namespace model {

void RuleSet::merge(Rooted<RuleSet> other){
	for(auto& o : other->rules){
		rules[o.first] = o.second;
	}
}

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
	return getChildren(nullptr, &className, &select);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator &op, const PseudoSelector &select)
{
	return getChildren(&op, nullptr, &select);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator &op, const std::string &className)
{
	return getChildren(&op, &className, nullptr);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const SelectionOperator &op)
{
	return getChildren(&op, nullptr, nullptr);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const std::string &className)
{
	return getChildren(nullptr, &className, nullptr);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren(
    const PseudoSelector &select)
{
	return getChildren(nullptr, nullptr, &select);
}

std::vector<Rooted<SelectorNode>> SelectorNode::getChildren()
{
	return getChildren(nullptr, nullptr, nullptr);
}

/*
 * append
 */

std::vector<Rooted<SelectorNode>> SelectorNode::append(
    Handle<SelectorEdge> edge)
{
	std::vector<Rooted<SelectorNode>> out;
	// look if we already have a child in an equivalent edge.
	std::vector<Rooted<SelectorNode>> children =
	    getChildren(edge->getSelectionOperator(), edge->getTarget()->getName(),
	                edge->getTarget()->getPseudoSelector());
	// note that this can only be one child or no child.
	if (children.empty()) {
		// if there is no child the appending process is trivial: We can just
		// add the whole subtree represented by the other node as child here.
		edges.push_back(edge);
	} else {
		// otherwise we start the appending process recursively on the child
		// level.
		// TODO: RuleSet merging
		if (edge->getTarget()->getEdges().empty()) {
			// if there are no more subsequent edges this is a leafe we could
			// not merge, because it is already present in the Tree.
			out.push_back(children[0]);
		} else {
			// otherwise we go into recursion.
			for (auto &e : edge->getTarget()->getEdges()) {
				Rooted<SelectorEdge> e2 {e};
				std::vector<Rooted<SelectorNode>> childLeafs =
				    children[0]->append(e2);
				out.insert(out.end(), childLeafs.begin(), childLeafs.end());
			}
		}
	}
	return out;
}

std::vector<Rooted<SelectorNode>> SelectorNode::append(Handle<SelectorNode> node){
	return append(new SelectorEdge{this->getManager(), node});
}
}
}
