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

#ifndef _OUSIA_TOKENIZER_HPP_
#define _OUSIA_TOKENIZER_HPP_

#include <istream>
#include <map>
#include <deque>

#include "BufferedCharReader.hpp"

namespace ousia {

/**
 * This exception is currently only thrown if errors are made during the
 * initialization of the Tokenizer. Have a closer look at the documentation
 * of the TokenTreeNode constructor for more information.
 */
class TokenizerException : public std::exception {
public:
	const std::string msg;

	TokenizerException(const std::string &msg) : msg(msg){};

	virtual const char *what() const noexcept override { return msg.c_str(); }
};

/**
 * The Tokenizer internally uses a TokenTree to be efficiently able to identify
 * the longest consecutive token in the text. This is equivalent to a prefix
 * trie.
 *
 * The TokenTree is a construct that structures all special tokens this
 * Tokenizer recognizes. Consider the Tokens "aab", "a" and "aac". Then
 * the TokenTree would look like this:
 *
 * a
 * | \
 * a $
 * | \
 * b c
 * | |
 * $ $
 *
 * Every node in the TokenTree is a valid end state that has a $ attached to it.
 * During the search algorithm the Tokenizer goes through the tree and stores
 * the last valid position. If a character follows that does not lead to a new
 * node in the TokenTree the search ends (and starts again at this character).
 * The token corresponding to the last valid position is returned.
 *
 * This allows us to uniquely identify the matching token given a certain
 * input text. Note that this is a greedy matching approach that does not
 * work if you're using truly ambiguous tokens (that have the same text).
 *
 * It is also not allowed that tokens have common middle parts but varying
 * pre- and suffixes. Consider the example of two tokens "abd" and "bc" and
 * the input string "abc". In that case we start looking for "abd" at the
 * start, won't find it, wenn we hit "c" and start the scanning process
 * anew. Thus the "bc" token is not found.
 *
 * For most (well-behaved) tokenization schemes this is not the case,
 * though.
 */
class TokenTreeNode {
public:
	const std::map<char, TokenTreeNode> children;
	const int tokenId;

	/**
	 * The TokenTreeNode constructor builds a TokenTree from the given token
	 * specifications. The node returned by this constructor then is the root of
	 * said TokenTree.
	 * @param inputs Specifications of tokens in map form. Each specification
	 * is a tuple of the text that should be matched and some unique ID (>= 0)
	 * that is returned to you if that Token is found in the text.
	 * An example for such a map would be
	 * {
	 *	{ "#" , 1},
	 *  { "##", 2},
	 *  { "/" , 3}
	 * }
	 * Note that IDs below zero are reserved for system Ids, mainly TOKEN_NONE
	 * (-1) and TOKEN_TEXT (-2).
	 */
	TokenTreeNode(const std::map<std::string, int> &inputs);
};

/**
 * This is a reserved constant for the empty token.
 */
static const int TOKEN_NONE = -1;
/**
 * This is a reserved constant for every part of the input text that is not a
 * specified token.
 */
static const int TOKEN_TEXT = -2;

/**
 * A token for us is identified by an integer tokenID (either one of the
 * constants TOKEN_NONE or TOKEN_TEXT or one of the user-defined constants).
 * Additionally we return the matched text (which should only be really
 * interesting in case of TOKEN_TEXT tokens) and the position in the input text.
 */
struct Token {
	int tokenId;
	std::string content;
	int startColumn;
	int startLine;
	int endColumn;
	int endLine;

	Token(int tokenId, std::string content, int startColumn, int startLine,
	      int endColumn, int endLine)
	    : tokenId(tokenId),
	      content(content),
	      startColumn(startColumn),
	      startLine(startLine),
	      endColumn(endColumn),
	      endLine(endLine)
	{
	}

	Token() : tokenId(TOKEN_NONE) {}
};

/**
 * A Tokenizer has the purpose of subdividing an input text into tokens. In our
 * definition here we distinguish between two kinds of tokens:
 * 1.) User-specified tokens that match a fixed text.
 * 2.) Any other text between those tokens.
 * The user might want to specify the tokens '#{' and '#}' for example, because
 * they have some meaning in her code. The user sets the IDs to 1 and 2.
 * Given the input text
 * "some text #{ special command #} some text"
 * the tokenizer would return the tokens:
 * 1.) "some text " with the id TOKEN_TEXT (-2).
 * 2.) "#{" with the id 1.
 * 3.) " special command " with the id TOKEN_TEXT (-2).
 * 4.) "#}" with the id 2.
 * 5.) " some text" with the id TOKEN_TEXT (-2).
 * This makes the subsequent parsing of files of a specific type easier.
 * Note that in case of tokens with that are prefixes of other tokens the
 * longest possible match is returned.
 */
class Tokenizer {
private:
	BufferedCharReader &input;
	const TokenTreeNode &root;
	std::deque<Token> peeked;
	unsigned int peekCursor = 0;

	bool prepare();

protected:
	/**
	* This method is an interface to build multiple tokens from a single one in
	* derived classes. This might be interesting if you want to implement
	* further logic on text tokens or similar applications.
	*
	* @param t a Token the "basic" tokenizer found.
	* @param peeked a reference to the deque containing all temporary Tokens.
	* You are supposed to append your tokens there. In the trivial case you just
	* put the given Token on top of the deque.
	* @return false if no token was appended to the deque (meaning that you want
	* to ignore the given token explicitly) and true in all other cases.
	*/
	virtual bool doPrepare(const Token &t, std::deque<Token> &peeked);

public:
	/**
	 * @param input The input of a Tokenizer is given in the form of a
	 * BufferedCharReader. Please refer to the respective documentation.
	 * @param root This is meant to be the root of a TokenTree giving the
	 * specification of user-defined tokens this Tokenizer should recognize.
	 * The Tokenizer promises to not change the TokenTree such that you can
	 * re-use the same specification for multiple inputs.
	 * Please refer to the TokenTreeNode documentation for more information.
	 */
	Tokenizer(BufferedCharReader &input, const TokenTreeNode &root);

	/**
	 * The next method consumes one Token from the input stream and gives
	 * it to the user (stored in the input argument).
	 *
	 * @param t a Token reference that is set to the next found token.
	 * @return true if a next token was found and false if the input is at its
	 * end.
	 */
	bool next(Token &t);
	/**
	 * The peek method does not consume the next Token but buffers it and
	 * shows it to the user (stored in the input argument).
	 *
	 * @param t a Token reference that is set to the next found token.
	 * @return true if a next token was found and false if the input is at its
	 * end.
	 */
	bool peek(Token &t);

	/**
	 * Resets the peek pointer to the current position in the stream (to the
	 * beginning of the buffer).
	 */
	void resetPeek();

	/**
	 * Clears the peek buffer, such that all peeked Tokens are consumed.
	 */
	void consumePeek();

	const BufferedCharReader &getInput() const { return input; }
	
	BufferedCharReader &getInput() { return input; }
};
}

#endif
