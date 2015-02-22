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
	 * Constructor of the TokenMark structure, initializes all members with the
	 * given values.
	 *
	 * @param id is the id of the token that is marked here.
	 * @param bufStart is the start position of the TokenMark in the internal
	 * character buffer.
	 * @param len is the length of the token.
	 */
	TokenMark(TokenId id, size_t bufStart, TokenLength len)
	    : bufStart(bufStart), id(id), len(len)
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
	      len(std::numeric_limits<TokenLength>::max())
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
		return (m1.bufStart < m2.bufStart) ||
		       (m1.bufStart == m2.bufStart && m1.len > m2.len);
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
	 * Vector containing all token marks.
	 */
	std::vector<TokenMark> marks;

	/**
	 * Vector storing all the character offsets efficiently.
	 */
	SourceOffsetVector offsets;

	/**
	 * Flag indicating whether the internal "marks" vector is sorted.
	 */
	bool sorted;

public:
	/**
	 * Constructor of TokenizedDataImpl. Takes the SourceId that should be used
	 * for returned tokens.
	 *
	 * @param sourceId is the source identifier that should be used for
	 * constructing the location when returning tokens.
	 */
	TokenizedDataImpl(SourceId sourceId) : sourceId(sourceId), sorted(true) {}

	/**
	 * Appends a complete string to the internal character buffer and extends
	 * the text regions in the regions map.
	 *
	 * @param data is the string that should be appended to the buffer.
	 * @param offsStart is the start offset in bytes in the input file.
	 * @return the current size of the internal byte buffer. The returned value
	 * is intended to be used for the "mark" function.
	 */
	size_t append(const std::string &data, SourceOffset offsStart)
	{  // Append the data to the internal buffer
		buf.insert(buf.end(), data.begin(), data.end());

		// Extend the text regions, interpolate the source position (this may
		// yield incorrect results)
		const size_t size = buf.size();
		for (SourceOffset offs = offsStart; offs < offsStart + data.size();
		     offs++) {
			offsets.storeOffset(offs, offs + 1);
		}

		return size;
	}

	/**
	 * Appends a single character to the internal character buffer and extends
	 * the text regions in the regions map.
	 *
	 * @param c is the character that should be appended to the buffer.
	 * @param offsStart is the start offset in bytes in the input file.
	 * @param offsEnd is the end offset in bytes in the input file.
	 * @return the current size of the internal byte buffer. The returned value
	 * is intended to be used for the "mark" function.
	 */
	size_t append(char c, SourceOffset offsStart, SourceOffset offsEnd)
	{
		// Add the character to the list and store the location of the character
		// in the source file
		buf.push_back(c);
		offsets.storeOffset(offsStart, offsEnd);
		return buf.size();
	}

	/**
	 * Stores a token at the given position.
	 *
	 * @param id is the token that should be stored.
	 * @param bufStart is the start position in the internal buffer. Use the
	 * values returned by append to calculate the start position.
	 * @param len is the length of the token.
	 */
	void mark(TokenId id, size_t bufStart, TokenLength len)
	{
		// Push the new instance back onto the list
		marks.emplace_back(id, bufStart, len);

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
	bool next(Token &token, WhitespaceMode mode,
	          const std::unordered_set<TokenId> &tokens, size_t &cursor)
	{
		// Sort the "marks" vector if it has not been sorted yet.
		if (!sorted) {
			std::sort(marks.begin(), marks.end());
			sorted = true;
		}

		// Fetch the next larger TokenMark instance, make sure the token is in
		// the "enabled" list
		auto it =
		    std::lower_bound(marks.begin(), marks.end(), TokenMark(cursor));
		while (it != marks.end() && tokens.count(it->id) == 0) {
			it++;
		}

		// Calculate the buffer start and end character, based on the returned
		// TokenMark instance
		const size_t end = (it != marks.end()) ? it->bufStart : buf.size();

		// Depending on the whitespace mode, fetch all the data between the
		// cursor position and the calculated end position and return a token
		// containing that data.
		if (cursor < end && cursor < buf.size()) {
			switch (mode) {
				case WhitespaceMode::PRESERVE: {
					token = Token(
					    Tokens::Data, std::string(&buf[cursor], end - cursor),
					    SourceLocation(sourceId,
					                   offsets.loadOffset(cursor).first,
					                   offsets.loadOffset(end).first));
					cursor = end;
					return true;
				}
				case WhitespaceMode::TRIM:
				case WhitespaceMode::COLLAPSE: {
					// Calculate the collapsed string and the corresponding
					// trimmed region
					size_t stringStart;
					size_t stringEnd;
					std::string content;
					if (mode == WhitespaceMode::TRIM) {
						content = Utils::trim(&buf[cursor], end - cursor,
						                      stringStart, stringEnd);
					} else {
						content = Utils::collapse(&buf[cursor], end - cursor,
						                          stringStart, stringEnd);
					}

					// If the resulting string is empty (only whitespaces),
					// abort
					if (content.empty()) {
						cursor = end;
						break;
					}

					// Calculate the absolute positions and return the token
					stringStart += cursor;
					stringEnd += cursor;
					token = Token(
					    Tokens::Data, content,
					    SourceLocation(sourceId,
					                   offsets.loadOffset(stringStart).first,
					                   offsets.loadOffset(stringEnd).first));
					cursor = end;
					return true;
				}
			}
		}

		// If start equals end, we're currently directly at a token
		// instance. Return this token and advance the cursor to the end of
		// the token.
		if (cursor == end && it != marks.end()) {
			const size_t tokenStart = it->bufStart;
			const size_t tokenEnd = it->bufStart + it->len;
			token = Token(
			    it->id, std::string(&buf[tokenStart], it->len),
			    SourceLocation(sourceId, offsets.loadOffset(tokenStart).first,
			                   offsets.loadOffset(tokenEnd).first));
			cursor = tokenEnd;
			return true;
		}

		// We've failed. There is no token. Only void. Reset token and return
		// false.
		token = Token();
		return false;
	}

	/**
	 * Returns the current size of the internal buffer.
	 *
	 * @return the size of the internal character buffer.
	 */
	size_t getSize() { return buf.size(); }
};

/* Class TokenizedData */

TokenizedData::TokenizedData() : TokenizedData(InvalidSourceId) {}

TokenizedData::TokenizedData(SourceId sourceId)
    : impl(std::make_shared<TokenizedDataImpl>(sourceId)), cursor(0)
{
}

TokenizedData::~TokenizedData() {}

size_t TokenizedData::append(const std::string &data, SourceOffset offsStart)
{
	return impl->append(data, offsStart);
}

size_t TokenizedData::append(char c, SourceOffset offsStart,
                             SourceOffset offsEnd)
{
	return impl->append(c, offsStart, offsEnd);
}

void TokenizedData::mark(TokenId id, TokenLength len)
{
	impl->mark(id, impl->getSize() - len, len);
}

void TokenizedData::mark(TokenId id, size_t bufStart, TokenLength len)
{
	impl->mark(id, bufStart, len);
}

bool TokenizedData::next(Token &token, WhitespaceMode mode)
{
	return impl->next(token, mode, tokens, cursor);
}

bool TokenizedData::text(Token &token, WhitespaceMode mode)
{
	// Copy the current cursor position to not update the actual cursor position
	// if the operation was not successful
	size_t cursorCopy = cursor;
	if (!impl->next(token, mode, tokens, cursorCopy) ||
	    token.id != Tokens::Data) {
		return false;
	}

	// There is indeed a text token, update the internal cursor position
	cursor = cursorCopy;
	return true;
}
}
