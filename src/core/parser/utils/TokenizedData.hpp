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
#include <core/common/Variant.hpp>
#include <core/common/Whitespace.hpp>
#include <core/common/Token.hpp>

namespace ousia {

// Forward declaration
class TokenizedDataImpl;
class TokenizedDataReader;
class TokenizedDataReaderFork;

/**
 * The TokenizedData class stores data extracted from a user defined document.
 * The data stored in TokenizedData
 */
class TokenizedData {
private:
	/**
	 * Shared pointer pointing at the internal data. This data is shared with
	 * all the TokenizedDataReader instances.
	 */
	std::shared_ptr<TokenizedDataImpl> impl;

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
	 * Resets the TokenizedData instance to the state it had when it was
	 * constructred.
	 */
	void clear();

	/**
	 * Trims the length of the TokenizedData instance to the given length. Note
	 * that this function does not remove any token matches for performance
	 * reasons, it merely renders them incaccessible. Appending new data after
	 * calling trim will make the token marks accessible again. Thus this method
	 * should be the last function called to modify the data buffer and the
	 * token marks.
	 *
	 * @param length is the number of characters to which the TokenizedData
	 * instance should be trimmed.
	 */
	void trim(size_t length);

	/**
	 * Returns the number of characters currently represented by this
	 * TokenizedData instance.
	 */
	size_t size() const;

	/**
	 * Returns true if the TokenizedData instance is empty, false otherwise.
	 *
	 * @return true if not data is stored inside the TokenizedData instance.
	 */
	bool empty() const;

	/**
	 * Returns the location of the entire TokenizedData instance.
	 *
	 * @return the location of the entire data represented by this instance.
	 */
	SourceLocation getLocation() const;

	/**
	 * Returns a TokenizedDataReader instance that can be used to access the
	 * data.
	 *
	 * @return a new TokenizedDataReader instance pointing at the beginning of
	 * the internal buffer.
	 */
	TokenizedDataReader reader() const;
};

/**
 * The TokenizedDataReader
 */
class TokenizedDataReader {
private:
	friend TokenizedData;

	/**
	 * Shared pointer pointing at the internal data. This data is shared with
	 * all the TokenizedDataReader instances.
	 */
	std::shared_ptr<const TokenizedDataImpl> impl;

	/**
	 * Position from which the last element was read from the internal buffer.
	 */
	size_t readCursor;

	/**
	 * Position from which the last element was peeked from the internal buffer.
	 */
	size_t peekCursor;

	/**
	 * Private constructor of TokenizedDataReader, taking a reference to the
	 * internal TokenizedDataImpl structure storing the data that is accessed by
	 * the reader.
	 *
	 * @param impl is the TokenizedDataImpl instance that holds the actual data.
	 * @param readCursor is the cursor position from which tokens and text are
	 * read.
	 * @param peekCursor is the cursor position from which tokens and text are
	 * peeked.
	 */
	TokenizedDataReader(std::shared_ptr<TokenizedDataImpl> impl,
	                    size_t readCursor, size_t peekCursor);

public:
	/**
	 * Returns a new TokenizedDataReaderFork from which tokens and text can be
	 * read without advancing this reader instance.
	 */
	TokenizedDataReaderFork fork();

	/**
	 * Returns true if this TokenizedData instance is at the end.
	 *
	 * @return true if the end of the TokenizedData instance has been reached.
	 */
	bool atEnd() const;

	/**
	 * Stores the next token in the given token reference, returns true if the
	 * operation was successful, false if there are no more tokens. Advances the
	 * internal cursor and re
	 *
	 * @param token is an output parameter into which the read token will be
	 * stored. The TokenId is set to Tokens::Empty if there are no more tokens.
	 * @param tokens is the set of token identifers, representing the currently
	 * enabled tokens.
	 * @param mode is the whitespace mode that should be used when a text token
	 * is returned.
	 * @return true if the operation was successful and there is a next token,
	 * false if there are no more tokens.
	 */
	bool read(Token &token, const TokenSet &tokens = TokenSet{},
	          WhitespaceMode mode = WhitespaceMode::COLLAPSE);

	/**
	 * Stores the next token in the given token reference, returns true if the
	 * operation was successful, false if there are no more tokens.
	 *
	 * @param token is an output parameter into which the read token will be
	 * stored. The TokenId is set to Tokens::Empty if there are no more tokens.
	 * @param tokens is the set of token identifers, representing the currently
	 * enabled tokens.
	 * @param mode is the whitespace mode that should be used when a text token
	 * is returned.
	 * @return true if the operation was successful and there is a next token,
	 * false if there are no more tokens.
	 */
	bool peek(Token &token, const TokenSet &tokens = TokenSet{},
	          WhitespaceMode mode = WhitespaceMode::COLLAPSE);

	/**
	 * Consumes the peeked tokens, the read cursor will now be at the position
	 * of the peek cursor.
	 */
	void consumePeek() { readCursor = peekCursor; }

	/**
	 * Resets the peek cursor to the position of the read cursor.
	 */
	void resetPeek() { peekCursor = readCursor; }

	/**
	 * Stores the next text token in the given token reference, returns true if
	 * the operation was successful (there was indeed a text token), false if
	 * the next token is not a text token or there were no more tokens.
	 *
	 * @param token is an output parameter into which the read token will be
	 * stored. The TokenId is set to Tokens::Empty if there are no more tokens.
	 * @param mode is the whitespace mode that should be used when a text token
	 * is returned.
	 * @return a string variant with the data if there is any data or a nullptr
	 * variant if there is no text.
	 */
	Variant text(WhitespaceMode mode = WhitespaceMode::COLLAPSE);
};

/**
 * The TokenizedDataReaderFork class is created when forking a
 * TokenizedDataReader
 */
class TokenizedDataReaderFork : public TokenizedDataReader {
private:
	friend TokenizedDataReader;

	/**
	 * Reference pointing at the parent TokenizedDataReader to which changes may
	 * be commited.
	 */
	TokenizedDataReader &parent;

	/**
	 * Private constructor of TokenizedDataReaderFork, taking a reference to the
	 * internal TokenizedDataImpl structure storing the data that is accessed by
	 * the reader and a reference at the parent TokenizedDataReader.
	 *
	 * @param parent is the TokenizedDataReader instance to which the current
	 * read/peek progress may be commited.
	 * @param impl is the TokenizedDataImpl instance that holds the actual data.
	 * @param readCursor is the cursor position from which tokens and text are
	 * read.
	 * @param peekCursor is the cursor position from which tokens and text are
	 * peeked.
	 */
	TokenizedDataReaderFork(TokenizedDataReader &parent,
	                        std::shared_ptr<TokenizedDataImpl> impl,
	                        size_t readCursor, size_t peekCursor)
	    : TokenizedDataReader(impl, readCursor, peekCursor), parent(parent)
	{
	}

public:
	/**
	 * Commits the read/peek progress to the underlying parent.
	 */
	void commit() { parent = *this; }
}
}

#endif /* _OUSIA_TOKENIZED_DATA_HPP_ */

