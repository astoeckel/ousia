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

#include <algorithm>
#include <limits>
#include <vector>

#include <core/common/Utils.hpp>

#include "SourceOffsetVector.hpp"
#include "TokenizedData.hpp"

namespace ousia {
/**
 * Maximum token length.
 */
constexpr TokenLength MaxTokenLength = std::numeric_limits<TokenLength>::max();

namespace {
/**
 * Structure used to represent the position of a token in the internal
 * character buffer.
 */
struct TokenMark {
	/**
	 * Relative position of the token in the buffer.
	 */
	size_t bufStart;

	/**
	 * TokenId of the associated token.
	 */
	TokenId id;

	/**
	 * Length of the token.
	 */
	TokenLength len;

	/**
	 * Specifies whether the token is special or not.
	 */
	bool special;

	/**
	 * Constructor of the TokenMark structure, initializes all members with the
	 * given values.
	 *
	 * @param id is the id of the token that is marked here.
	 * @param bufStart is the start position of the TokenMark in the internal
	 * character buffer.
	 * @param len is the length of the token.
	 * @param special modifies the sort order, special tokens are prefered.
	 */
	TokenMark(TokenId id, size_t bufStart, TokenLength len, bool special)
	    : bufStart(bufStart), id(id), len(len), special(special)
	{
	}

	/**
	 * Creates a dummy TokenMark instance used for comparison purposes. This
	 * TokenMark will compare to be smaller than an equal TokenMark with
	 * equivalent start.
	 *
	 * @param bufStart is start position of the TokenMark in the internal
	 * character buffer.
	 */
	TokenMark(size_t bufStart)
	    : bufStart(bufStart),
	      id(Tokens::Empty),
	      len(MaxTokenLength),
	      special(true)
	{
	}

	/**
	 * Operator used for sorting TokenMark instances. They are sorted in such a
	 * way that the instances with smallest bufStart come first and for equal
	 * bufStart values instances with the largest length come first.
	 *
	 * @param m1 is the left-hand side TokenMark instance of the comparison.
	 * @param m2 is the right-hand side TokenMark instance of the comparison.
	 */
	friend bool operator<(const TokenMark &m1, const TokenMark &m2)
	{
		// Prefer the mark with the smaller bufStart
		if (m1.bufStart < m2.bufStart) {
			return true;
		}

		// Special handling for marks with the same bufStart
		if (m1.bufStart == m2.bufStart) {
			// If exactly one of the two marks is special, return true if this
			// one is special
			if (m1.special != m2.special) {
				return m1.special;
			}
			// Otherwise prefer longer marks
			return m1.len > m2.len;
		}
		return false;
	}
};
}

/**
 * Structure used to hold all the internal data structures that may be
 * exchanged between TokenizedData instances.
 */
class TokenizedDataImpl {
private:
	/**
	 * SourceId representing the source file from which the current content is
	 * being read.
	 */
	SourceId sourceId;

	/**
	 * Buffer containing the actual character data.
	 */
	std::vector<char> buf;

	/**
	 * Buffset storing the "protected" flag of the character data.
	 */
	std::vector<bool> protectedChars;

	/**
	 * Vector storing all the character offsets efficiently.
	 */
	SourceOffsetVector offsets;

	/**
	 * Vector containing all token marks.
	 */
	mutable std::vector<TokenMark> marks;

	/**
	 * Position of the first linebreak in a sequence of linebreaks.
	 */
	size_t firstLinebreak;

	/**
	 * Current indentation level.
	 */
	uint16_t currentIndentation;

	/**
	 * Last indentation level.
	 */
	uint16_t lastIndentation;

	/**
	 * Number of linebreaks without any content between them.
	 */
	uint16_t numLinebreaks;

	/**
	 * Flag indicating whether the internal "marks" vector is sorted.
	 */
	mutable bool sorted;

public:
	/**
	 * Constructor of TokenizedDataImpl. Takes the SourceId that should be used
	 * for returned tokens.
	 *
	 * @param sourceId is the source identifier that should be used for
	 * constructing the location when returning tokens.
	 */
	TokenizedDataImpl(SourceId sourceId) : sourceId(sourceId) { clear(); }

