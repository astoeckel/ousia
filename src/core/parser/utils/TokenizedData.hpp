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
 * @file TokenizedData.hpp
 *
 * The TokenizedData class defined in this file stores string data extracted
 * from a document including user defined tokens. Tokens can be dynamically
 * enabled and disabled. And the data up to the next enabled token can be
 * returned. Additionally, the data provided by the TokenizedData class is
 * processed according to a whitespace mode that can be dynamically updated.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_TOKENIZED_DATA_HPP_
#define _OUSIA_TOKENIZED_DATA_HPP_

#include <cstdint>
#include <memory>
#include <unordered_set>

#include <core/common/Location.hpp>
#include <core/common/Whitespace.hpp>

#include "Token.hpp"

namespace ousia {

// Forward declaration
class TokenizedDataImpl;

/**
 * The TokenizedData class stores data extracted from a user defined document.
 * As users are capable of defining their own tokens and these are only valid
 * in certain scopes TokenizedData allows to divide the stored data into chunks
 * separated by tokens.
 */
class TokenizedData {
private:
	/**
	 * Shared pointer pointing at the internal data. This data is shared when
	 * copying TokenizedData instances, which corresponds to forking a
	 * TokenizedData instance.
	 */
	std::shared_ptr<TokenizedDataImpl> impl;

	/**
	 * Contains all currently enabled token ids.
	 */
	std::unordered_set<TokenId> tokens;

	/**
	 * Position from which the last element was read from the internal buffer.
	 * This information is not shared with the other instances of TokenizedData
	 * pointing at the same location.
	 */
	size_t cursor;

public:
	/**
	 * Default constructor, creates a new instance of TokenizedData, sets the
	 * internal SourceId to the InvalidSourceId constant.
	 */
	TokenizedData();

	/**
	 * Creates a new instance of TokenizedData, takes a SourceId.
	 *
	 * @param sourceId is the source identifier that should be used for
	 * constructing the location when returning tokens.
	 */
	TokenizedData(SourceId sourceId);

	/**
	 * Destructor. Needs to be defined explicitly for freeing a shared pointer
	 * of the incomplete TokenizedDataImpl type.
	 */
	~TokenizedData();

	/**
	 * Appends a complete string to the internal character buffer. Note that the
	 * start and end positions for each character in the given data string will
	 * be interpolated and may thus be incorrect (e.g. when multi-character
	 * linebreaks or multi-character characters (not handled now) are read).
	 *
	 * @param data is the string that should be appended to the buffer.
	 * @param offsStart is the start offset in bytes in the input file.
	 * @return the current size of the internal byte buffer. The returned value
	 * is intended to be used for the "mark" function.
	 */
	size_t append(const std::string &data, SourceOffset offsStart = 0);

	/**
	 * Appends a single character to the internal character buffer.
	 *
	 * @param c is the character that should be appended to the buffer.
	 * @param start is the start offset in bytes in the input file.
	 * @param end is the end offset in bytes in the input file.
	 * @return the current size of the internal byte buffer. The returned value
	 * is intended to be used for the "mark" function.
	 */
	size_t append(char c, SourceOffset offsStart, SourceOffset offsEnd);

	/**
	 * Stores a token ending at the last character of the current buffer.
	 *
	 * @param id is the id of the token for which the mark should be stored.
	 * @param len is the length of the token.
	 */
	void mark(TokenId id, TokenLength len);

	/**
	 * Stores a token at the given position.
	 *
	 * @param id is the if of the token for which the mark should be stored.
	 * @param bufStart is the start position in the internal buffer. Use the
	 * values returned by append to calculate the start position.
	 * @param len is the length of the token.
	 */
	void mark(TokenId id, size_t bufStart, TokenLength len);

	/**
	 * Enables a single token id. Enabled tokens will no longer be returned as
	 * text. Instead, when querying for the next token, TokenizedData will
	 * return them as token and not as part of a Text token.
	 *
	 * @param id is the TokenId of the token that should be enabled.
	 */
	void enableToken(TokenId id) { tokens.insert(id); }

	/**
	 * Enables a set of token ids. Enabled tokens will no longer be returned as
	 * text. Instead, when querying for the next token, TokenizedData will
	 * return them as token and not as part of a Text token.
	 *
	 * @param ids is the TokenId of the token that should be enabled.
	 */
	void enableToken(const std::unordered_set<TokenId> &ids)
	{
		tokens.insert(ids.begin(), ids.end());
	}

	/**
	 * Stores the next token in the given token reference, returns true if the
	 * operation was successful, false if there are no more tokens.
	 *
	 * @param token is an output parameter into which the read token will be
	 * stored. The TokenId is set to Tokens::Empty if there are no more tokens.
	 * @param mode is the whitespace mode that should be used when a text token
	 * is returned.
	 * @return true if the operation was successful and there is a next token,
	 * false if there are no more tokens.
	 */
	bool next(Token &token, WhitespaceMode mode = WhitespaceMode::COLLAPSE);

	/**
	 * Stores the next text token in the given token reference, returns true if
	 * the operation was successful (there was indeed a text token), false if
	 * the next token is not a text token or there were no more tokens.
	 *
	 * @param token is an output parameter into which the read token will be
	 * stored. The TokenId is set to Tokens::Empty if there are no more tokens.
	 * @param mode is the whitespace mode that should be used when a text token
	 * is returned.
	 * @return true if the operation was successful and there is a next token,
	 * false if there are no more tokens.
	 */
	bool text(Token &token, WhitespaceMode mode = WhitespaceMode::COLLAPSE);
};
}

#endif /* _OUSIA_DYNAMIC_TOKENIZER_HPP_ */

