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

#ifndef _OUSIA_CSS_PARSER_HPP_
#define _OUSIA_CSS_PARSER_HPP_

#include <istream>
#include <map>
#include <vector>
#include <tuple>

#include "BufferedCharReader.hpp"
#include "Managed.hpp"
#include "Node.hpp"

namespace ousia {

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
};

bool operator<(const Specificity &x, const Specificity &y)
{
	return std::tie(x.b, x.c, x.d) < std::tie(y.b, y.c, y.d);
}

bool operator>(const Specificity &x, const Specificity &y)
{
	return std::tie(x.b, x.c, x.d) > std::tie(y.b, y.c, y.d);
}

bool operator==(const Specificity &x, const Specificity &y)
{
	return std::tie(x.b, x.c, x.d) == std::tie(y.b, y.c, y.d);
}

class RuleSet : public Managed {
private:
	const std::map<std::string, std::string> values;
	const Specificity specificity;

public:
	RuleSet(Manager &mgr, std::map<std::string, std::string> values,
	        Specificity specificity)
	    : Managed(mgr), values(std::move(values)), specificity(specificity)
	{
	}

	const std::map<std::string, std::string> &getValues() const
	{
		return values;
	}

	const Specificity &getSpecificity() const { return specificity; }
};

class PseudoSelector {
private:
	const std::string name;
	const std::vector<std::string> args;
	const bool generative;

public:
	PseudoSelector(std::string name, std::vector<std::string> args,
	               bool generative)
	    : name(std::move(name)), args(std::move(args)), generative(generative)
	{
	}

	const std::string &getName() const { return name; }

	const std::vector<std::string> &getArgs() const { return args; }

	const bool &isGenerative() const { return generative; }
};

enum class SelectionOperator { DESCENDANT, DIRECT_DESCENDANT };

class StyleNode : public Node {
public:
	class StyleEdge : public Managed {
	private:
		Owned<StyleNode> target;
		const SelectionOperator selectionOperator;

	public:
		StyleEdge(Manager &mgr, Handle<StyleNode> target,
		          SelectionOperator selectionOperator)
		    : Managed(mgr),
		      target(acquire(target)),
		      selectionOperator(selectionOperator)
		{
		}

		Rooted<StyleNode> getTarget() const { return target; }

		const SelectionOperator &getSelectionOperator() const
		{
			return selectionOperator;
		}
	};

private:
	const PseudoSelector pseudoSelector;
	std::vector<Owned<StyleEdge>> edges;
	const std::vector<Owned<RuleSet>> ruleSets;

public:
	StyleNode(Manager &mgr, std::string name,
	          PseudoSelector pseudoSelector,
	          const std::vector<Handle<StyleEdge>> &edges,
	          const std::vector<Handle<RuleSet>> &ruleSets)
	    : Node(mgr, std::move(name)),
	      pseudoSelector(std::move(pseudoSelector)),
	      edges(acquire(edges)),
	      ruleSets(acquire(ruleSets))
	{
	}

	const PseudoSelector &getPseudoSelector() const { return pseudoSelector; }

	const std::vector<Owned<StyleEdge>> &getEdges() const { return edges; }

	const std::vector<Owned<RuleSet>> &getRuleSets() const { return ruleSets; }
};

class CSSParser {

private:

public:
	StyleNode parse(BufferedCharReader &input);
};
}

#endif