	/**
	 * Appends a complete string to the internal character buffer and extends
	 * the text regions in the regions map.
	 *
	 * @param data is the string that should be appended to the buffer.
	 * @param offsStart is the start offset in bytes in the input file.
	 * @param protect if set to true, the appended characters will not be
	 * affected by whitespace handling, they will be returned as is.
	 * @return the current size of the internal byte buffer. The returned value
	 * is intended to be used for the "mark" function.
	 */
	size_t append(const std::string &data, SourceOffset offsStart, bool protect)
	{
		for (size_t i = 0; i < data.size(); i++) {
			if (offsStart != InvalidSourceOffset) {
				append(data[i], offsStart + i, offsStart + i + 1, protect);
			} else {
				append(data[i], InvalidSourceOffset, InvalidSourceOffset,
				       protect);
			}
		}
		return size();
	}

	/**
	 * Appends a single character to the internal character buffer and extends
	 * the text regions in the regions map.
	 *
	 * @param c is the character that should be appended to the buffer.
	 * @param offsStart is the start offset in bytes in the input file.
	 * @param offsEnd is the end offset in bytes in the input file.
	 * @param protect if set to true, the appended character will not be
	 * affected by whitespace handling, it will be returned as is.
	 * @return the current size of the internal byte buffer. The returned value
	 * is intended to be used for the "mark" function.
	 */
	size_t append(char c, SourceOffset offsStart, SourceOffset offsEnd,
	              bool protect)
	{
		// Add the character to the list and store the location of the character
		// in the source file
		buf.push_back(c);
		protectedChars.push_back(protect);
		offsets.storeOffset(offsStart, offsEnd);

		// Insert special tokens
		const size_t size = buf.size();
		const bool isWhitespace = Utils::isWhitespace(c);
		const bool isLinebreak = Utils::isLinebreak(c);

		// Handle linebreaks
		if (isLinebreak) {
			// Mark linebreaks as linebreak
			mark(Tokens::Newline, size - 1, 1, false);

			// The linebreak sequence started at the previous character
			if (numLinebreaks == 0) {
				firstLinebreak = size - 1;
			}

			// Reset the indentation
			currentIndentation = 0;

			// Increment the number of linebreaks
			numLinebreaks++;

			const size_t markStart = firstLinebreak;
			const size_t markLength = size - firstLinebreak;

			// Issue two consecutive linebreaks as paragraph token
			if (numLinebreaks == 2) {
				mark(Tokens::Paragraph, markStart, markLength, false);
			}

			// Issue three consecutive linebreaks as paragraph token
			if (numLinebreaks >= 3) {
				mark(Tokens::Section, markStart, markLength, false);
			}
		} else if (isWhitespace) {
			// Count the whitespace characters at the beginning of the line
			if (numLinebreaks > 0) {
				// Implement the UNIX/Pyhton rule for tabs: Tabs extend to the
				// next multiple of eight.
				if (c == '\t') {
					currentIndentation = (currentIndentation + 8) & ~7;
				} else {
					currentIndentation++;
				}
			}
		}

		// Issue indent and unindent tokens
		if (!isWhitespace && numLinebreaks > 0) {
			// Issue a larger indentation than that in the previous line as
			// "Indent" token
			if (currentIndentation > lastIndentation) {
				mark(Tokens::Indent, size - 1, 0, true);
			}

			// Issue a smaller indentation than that in the previous line as
			// "Dedent" token
			if (currentIndentation < lastIndentation) {
				mark(Tokens::Dedent, size - 1, 0, true);
			}

			// Reset the internal state machine
			lastIndentation = currentIndentation;
			numLinebreaks = 0;
		}

		return size;
	}

	/**
	 * Stores a token at the given position.
	 *
	 * @param id is the token that should be stored.
	 * @param bufStart is the start position in the internal buffer. Use the
	 * values returned by append to calculate the start position.
	 * @param len is the length of the token.
	 * @param special tags the mark as "special", prefering it in the sort order
	 */
	void mark(TokenId id, size_t bufStart, TokenLength len, bool special)
	{
		// Push the new instance back onto the list
		marks.emplace_back(id, bufStart, len, special);

		// Update the sorted flag as soon as more than one element is in the
		// list
		if (marks.size() > 1U) {
			sorted = sorted && *(marks.end() - 2) < *(marks.end() - 1);
		}
	}

