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

/**
 * @file Style.hpp
 
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */
#ifndef _OUSIA_STYLE_HPP_
#define _OUSIA_STYLE_HPP_

#include <map>
#include <vector>
#include <tuple>

#include <core/common/Variant.hpp>
#include <core/managed/Managed.hpp>
#include <core/model/Node.hpp>

namespace ousia {
namespace model {

/*
 * The Specificity or Precedence of a CSS RuleSet, which decides which
 * rules are applied when different RuleSets contain conflicting information.
 *
 * The Specificity is calculated using the official W3C recommendation
 * http://www.w3.org/TR/CSS2/cascade.html#specificity
 *
 * Note that we do not need to use the integer 'a', since we do not allow
 * local style definitions for single nodes.
 */
struct Specificity {
	int b;
	int c;
	int d;

	Specificity(int b, int c, int d) : b(b), c(c), d(d) {}

	friend bool operator<(const Specificity &x, const Specificity &y)
	{
		return std::tie(x.b, x.c, x.d) < std::tie(y.b, y.c, y.d);
	}

	friend bool operator>(const Specificity &x, const Specificity &y)
	{
		return std::tie(x.b, x.c, x.d) > std::tie(y.b, y.c, y.d);
	}

	friend bool operator==(const Specificity &x, const Specificity &y)
	{
		return std::tie(x.b, x.c, x.d) == std::tie(y.b, y.c, y.d);
	}
};

/**
 * The RuleSet class serves as a container class for key-value
 * pairs. The values are TypeInstances. The proper type is
 * implicitly defined by the keyword.
 */
class RuleSet : public Managed {
private:
	std::map<std::string, Variant> rules;

public:
	/**
	 * Initializes an empty RuleSet.
	 */
	RuleSet(Manager &mgr) : Managed(mgr), rules() {}

	std::map<std::string, Variant> &getRules() { return rules; }

	const std::map<std::string, Variant> &getRules() const
	{
		return rules;
	}

	/**
	 * This implements an overriding "insert all" of all rules in the other
	 * RuleSet to the rules in this RuleSet.
	 */
	void merge(Rooted<RuleSet> other);
};

/**
 * PseudoSelectors are functions that change the behaviour of Selectors.
 * They come in two different flavours:
 * 1.) restricting PseudoSelectors are denoted as :my_selector(arg1,arg2,...)
 *     and are functions returning a boolean value given a node in the
 *     document tree and the additional arguments arg1, arg2, etc.
 *     If the function returns true the selector matches to the given document
 *     node. Otherwise it does not. Note that the #id notation is only
 *     syntactic sugar for the PseudoSelectors :has_id(id). Likewise
 *     the notation [attr] is a shorthand for :has_attribute(attr) and
 *     [attr="value"] is a horthand for :has_value(attr,value).
 * 2.) generative PseudoSelectors are denoted as ::my_selector(arg1,arg2,...)
 *     and are functions returning a document node (probably a newly created
 *     one) referring to the element that shall be styled. An example is the
 *     CSS3 PseudoSelector ::first_letter which creates a new document node
 *     only containing the first letter of the text contained in the input
 *     document node, inserts it into the document tree and returns it to be
 *     styled. This mechanism also implies that generative PseudoSelectors
 *     only make sense at the end of a Selector Path (A B::my_selector C
 *     would not be a well-formed Selector).
 *     TODO: How do we control for this special case?
 *
 * Note that both restrictive and generative PseudoSelectors may be pre-defined
 * and implemented in C++ code as well as user-defined and implemented as
 * JavaScripts. The internal mechanism will resolve the given PseudoSelector
 *name
 * to the according implementation.
 *
 * Also note that the arguments of PseudoSelectors are always given as strings.
 * PseudoSelector implementations have to ensure proper parsing of their inputs
 * themselves.
 */
class PseudoSelector {
private:
	const std::string name;
	const Variant::arrayType args;
	const bool generative;

public:
	PseudoSelector(std::string name, Variant::arrayType args,
	               bool generative)
	    : name(std::move(name)), args(std::move(args)), generative(generative)
	{
	}

	PseudoSelector(std::string name, bool generative)
	    : name(std::move(name)), args(), generative(generative)
	{
	}

