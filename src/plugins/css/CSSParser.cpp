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

namespace ousia {
namespace parser {
namespace css {

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
                                     // linux linebreak
                                     {"\n", LINEBREAK},
                                     // windows linebreak
                                     {"\r\n", LINEBREAK},
                                     // Mac OS linebreak
                                     {"\r", LINEBREAK}}};

static const std::map<int, CodeTokenDescriptor> CSS_DESCRIPTORS = {
    {COMMENT_OPEN, {CodeTokenMode::BLOCK_COMMENT_START, COMMENT}},
    {COMMENT_CLOSE, {CodeTokenMode::BLOCK_COMMENT_END, COMMENT}},
    {DOUBLE_QUOTE, {CodeTokenMode::STRING_START_END, STRING}},
    {ESCAPE, {CodeTokenMode::ESCAPE, ESCAPE}},
    {LINEBREAK, {CodeTokenMode::LINEBREAK, LINEBREAK}}};

Rooted<Node> CSSParser::parse(std::istream &is, ParserContext &ctx)
{
	BufferedCharReader input{is};
	CodeTokenizer tokenizer{input, CSS_ROOT, CSS_DESCRIPTORS};
	tokenizer.ignoreComments = true;
	Rooted<SelectorNode> root = {new SelectorNode{ctx.manager, "root"}};
	parseDocument(root, tokenizer, ctx);
	return root;
}

void CSSParser::parseDocument(Rooted<SelectorNode> root,
                              CodeTokenizer &tokenizer, ParserContext &ctx)
{
	Token t;
	if (!tokenizer.peek(t)) {
		return;
	}
	tokenizer.resetPeek();
	std::vector<Rooted<SelectorNode>> leafList;
	parseSelectors(root, tokenizer, leafList, ctx);
	// TODO: Parse Ruleset
	parseDocument(root, tokenizer, ctx);
}

void CSSParser::parseSelectors(Rooted<SelectorNode> root,
                               CodeTokenizer &tokenizer,
                               std::vector<Rooted<SelectorNode>> &leafList,
                               ParserContext &ctx)
{
	auto tuple = parseSelector(tokenizer, ctx);
	// append the SelectorPath to the root node.
	std::vector<Rooted<SelectorNode>> unmergedLeafs =
	    root->append(std::get<0>(tuple));
	// append the leaf to the leafList.
	switch (unmergedLeafs.size()) {
		case 0:
			// if the leaf could be merged we take the leaf reference from the
			// parseSelector method.
			leafList.push_back(std::get<1>(tuple));
			break;
		case 1:
			// if the leaf could not be merged we take the existing leaf.
			leafList.push_back(unmergedLeafs[0]);
			break;
		case 2:
			// as the parseSelector is supposed to parse only a SelectorPath
			// there should not be more than one leaf.
			throw ParserException{
			    "Internal Error: More than one leaf in SelectorPath!", "",
			    // TODO: Line handling?
			    //			    tokenizer.getInput().getLine(),
			    //			    tokenizer.getInput().getColumn()
			};
	}
	// if we find a comma, we can proceed parsing selectors.
	Token t;
	if (expect(COMMA, tokenizer, t, false, ctx)) {
		parseSelectors(root, tokenizer, leafList, ctx);
	}
}

std::tuple<Rooted<SelectorNode>, Rooted<SelectorNode>> CSSParser::parseSelector(
    CodeTokenizer &tokenizer, ParserContext &ctx)
{
	Rooted<SelectorNode> s = parsePrimitiveSelector(tokenizer, ctx);
	Token t;
	if (!tokenizer.peek(t)) {
		// if we are at the end the found selector is the immediate child as
		// well as the leaf.
		return std::make_tuple(s, s);
	}
	switch (t.tokenId) {
		case TOKEN_TEXT: {
			// if we find text there is a next token in a DESCENDANT
			// relationship (A B)
			tokenizer.resetPeek();
			// so we parse the rest of the subsequent SelectorPath
			auto tuple = parseSelector(tokenizer, ctx);
			// then we establish the DESCENDANT relationship
			s->getEdges().push_back(new SelectorNode::SelectorEdge(
			    ctx.manager, std::get<0>(tuple)));
			// and we return this node as well as the leaf.
			return std::make_tuple(s, std::get<1>(tuple));
		}
		case ARROW: {
			tokenizer.consumePeek();
			// if we find an arrow there is a next token in a CHILD
			// relationship (A > B)
			// so we parse the rest of the subsequent SelectorPath
			auto tuple = parseSelector(tokenizer, ctx);
			// then we establish the DESCENDANT relationship
			s->getEdges().push_back(new SelectorNode::SelectorEdge(
			    ctx.manager, std::get<0>(tuple),
			    SelectionOperator::DIRECT_DESCENDANT));
			// and we return this node as well as the leaf.
			return std::make_tuple(s, std::get<1>(tuple));
		}
		default:
			// everything else is not part of the SelectorPath anymore.
			tokenizer.resetPeek();
			return std::make_tuple(s, s);
	}
}

Rooted<SelectorNode> CSSParser::parsePrimitiveSelector(CodeTokenizer &tokenizer,
                                                       ParserContext &ctx)
{
	// first and foremost we expect a class name.
	Token t;
	expect(TOKEN_TEXT, tokenizer, t, true, ctx);
	const std::string name = t.content;
	if (!tokenizer.peek(t)) {
		// if we are at the end, we just return this selector with its name.
		Rooted<SelectorNode> n{new SelectorNode(ctx.manager, name)};
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
				Rooted<SelectorNode> n{new SelectorNode(
				    ctx.manager, name, {pseudo_select_name, isGenerative})};
				return n;
			}
			// parse the argument list.
			std::vector<std::string> args;
			// we require at least one argument, if parantheses are used
			expect(TOKEN_TEXT, tokenizer, t, true, ctx);
			args.push_back(t.content);
			while (expect(COMMA, tokenizer, t, false, ctx)) {
				// as long as we find commas we expect new arguments.
				expect(TOKEN_TEXT, tokenizer, t, true, ctx);
				args.push_back(t.content);
			}
			expect(PAREN_CLOSE, tokenizer, t, true, ctx);
			// and we return with the finished Selector.
			Rooted<SelectorNode> n{new SelectorNode(
			    ctx.manager, name, {pseudo_select_name, args, isGenerative})};
			return n;
		}
		case HASH: {
			// a hash symbol is syntactic sugar for the PseudoSelector
			// :has_id(id)
			// so we expect an ID now.
			Token t;
			expect(TOKEN_TEXT, tokenizer, t, true, ctx);
			std::vector<std::string> args{t.content};
			// and we return the finished Selector
			Rooted<SelectorNode> n{
			    new SelectorNode(ctx.manager, name, {"has_id", args, false})};
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
			std::vector<std::string> args{t.content};
			if (!expect(EQUALS, tokenizer, t, false, ctx)) {
				// if no equals sign follows we have a has_attribute
				// PseudoSelector
				// we expect a closing bracket.
				expect(BRACKET_CLOSE, tokenizer, t, true, ctx);
				// and then we can return the result.
				Rooted<SelectorNode> n{new SelectorNode(
				    ctx.manager, name, {"has_attribute", args, false})};
				return n;
			} else {
				// with an equals sign we have a has_value PseudoSelector and
				// expect the value next.
				expect(STRING, tokenizer, t, true, ctx);
				args.push_back(t.content);
				// then we expect a closing bracket.
				expect(BRACKET_CLOSE, tokenizer, t, true, ctx);
				// and then we can return the result.
				Rooted<SelectorNode> n{new SelectorNode(
				    ctx.manager, name, {"has_value", args, false})};
				return n;
			}
		}
		default:
			// everything else is not part of the Selector anymore.
			tokenizer.resetPeek();
			Rooted<SelectorNode> n{new SelectorNode(ctx.manager, name)};
			return n;
	}
}

// TODO: Add RuleSet parsing methods.

bool CSSParser::expect(int expectedType, CodeTokenizer &tokenizer, Token &t,
                       bool force, ParserContext &ctx)
{
	bool end = !tokenizer.peek(t);
	if (end || t.tokenId != expectedType) {
		if (force) {
			if (end) {
				throw ParserException{
				    "Unexpected end of file!", "",
				    // TODO: Line handling?
				    //			    tokenizer.getInput().getLine(),
				    //			    tokenizer.getInput().getColumn()
				};
			} else {
				throw ParserException{
				    "Unexpected token!", "",
				    // TODO: Line handling?
				    //			    tokenizer.getInput().getLine(),
				    //			    tokenizer.getInput().getColumn()
				};
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
}
}