	/**
	 * Returns the next token or a text token if no explicit token is available.
	 * Advances the given cursor to the end of the returned token.
	 *
	 * @param token is the Token instance to which the token should be written.
	 * @param mode is the WhitespaceMode to be used for extracting the text
	 * cursor.
	 * @param tokens is a set of enabled tokens. Tokens that are not in this set
	 * are ignored and returned as text.
	 * @param cursor is the position in the character buffer from which on the
	 * next token should be read. The cursor will be updated to the position
	 * beyond the returned token.
	 * @return true if a token was returned, false if no more tokens are
	 * available.
	 */
	bool next(Token &token, WhitespaceMode mode, const TokenSet &tokens,
	          TokenizedDataCursor &cursor) const
	{
		// Some variables for convenient access
		size_t &bufPos = cursor.bufPos;
		size_t &markPos = cursor.markPos;

		// Sort the "marks" vector if it has not been sorted yet.
		if (!sorted) {
			std::sort(marks.begin(), marks.end());
			sorted = true;
		}

		// Fetch the next larger TokenMark instance, make sure the token is in
		// the "enabled" list and within the buffer range
		auto it = std::lower_bound(marks.begin() + markPos, marks.end(),
		                           TokenMark(bufPos));
		while (it != marks.end() && (tokens.count(it->id) == 0 ||
		                             it->bufStart + it->len > buf.size())) {
			it++;
		}

		// Calculate the buffer start and end character, based on the returned
		// TokenMark instance
		const size_t end = (it != marks.end()) ? it->bufStart : buf.size();

		// Depending on the whitespace mode, fetch all the data between the
		// cursor position and the calculated end position and return a token
		// containing that data.
		if (bufPos < end && bufPos < buf.size()) {
			switch (mode) {
				case WhitespaceMode::PRESERVE: {
					token = Token(
					    Tokens::Data, std::string(&buf[bufPos], end - bufPos),
					    SourceLocation(sourceId,
					                   offsets.loadOffset(bufPos).first,
					                   offsets.loadOffset(end).first));
					bufPos = end;
					return true;
				}
				case WhitespaceMode::TRIM:
				case WhitespaceMode::COLLAPSE: {
					// Calculate the collapsed string and the corresponding
					// trimmed region
					size_t stringStart;
					size_t stringEnd;
					std::string content;
					const char *cBuf = &buf[bufPos];
					auto filter = [cBuf, this](size_t i) -> bool {
						return Utils::isWhitespace(cBuf[i]) &&
						       !protectedChars[i];
					};
					if (mode == WhitespaceMode::TRIM) {
						content = Utils::trim(cBuf, end - bufPos, stringStart,
						                      stringEnd, filter);
					} else {
						content = Utils::collapse(
						    cBuf, end - bufPos, stringStart, stringEnd, filter);
					}

					// If the resulting string is empty (only whitespaces),
					// abort
					if (content.empty()) {
						bufPos = end;
						break;
					}

					// Calculate the absolute positions and return the token
					stringStart += bufPos;
					stringEnd += bufPos;
					token = Token(
					    Tokens::Data, content,
					    SourceLocation(sourceId,
					                   offsets.loadOffset(stringStart).first,
					                   offsets.loadOffset(stringEnd).first));
					bufPos = end;
					return true;
				}
			}
		}

		// If start equals end, we're currently directly at a token
		// instance. Return this token and advance the cursor to the end of
		// the token.
		if (bufPos == end && it != marks.end()) {
			const size_t tokenStart = it->bufStart;
			const size_t tokenEnd = it->bufStart + it->len;
			token = Token(
			    it->id, std::string(&buf[tokenStart], it->len),
			    SourceLocation(sourceId, offsets.loadOffset(tokenStart).first,
			                   offsets.loadOffset(tokenEnd).first));

			// Update the cursor, consume the token by incrementing the marks
			// pos counter
			bufPos = tokenEnd;
			markPos = it - marks.begin() + 1;
			return true;
		}

		// We've failed. There is no token. Only void. Reset token and return
		// false.
		token = Token();
		return false;
	}

