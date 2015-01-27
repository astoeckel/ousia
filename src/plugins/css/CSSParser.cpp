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

#include "CSSParser.hpp"

#include <core/common/VariantReader.hpp>
#include <core/parser/ParserContext.hpp>

namespace ousia {

// CSS code tokens
static const int CURLY_OPEN = 1;
static const int CURLY_CLOSE = 2;
static const int COLON = 3;
static const int DOUBLE_COLON = 4;
static const int SEMICOLON = 5;
static const int HASH = 6;
static const int BRACKET_OPEN = 7;
static const int BRACKET_CLOSE = 8;
static const int PAREN_OPEN = 9;
static const int PAREN_CLOSE = 10;
static const int EQUALS = 11;
static const int ARROW = 12;
static const int COMMA = 13;
// comments
static const int COMMENT = 100;
static const int COMMENT_OPEN = 101;
static const int COMMENT_CLOSE = 102;
// strings
static const int STRING = 200;
static const int DOUBLE_QUOTE = 201;
static const int ESCAPE = 202;
// general syntax
static const int LINEBREAK = 300;

static const TokenTreeNode CSS_ROOT{{{"{", CURLY_OPEN},
                                     {"}", CURLY_CLOSE},
                                     {":", COLON},
                                     {"::", DOUBLE_COLON},
                                     {";", SEMICOLON},
                                     {"#", HASH},
                                     {"[", BRACKET_OPEN},
                                     {"]", BRACKET_CLOSE},
                                     {"(", PAREN_OPEN},
                                     {")", PAREN_CLOSE},
                                     {"=", EQUALS},
                                     {">", ARROW},
                                     {",", COMMA},
                                     {"/*", COMMENT_OPEN},
                                     {"*/", COMMENT_CLOSE},
                                     {"\"", DOUBLE_QUOTE},
                                     {"\\", ESCAPE},
                                     {"\n", LINEBREAK}}};

static const std::map<int, CodeTokenDescriptor> CSS_DESCRIPTORS = {
    {COMMENT_OPEN, {CodeTokenMode::BLOCK_COMMENT_START, COMMENT}},
    {COMMENT_CLOSE, {CodeTokenMode::BLOCK_COMMENT_END, COMMENT}},
    {DOUBLE_QUOTE, {CodeTokenMode::STRING_START_END, STRING}},
    {ESCAPE, {CodeTokenMode::ESCAPE, ESCAPE}},
    {LINEBREAK, {CodeTokenMode::LINEBREAK, LINEBREAK}}};

void CSSParser::doParse(CharReader &reader, ParserContext &ctx)
{
	CodeTokenizer tokenizer{reader, CSS_ROOT, CSS_DESCRIPTORS};
	tokenizer.ignoreComments = true;
	tokenizer.ignoreLinebreaks = true;

	// Create the root node and push it onto the parser scope
	Rooted<model::SelectorNode> root = {
	    new model::SelectorNode{ctx.getManager(), "root"}};
	ctx.getScope().push(root);

	// Parse the document into the root node
	parseDocument(root, tokenizer, ctx);

	// Remove the element from the parser scope
	ctx.getScope().pop();
}

void CSSParser::parseDocument(Rooted<model::SelectorNode> root,
                              CodeTokenizer &tokenizer, ParserContext &ctx)
{
	Token t;
	if (!tokenizer.peek(t)) {
		return;
	}
	tokenizer.resetPeek();
	std::vector<Rooted<model::SelectorNode>> leafList;
	// parse the SelectorTree for this ruleSet.
	parseSelectors(root, tokenizer, leafList, ctx);
	// parse the RuleSet itself.
	Rooted<model::RuleSet> ruleSet = parseRuleSet(tokenizer, ctx);
	for (auto &leaf : leafList) {
		/*
		* every leaf is an accepting node, if one considers the SelectorTree
		* to be a finite state machine. This is relevant, if users do not use
		* the CSS Parser to parse actual Ruleset content but to construct a
		* SelectorTree just to identify a part of the DocumentTree.
		*/
		leaf->setAccepting(true);
		/*
		 * similarly we append the found rules to all leafs.
		 */
		leaf->getRuleSet()->merge(ruleSet);
	}
	parseDocument(root, tokenizer, ctx);
}

void CSSParser::parseSelectors(
    Rooted<model::SelectorNode> root, CodeTokenizer &tokenizer,
    std::vector<Rooted<model::SelectorNode>> &leafList, ParserContext &ctx)
{
	auto tuple = parseSelector(tokenizer, ctx);
	// append the SelectorPath to the root node.
	std::vector<Rooted<model::SelectorNode>> unmergedLeafs =
	    root->append(tuple.first);
	// append the leaf to the leafList.
	switch (unmergedLeafs.size()) {
		case 0:
			// if the leaf could be merged we take the leaf reference from the
			// parseSelector method.
			leafList.push_back(tuple.second);
			break;
		case 1:
			// if the leaf could not be merged we take the existing leaf.
			leafList.push_back(unmergedLeafs[0]);
			break;
		case 2:
			// as the parseSelector is supposed to parse only a SelectorPath
			// there should not be more than one leaf.
			throw LoggableException{
			    "Internal Error: More than one leaf in SelectorPath!",
			    tokenizer.getInput()};
	}
	// if we find a comma, we can proceed parsing selectors.
	Token t;
	if (expect(COMMA, tokenizer, t, false, ctx)) {
		parseSelectors(root, tokenizer, leafList, ctx);
	}
}

std::pair<Rooted<model::SelectorNode>, Rooted<model::SelectorNode>>
CSSParser::parseSelector(CodeTokenizer &tokenizer, ParserContext &ctx)
{
	Rooted<model::SelectorNode> s = parsePrimitiveSelector(tokenizer, ctx);
	Token t;
	if (!tokenizer.peek(t)) {
		// if we are at the end the found selector is the immediate child as
		// well as the leaf.
		return std::make_pair(s, s);
	}
	switch (t.tokenId) {
		case TOKEN_TEXT: {
			// if we find text there is a next token in a DESCENDANT
			// relationship (A B)
			tokenizer.resetPeek();
			// so we parse the rest of the subsequent SelectorPath
			auto tuple = parseSelector(tokenizer, ctx);
			// then we establish the DESCENDANT relationship
			s->getEdges().push_back(new model::SelectorNode::SelectorEdge(
			    ctx.getManager(), tuple.first));
			// and we return this node as well as the leaf.
			return std::make_pair(s, tuple.second);
		}
		case ARROW: {
			tokenizer.consumePeek();
			// if we find an arrow there is a next token in a CHILD
			// relationship (A > B)
			// so we parse the rest of the subsequent SelectorPath
			auto tuple = parseSelector(tokenizer, ctx);
			// then we establish the DESCENDANT relationship
			s->getEdges().push_back(new model::SelectorNode::SelectorEdge(
			    ctx.getManager(), tuple.first,
			    model::SelectionOperator::DIRECT_DESCENDANT));
			// and we return this node as well as the leaf.
			return std::make_pair(s, tuple.second);
		}
		default:
			// everything else is not part of the SelectorPath anymore.
			tokenizer.resetPeek();
			return std::make_pair(s, s);
	}
}

Rooted<model::SelectorNode> CSSParser::parsePrimitiveSelector(
    CodeTokenizer &tokenizer, ParserContext &ctx)
{
	// first and foremost we expect a class name.
	Token t;
	expect(TOKEN_TEXT, tokenizer, t, true, ctx);
	const std::string name = t.content;
	if (!tokenizer.peek(t)) {
		// if we are at the end, we just return this selector with its name.
		Rooted<model::SelectorNode> n{
		    new model::SelectorNode(ctx.getManager(), name)};
		return n;
	}

	bool isGenerative = false;

	switch (t.tokenId) {
		case DOUBLE_COLON:
			// if we find a double colon we have a generative PseudoSelector.
			isGenerative = true;
		// this is supposed to fall through; no missing break.
		case COLON: {
			// if we find a colon we have a restrictive PseudoSelector.
			tokenizer.consumePeek();
			// get the PseudoSelector name.
			expect(TOKEN_TEXT, tokenizer, t, true, ctx);
			const std::string pseudo_select_name = t.content;
			// look for additional arguments.
			if (!expect(PAREN_OPEN, tokenizer, t, false, ctx)) {
				// if we don't have any, we return here.
				Rooted<model::SelectorNode> n{
				    new model::SelectorNode(ctx.getManager(), name,
				                     {pseudo_select_name, isGenerative})};
				return n;
			}
			// parse the argument list.
			Variant::arrayType args;
			// we require at least one argument, if parantheses are used
			// XXX
			args.push_back(VariantReader::parseGeneric(tokenizer.getInput(),
			                                           ctx.getLogger(),
			                                           {',', ')'}).second);
			while (expect(COMMA, tokenizer, t, false, ctx)) {
				// as long as we find commas we expect new arguments.
				args.push_back(VariantReader::parseGeneric(tokenizer.getInput(),
				                                           ctx.getLogger(),
				                                           {',', ')'}).second);
			}
			expect(PAREN_CLOSE, tokenizer, t, true, ctx);
			// and we return with the finished Selector.
			Rooted<model::SelectorNode> n{
			    new model::SelectorNode(ctx.getManager(), name,
			                     {pseudo_select_name, args, isGenerative})};
			return n;
		}
		case HASH: {
			// a hash symbol is syntactic sugar for the PseudoSelector
			// :has_id(id)
			// so we expect an ID now.
			Token t;
			expect(TOKEN_TEXT, tokenizer, t, true, ctx);
			Variant::arrayType args{Variant(t.content.c_str())};
			// and we return the finished Selector
			Rooted<model::SelectorNode> n{new model::SelectorNode(
			    ctx.getManager(), name, {"has_id", args, false})};
			return n;
		}
		case BRACKET_OPEN: {
			// in case of brackets we have one of two restrictive
			// PseudoSelectors
			// has_attribute ([attribute_name])
			// or
			// has_value [attribute_name="value"]
			// in both cases the attribute name comes first.
			Token t;
			expect(TOKEN_TEXT, tokenizer, t, true, ctx);
			Variant::arrayType args{Variant(t.content.c_str())};
			if (!expect(EQUALS, tokenizer, t, false, ctx)) {
				// if no equals sign follows we have a has_attribute
				// PseudoSelector
				// we expect a closing bracket.
				expect(BRACKET_CLOSE, tokenizer, t, true, ctx);
				// and then we can return the result.
				Rooted<model::SelectorNode> n{new model::SelectorNode(
				    ctx.getManager(), name, {"has_attribute", args, false})};
				return n;
			} else {
				// with an equals sign we have a has_value PseudoSelector and
				// expect the value next.
				expect(STRING, tokenizer, t, true, ctx);
				args.push_back(Variant(t.content.c_str()));
				// then we expect a closing bracket.
				expect(BRACKET_CLOSE, tokenizer, t, true, ctx);
				// and then we can return the result.
				Rooted<model::SelectorNode> n{new model::SelectorNode(
				    ctx.getManager(), name, {"has_value", args, false})};
				return n;
			}
		}
		default:
			// everything else is not part of the Selector anymore.
			tokenizer.resetPeek();
			Rooted<model::SelectorNode> n{
			    new model::SelectorNode(ctx.getManager(), name)};
			return n;
	}
}

Rooted<model::RuleSet> CSSParser::parseRuleSet(CodeTokenizer &tokenizer,
                                               ParserContext &ctx)
{
	Rooted<model::RuleSet> ruleSet{new model::RuleSet(ctx.getManager())};
	// if we have no ruleset content, we return an empty ruleset.
	Token t;
	if (!expect(CURLY_OPEN, tokenizer, t, false, ctx)) {
		return ruleSet;
	}
	// otherwise we parse the rules.
	parseRules(tokenizer, ruleSet, ctx);
	// and we expect closing curly braces.
	expect(CURLY_CLOSE, tokenizer, t, true, ctx);
	return ruleSet;
}

void CSSParser::parseRules(CodeTokenizer &tokenizer,
                           Rooted<model::RuleSet> ruleSet, ParserContext &ctx)
{
	std::string key;
	Variant value;
	while (parseRule(tokenizer, ctx, key, value)) {
		ruleSet->getRules().insert({key, value});
	}
}

bool CSSParser::parseRule(CodeTokenizer &tokenizer, ParserContext &ctx,
                          std::string &key, Variant &value)
{
	Token t;
	if (!expect(TOKEN_TEXT, tokenizer, t, false, ctx)) {
		return false;
	}
	// if we find text that is the key first.
	key = t.content;
	// then we expect a :
	expect(COLON, tokenizer, t, true, ctx);
	// then the value
	// TODO: Resolve key for appropriate parsing function here.
	value = VariantReader::parseGeneric(tokenizer.getInput(), ctx.getLogger(),
	                                    {';'}).second;
	// and a ;
	expect(SEMICOLON, tokenizer, t, true, ctx);
	return true;
}

bool CSSParser::expect(int expectedType, CodeTokenizer &tokenizer, Token &t,
                       bool force, ParserContext &ctx)
{
	bool end = !tokenizer.peek(t);
	if (end || t.tokenId != expectedType) {
		if (force) {
			if (end) {
				throw LoggableException{"Unexpected end of file!",
				                        tokenizer.getInput()};
			} else {
				throw LoggableException{"Unexpected token!",
				                        tokenizer.getInput()};
			}
		} else {
			tokenizer.resetPeek();
			return false;
		}
	}
	tokenizer.consumePeek();
	return true;
}
}

