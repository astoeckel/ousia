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

#include <vector>
#include <utility>

#include <core/BufferedCharReader.hpp>
#include <core/CodeTokenizer.hpp>
#include <core/CSS.hpp>
#include <core/parser/Parser.hpp>

namespace ousia {
namespace parser {
namespace css {

/**
 * This is a context free, recursive parser for a subset of the CSS3 language
 * as defined by W3C. We allow the following grammar:
 *
 * DOC              := SELECT RULESET DOC | epsilon
 * SELECTORS        := SELECT , SELECTORS | SELECT
 * SELECT           := SELECT' OPERATOR SELECT | SELECT'
 * SELECT'          := TYPE | TYPE:PSEUDO | TYPE::GEN_PSEUDO |
 *                     TYPE:PSEUDO(ARGUMENTS) |
 *                     TYPE::GEN_PSEUDO(ARGUMENTS) | TYPE#ID |
 *                     TYPE[ATTRIBUTE] | TYPE[ATTRIBUTE=VALUE]
 * TYPE             := string
 * PSEUDO           := string
 * GEN_PSEUDO       := string
 * ARGUMENTS        := string , ARGUMENTS
 * ID               := string
 * ATTRIBUTE        := string
 * VALUE            := string
 * OPERATOR         := epsilon | &gt;
 * RULESET          := epsilon | { RULES }
 * RULES            := RULE RULES | epsilon
 * RULE             := KEY : VALUE ;
 * KEY              := string
 * VALUE            := type-specific parser
 *
 *
 * @author Benjamin Paassen - bpaassen@techfak.uni-bielefeld.de
 */
class CSSParser : public Parser {
private:
	/**
	 * Implements the DOC Nonterminal
	 */
	void parseDocument(Rooted<SelectorNode> root, CodeTokenizer &tokenizer,
	                   ParserContext &ctx);
	/**
	 * Implements the SELECTORS Nonterminal and adds all leaf nodes of the
	 * resulting SelectorTree to the input leafList so that a parsed RuleSet can
	 * be inserted there.
	 */
	void parseSelectors(Rooted<SelectorNode> root, CodeTokenizer &tokenizer,
	                    std::vector<Rooted<SelectorNode>> &leafList,
	                    ParserContext &ctx);
	/**
	 * Implements the SELECT Nonterminal, which in effect parses a SelectorPath
	 * of the SelectorTree and returns the beginning node of the path as first
	 * element as well as the leaf of the path as second tuple element.
	 */
	std::pair<Rooted<SelectorNode>, Rooted<SelectorNode>> parseSelector(
	    CodeTokenizer &tokenizer, ParserContext &ctx);

	/**
	 * Implements the SELECT' Nonterminal, which parses a single Selector with
	 * its PseudoSelector and returns it.
	 */
	Rooted<SelectorNode> parsePrimitiveSelector(CodeTokenizer &tokenizer,
	                                            ParserContext &ctx);

	/**
	 * Implements the RULESET Nonterminal, which parses an entire RuleSet. Note
	 * that we do not require RuleSets to be parsed. It is permitted to just
	 * insert Selector expressions.
	 */
	Rooted<RuleSet> parseRuleSet(CodeTokenizer &tokenizer, ParserContext &ctx);

	/**
	 * Implements the RULES Nonterminal, which parses CSSRules inside a RuleSet.
	 */
	void parseRules(CodeTokenizer &tokenizer, Rooted<RuleSet> ruleSet,
	                ParserContext &ctx);

	/**
	 * Implements the RULE Nonterminal, which parses one single CSSRule. Key
	 * and value are stored in the input references.
	 *
	 * @param key is a (possibly empty) string reference for the key found.
	 * @param value is a (possibly empty) Variant reference for the value found.
	 *
	 * @return true if a rule was found.
	 */
	bool parseRule(CodeTokenizer &tokenizer, ParserContext &ctx,
	               std::string &key, Variant &value);

	/**
	 * A convenience function to wrap around the tokenizer peek() function that
	 * only returns true if an instance of the expected type occurs.
	 *
	 * @param expectedType the ID of the expected type according to the
	 *                     CodeTokenizer specification.
	 * @param tokenizer    the tokenizer for the input.
	 * @param t            an empty token that gets the parsed token content
	 *                     if it has the expected type.
	 * @param force        a flag to be set if it would be fatal for the
	 *                     parsing process to get the wrong type. In that case
	 *                     an exception is thrown.
	 * @return             true iff a token of the expected type was found.
	 */
	bool expect(int expectedType, CodeTokenizer &tokenizer, Token &t,
	            bool force, ParserContext &ctx);

public:
	/**
	 * This parses the given input as CSS content as specified by the grammar
	 * seen above. The return value is a Rooted reference to the root of the
	 * SelectorTree. SelectorTrees are documented in detail in the CSS.hpp
	 * The RuleSet at the respective node at the tree lists all CSS Style
	 * rules that apply.
	 * Note that you are not required to insert CSS code containing actual
	 * rules. You are permitted to just insert a CSS Selector expression
	 * specifying some part of a DocumentTree you want to refer to.
	 *
	 * @param is  is a reference to the input stream that should be parsed.
	 * @param ctx is a reference to the context that should be used while
	 *            parsing the document.
	 * @return    returns the root node of the resulting SelectorTree. For more
	 *            information on the return conventions consult the Parser.hpp.
	 */
	Rooted<Node> parse(std::istream &is, ParserContext &ctx) override;

	/**
	 * As befits a class called CSSParser, this Parser parses CSS.
	 */
	std::set<std::string> mimetypes()
	{
		std::set<std::string> out{"text/css"};
		return out;
	}
};
}
}
}

#endif