	/**
	 * Resets the TokenizedDataImpl instance to the state it had when it was
	 * constructred.
	 */
	void clear()
	{
		buf.clear();
		protectedChars.clear();
		offsets.clear();
		marks.clear();
		firstLinebreak = 0;
		currentIndentation = 0;
		lastIndentation = 0;
		numLinebreaks = 1;  // Assume the stream starts with a linebreak
		sorted = true;
	}

	/**
	 * Trims the length of the TokenizedDataImpl instance to the given length.
	 *
	 * @param length is the number of characters to which the TokenizedData
	 * instance should be trimmed.
	 */
	void trim(size_t length)
	{
		if (length < size()) {
			buf.resize(length);
			protectedChars.resize(length);
			offsets.trim(length);
		}
	}

	/**
	 * Returns the current size of the internal buffer.
	 *
	 * @return the size of the internal character buffer.
	 */
	size_t size() const { return buf.size(); }

	/**
	 * Returns true if no data is in the data buffer.
	 *
	 * @return true if the "buf" instance has no data.
	 */
	bool empty() const { return buf.empty(); }

	/**
	 * Returns the current location of all data in the buffer.
	 *
	 * @return the location of the entire data represented by this instance.
	 */
	SourceLocation getLocation() const
	{
		if (empty()) {
			return SourceLocation{sourceId};
		}
		return SourceLocation{sourceId, offsets.loadOffset(0).first,
		                      offsets.loadOffset(size()).second};
	}
};

/* Class TokenizedData */

TokenizedData::TokenizedData() : TokenizedData(InvalidSourceId) {}

TokenizedData::TokenizedData(SourceId sourceId)
    : impl(std::make_shared<TokenizedDataImpl>(sourceId))
{
}

TokenizedData::TokenizedData(const std::string &data, SourceOffset offsStart,
                             SourceId sourceId)
    : TokenizedData(sourceId)
{
	append(data, offsStart);
}

TokenizedData::~TokenizedData() {}

size_t TokenizedData::append(const std::string &data, SourceOffset offsStart,
                             bool protect)
{
	return impl->append(data, offsStart, protect);
}

size_t TokenizedData::append(char c, SourceOffset offsStart,
                             SourceOffset offsEnd, bool protect)
{
	return impl->append(c, offsStart, offsEnd, protect);
}

void TokenizedData::mark(TokenId id, TokenLength len)
{
	impl->mark(id, impl->size() - len, len, false);
}

void TokenizedData::mark(TokenId id, size_t bufStart, TokenLength len)
{
	impl->mark(id, bufStart, len, false);
}

void TokenizedData::clear() { impl->clear(); }

void TokenizedData::trim(size_t length) { impl->trim(length); }

size_t TokenizedData::size() const { return impl->size(); }

bool TokenizedData::empty() const { return impl->empty(); }

SourceLocation TokenizedData::getLocation() const
{
	return impl->getLocation();
}

TokenizedDataReader TokenizedData::reader() const
{
	return TokenizedDataReader(impl, TokenizedDataCursor(),
	                           TokenizedDataCursor());
}

/* Class TokenizedDataReader */

TokenizedDataReader::TokenizedDataReader(
    std::shared_ptr<const TokenizedDataImpl> impl,
    const TokenizedDataCursor &readCursor,
    const TokenizedDataCursor &peekCursor)
    : impl(impl), readCursor(readCursor), peekCursor(peekCursor)
{
}

TokenizedDataReaderFork TokenizedDataReader::fork()
{
	return TokenizedDataReaderFork(*this, impl, readCursor, peekCursor);
}

bool TokenizedDataReader::atEnd() const
{
	return readCursor.bufPos >= impl->size();
}

bool TokenizedDataReader::read(Token &token, const TokenSet &tokens,
                               WhitespaceMode mode)
{
	peekCursor = readCursor;
	return impl->next(token, mode, tokens, readCursor);
}

bool TokenizedDataReader::peek(Token &token, const TokenSet &tokens,
                               WhitespaceMode mode)
{
	return impl->next(token, mode, tokens, peekCursor);
}
}
