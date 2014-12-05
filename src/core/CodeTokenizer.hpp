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

#ifndef _OUSIA_CODE_TOKENIZER_HPP_
#define _OUSIA_CODE_TOKENIZER_HPP_

#include <map>
#include <sstream>

#include "BufferedCharReader.hpp"
#include "Tokenizer.hpp"

namespace ousia {

/*
 * This enum contains all special Token the CodeTokenizer supports, namely:
 *
 * 1.) An ambigous Tokens - in post programming languages single-quotes ' or
 * double-quotes " - to delimit string tokens.
 * 2.) A start token for line comments, which would e.g. be // in Java.
 * 3.) A start token for a block comment
 * 4.) An end token for a block comment.
 * 5.) A linebreak token
 * 6.) The escape token, which would e.g. be \ in java.
 */
enum class CodeTokenMode {
	STRING_START_END,
	LINE_COMMENT,
	BLOCK_COMMENT_START,
	BLOCK_COMMENT_END,
	LINEBREAK,
	ESCAPE,
	NONE
};

/**
 * A CodeTokenDescriptor defines the id the user likes to have returned for
 * a Token of the mode specified, e.g. if you want to get the id 4 for a
 * String Token the corresponding CodeTokenDescriptor would be inizialized
 * with CodeTokenDescriptor myDesc {CodeTokenMode::STRING_START_END, 4};
 */
struct CodeTokenDescriptor {
	CodeTokenMode mode;
	int id;

	CodeTokenDescriptor(CodeTokenMode mode, int id) : mode(mode), id(id) {}
};

/**
 * The CodeTokenizer is a finite state machine with the states NORMAL, being
 * IN_BLOCK_COMMENT, being IN_LINE_COMMENT or being IN_STRING.
 */
enum class CodeTokenizerState {
	NORMAL,
	IN_BLOCK_COMMENT,
	IN_LINE_COMMENT,
	IN_STRING
};

/**
 * The purpose of a CodeTokenizer is to make it easier to parse classical
 * programming Code. It adds the following features to a regular Tokenizer:
 * 1.) String tokens (e.g. "string" in Java Code) instead of 3 separate tokens
 * for the opening delimiter, the text and the closing delimiter.
 * 2.) Escaping in String tokens.
 * 3.) Comment Tokens (for line comments as well as block comments)
 */
class CodeTokenizer : public Tokenizer {
private:
	std::map<int, CodeTokenDescriptor> descriptors;
	CodeTokenizerState state;
	std::stringstream buf;
	Token startToken;
	int returnTokenId;
	bool escaped = false;

	Token constructToken(const Token &t);
	void buffer(const Token &t);

protected:
	bool doPrepare(const Token &t, std::deque<Token> &peeked) override;

public:
	/**
	 * If you do not want comment tokens to be returned you can set this to
	 * true.
	 */
	bool ignoreComments = false;
	/**
	 * If you do not want linebreaks to be returned you can set this to true.
	 */
	 bool ignoreLinebreaks = false;

	/**
	 *
	 * @param input a BufferedCharReader containing the input for this
	 * tokenizer, as with a regular tokenizer.
	 * @param root a TokenTreeNode representing the root of the TokenTree.
	 * Please note that you have to specify all tokenIDs here that you use
	 * in the descriptors map.
	 * @param descriptors a map mapping tokenIDs to CodeTokenDescriptors.
	 * In this way you can specify the meaning of certain Tokens. Say you
	 * specified the Token "//" with the id 1 in the TokenTree. Then you could
	 * add the entry "1" with the Mode "LINE_COMMENT" to the descriptors map
	 * and this CodeTokenizer would recognize the token "//" as starting a
	 * line comment.
	 */
	CodeTokenizer(BufferedCharReader &input, const TokenTreeNode &root,
	              std::map<int, CodeTokenDescriptor> descriptors)
	    : Tokenizer(input, root), descriptors(descriptors), state(CodeTokenizerState::NORMAL)
	{
	}
};
}

#endif