	const std::string &getName() const { return name; }

	const Variant::arrayType &getArgs() const { return args; }

	const bool &isGenerative() const { return generative; }
};

inline bool operator==(const PseudoSelector &x, const PseudoSelector &y)
{
	return std::tie(x.getName(), x.getArgs(), x.isGenerative()) ==
	       std::tie(y.getName(), y.getArgs(), y.isGenerative());
}

inline bool operator!=(const PseudoSelector &x, const PseudoSelector &y)
{
	return std::tie(x.getName(), x.getArgs(), x.isGenerative()) !=
	       std::tie(y.getName(), y.getArgs(), y.isGenerative());
}

/**
 * A SelectionOperator for now is just an enumeration class deciding
 * whether a SelectorEdge builds a Descendant relationship or a
 * (direct) child relationship.
 */
enum class SelectionOperator { DESCENDANT, DIRECT_DESCENDANT };

/**
 * This represents a node in the SelectorTree. The SelectorTree makes it
 * possible to efficiently resolve which elements of the documents are selected
 * by a certain selector expression.
 *
 * Assume we have the following CSS specification.
 *
 * A B:p(a,b) { ruleset1 }
 *
 * A { ruleset2 }
 *
 * B::gp(c) { ruleset 3 }
 *
 * where p is a restricting pseudo-selector taking some arguments a and b and
 * gp is a generating pseudo-selector taking some argument c. Both kinds of
 * pseudo selectors result in a function (either C++ hard coded or JavaScript)
 * that either returns a boolean, whether the current node in the document tree
 * fulfils the restricting conditions (take :first_child, for example, which
 * only returns true if the element is in fact the first child of its parent)
 * or, in case of generative pseudo-selectors, returns a new element for the
 * document tree (take ::first-letter for example, which takes the first letter
 * of the text contained in a matching element of the document tree and
 * generates a new node in the document tree containing just this letter such
 * that it is possible to style it differently.
 *
 * The resulting style tree for our example would be
 *
 * A - ruleset 2
 * |_ B:p(a,b) - ruleset 1
 * B::gp(c) - ruleset 3
 *
 * Given the document
 * &lt;A&gt;
 *     &lt;B/&gt;
 *     &lt;B/&gt;
 * &lt;/A&gt;
 *
 * and assuming that the restricting pseudo-selector condition p only applied to
 * the first B we get the following applications of RuleSets:
 *
 * A - ruleset 2
 * first B - ruleset 1 and ruleset 3
 * second B - ruleset 3
 *
 * Furthermore, ruleset 1 has a higher precedence/specificity than ruleset 3.
 * Therefore style rules contained in ruleset 3 will be overridden by
 * contradicting style rules in ruleset 1.
 */
class SelectorNode : public Node {
public:
	/*
	 * A SelectorEdge is a parent-to-child connection in the SelectorTree.
	 * We store edges in the parent. Accordingly SelectorEdges are
	 * defined by their target and the SelectionOperator specifying the
	 * kind of connection.
	 */
	class SelectorEdge : public Managed {
	private:
		Owned<SelectorNode> target;
		const SelectionOperator selectionOperator;

	public:
		SelectorEdge(
		    Manager &mgr, Handle<SelectorNode> target,
		    SelectionOperator selectionOperator = SelectionOperator::DESCENDANT)
		    : Managed(mgr),
		      target(acquire(target)),
		      selectionOperator(selectionOperator)
		{
		}

		Rooted<SelectorNode> getTarget() const { return target; }

		const SelectionOperator &getSelectionOperator() const
		{
			return selectionOperator;
		}
	};

	// Content of the SelectorNode class.
private:
	const PseudoSelector pseudoSelector;
	ManagedVector<SelectorEdge> edges;
	Owned<RuleSet> ruleSet;
	bool accepting = false;

	/**
	 * This is an internal method all getChildren variants refer to.
	 */
	std::vector<Rooted<SelectorNode>> getChildren(const SelectionOperator *op,
	                                              const std::string *className,
	                                              const PseudoSelector *select);

public:
	/**
	 * This initializes an empty SelectorNode with the given name and the
	 * given PseudoSelector.
	 */
	SelectorNode(Manager &mgr, std::string name, PseudoSelector pseudoSelector)
	    : Node(mgr, std::move(name)),
	      pseudoSelector(std::move(pseudoSelector)),
	      edges(this),
	      ruleSet(acquire(new RuleSet(mgr)))
	{
	}

