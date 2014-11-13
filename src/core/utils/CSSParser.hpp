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

#ifndef _OUSIA_UTILS_CSS_PARSER_HPP_
#define _OUSIA_UTILS_CSS_PARSER_HPP_

#include <istream>
#include <map>
#include <vector>
#include <tuple>

#include "BufferedCharReader.hpp"

namespace ousia {
namespace utils {

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

class RuleSet {
private:
	const std::map<std::string, std::string> values;
	const Specificity specificity;

public:
	RuleSet(std::map<std::string, std::string> values, Specificity specificity)
	    : values(values), specificity(specificity)
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
	    : name(name), args(args), generative(generative)
	{
	}

	const std::string &getName() const { return name; }

	const std::vector<std::string> &getArgs() const { return args; }

	const bool &isGenerative() const { return generative; }
};

enum class SelectionOperator { DESCENDANT, DIRECT_DESCENDANT };

// TODO: Subclass of Andreas' Node class
class StyleEdge {
private:
	// TODO: This is wrong! Here we want to have a managed pointer as Andreas
	// mentioned!
	//	const StyleNode target;
	const SelectionOperator selectionOperator;

public:
	StyleEdge(/*StyleNode target,*/ SelectionOperator selectionOperator)
	    : /*target(target),*/ selectionOperator(selectionOperator)
	{
	}

	//	const StyleNode &getTarget() const { return target; }

	const SelectionOperator &getSelectionOperator() const
	{
		return selectionOperator;
	}
};

// TODO: Subclass of Andreas' Node class
class StyleNode {
private:
	const std::string className;
	const PseudoSelector pseudoSelector;
	const std::vector<StyleEdge> edges;
	const std::vector<RuleSet> ruleSets;

public:
	StyleNode(std::string className, PseudoSelector pseudoSelector,
	          std::vector<StyleEdge> edges, std::vector<RuleSet> ruleSets)
	    : className(className),
	      pseudoSelector(pseudoSelector),
	      edges(edges),
	      ruleSets(ruleSets)
	{
	}

	const std::string &getClassName() const { return className; }

	const PseudoSelector &getPseudoSelector() const { return pseudoSelector; }

	const std::vector<StyleEdge> &getEdges() const { return edges; }

	const std::vector<RuleSet> &getRuleSets() const { return ruleSets; }
};

class CSSParser {
public:
	StyleNode parse(BufferedCharReader &input);
};
}
}
#endif