	/**
	 * This initializes an empty SelectorNode with the given name and the
	 * trivial PseudoSelector "true".
	 */
	SelectorNode(Manager &mgr, std::string name)
	    : Node(mgr, std::move(name)),
	      pseudoSelector("true", false),
	      edges(this),
	      ruleSet(acquire(new RuleSet(mgr)))
	{
	}

	const PseudoSelector &getPseudoSelector() const { return pseudoSelector; }

	ManagedVector<SelectorEdge> &getEdges() { return edges; }

	Rooted<RuleSet> getRuleSet() const { return ruleSet; }

	/**
	 * This returns the child of this SelectorNode that is connected by
	 * the given operator, has the given className and the given
	 * PseudoSelector. For convention reasons with the other methods, this
	 * also returns a vector, which might either be empty or has exactly one
	 * element.
	 */
	std::vector<Rooted<SelectorNode>> getChildren(const SelectionOperator &op,
	                                              const std::string &className,
	                                              const PseudoSelector &select);

	/**
	 * This returns all children of this SelectorNode that have the given
	 * className and the given PseudoSelector.
	 */
	std::vector<Rooted<SelectorNode>> getChildren(const std::string &className,
	                                              const PseudoSelector &select);

	/**
	 * This returns all children of this SelectorNode that are connected by the
	 * given SelectionOperator and have the given PseudoSelector.
	 */
	std::vector<Rooted<SelectorNode>> getChildren(const SelectionOperator &op,
	                                              const PseudoSelector &select);

	/**
	 * This returns all children of this SelectorNode that are connected by the
	 * given SelectionOperator and have the given className.
	 */
	std::vector<Rooted<SelectorNode>> getChildren(const SelectionOperator &op,
	                                              const std::string &className);

	/**
	 * This returns all children of this SelectorNode that are connected by the
	 * given SelectionOperator.
	 */
	std::vector<Rooted<SelectorNode>> getChildren(const SelectionOperator &op);

	/**
	 * This returns all children of this SelectorNode that have the given
	 * className.
	 */
	std::vector<Rooted<SelectorNode>> getChildren(const std::string &className);

	/**
	 * This returns all children of this SelectorNode that have the given
	 * PseudoSelector.
	 */
	std::vector<Rooted<SelectorNode>> getChildren(const PseudoSelector &select);

	/**
	 * This returns all children of this SelectorNode.
	 */
	std::vector<Rooted<SelectorNode>> getChildren();

	/**
	 * This appends the given edge and the subsequent SelectorTree to
	 * this SelectorNode. Note that only those nodes get appended to the
	 * SelectorTree that are not already contained in this SelectorTree.
	 *
	 * Consider the example of the following SelectorTree T:
	 *
	 * root
	 * | \
	 * A  B
	 * |
	 * C
	 *
	 * and the following SelectorEdge e with its subsequent Tree T_e
	 *
	 * |
	 * A
	 * |\
	 * C D
	 *
	 * If we call root.append(e) the resulting SelectorTree looks like
	 * this:
	 *
	 * root
	 * | \
	 * A  B
	 * |\
	 * C D
	 *
	 * The method returns all leafs of T that are equivalent to leafs of T_e
	 * and thus could not be appended to T, because they were already contained
	 * there. In our example this would be a vector containing just C.
	 *
	 * @param edge a Rooted reference to an edge that shall be appended to this
	 *             SelectorNode.
	 * @return A list of leafs of this SelectorTree that could not be appended,
	 *         because they were already contained.
	 */
	std::vector<Rooted<SelectorNode>> append(Handle<SelectorEdge> edge);

	/**
	 * This is just a convenience function which creates a new edge
	 * automatically using the DESCENDANT SelectionOperator.
	 */
	std::vector<Rooted<SelectorNode>> append(Handle<SelectorNode> node);

	bool isAccepting() { return accepting; }

	void setAccepting(bool accepting) { this->accepting = accepting; }
};
}
}
#endif
