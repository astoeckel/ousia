/*
    Ousía
    Copyright (C) 2014, 2015  Benjamin Paaßen, Andreas Stöckel

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
 * @file Syntax.hpp
 *
 * This header contains the Descriptor classes for user definable syntax for
 * Document entities or fields. These classes are referenced in Ontology.hpp.
 */

#ifndef _OUSIA_MODEL_SYNTAX_HPP_
#define _OUSIA_MODEL_SYNTAX_HPP_

#include <core/common/Token.hpp>
#include "Node.hpp"

namespace ousia {

/**
 * Class to describe a single token that shall be used as user-defined syntax.
 */
struct TokenDescriptor {
	/**
	 * The string content of this token, if it is not a special one.
	 */
	std::string token;
	/**
	 * A flag to be set true if this TokenDescriptor uses a special token.
	 */
	bool special;
	/**
	 * An id to uniquely identify this token.
	 */
	TokenId id;

	/**
	 * Constructor for non-special tokens. The special flag is set to false and
	 * the id to Tokens::Empty.
	 *
	 * @param token The string content of this token, if it is not a special
	 *              one.
	 */
	TokenDescriptor(std::string token = std::string())
	    : token(std::move(token)), special(false), id(Tokens::Empty)
	{
	}

	/**
	 * Constructor for special tokens. The token is set to an empty string and
	 * the special flag to true.
	 *
	 * @param id the id of the special token.
	 */
	TokenDescriptor(TokenId id) : special(true), id(id) {}

	/**
	 * Returns true if and only if neither a string nor an ID is given.
	 *
	 * @return true if and only if neither a string nor an ID is given.
	 */
	bool isEmpty() const { return token.empty() && id == Tokens::Empty; }
};

/**
 * Class describing the user defined syntax for a StructuredClass,
 * AnnotationClass or FieldDescriptor.
 *
 * This class is used during parsing of a Document. It is used to describe
 * the tokens relevant for one Descriptor that could be created at this point
 * during parsing.
 */
struct SyntaxDescriptor {
	/**
	 * Possible start token or Tokens::Empty if no token is set.
	 */
	TokenId start;

	/**
	 * Possible end token or Tokens::Empty if no token is set.
	 */
	TokenId end;

	/**
	 * Possible representation token or Tokens::Empty if no token is set.
	 */
	TokenId shortForm;

	/*
	 * The Descriptor this SyntaxDescriptor belongs to. As this may be
	 * a FieldDescriptor as well as a class Descriptor (StructuredClass or
	 * AnnotationClass) we can only use the class Node as inner argument here.
	 */
	Rooted<Node> descriptor;
	/*
	 * Given the current leaf in the parsed document the depth of a
	 * SyntaxDescriptor is defined as the number of transparent elements that
	 * would be needed to construct an instance of the referenced descriptor.
	 */
	ssize_t depth;

	/**
	 * Default constructor, sets all token ids to Tokens::Empty and the
	 * descriptor handle to nullptr.
	 */
	SyntaxDescriptor()
	    : start(Tokens::Empty),
	      end(Tokens::Empty),
	      shortForm(Tokens::Empty),
	      descriptor(nullptr),
	      depth(-1)
	{
	}

	/**
	 * Member initializer constructor.
	 *
	 * @param start is a possible start token.
	 * @param end is a possible end token.
	 * @param shortForm is a possible short form token.
	 * @param descriptor The Descriptor this SyntaxDescriptor belongs to.
	 * @param depth Given the current leaf in the parsed document the depth of a
	 * SyntaxDescriptor is defined as the number of transparent elements that
	 * would be needed to construct an instance of the referenced descriptor.
	 */
	SyntaxDescriptor(TokenId start, TokenId end, TokenId shortForm,
	                 Handle<Node> descriptor, ssize_t depth)
	    : start(start),
	      end(end),
	      shortForm(shortForm),
	      descriptor(descriptor),
	      depth(depth)
	{
	}

	/**
	 * Inserts all tokens referenced in this SyntaxDescriptor into the
	 * given TokenSet. Skips token ids set to Tokens::Empty.
	 *
	 * @param set is the TokenSet instance into which the Tokens should be
	 * inserted.
	 */
	void insertIntoTokenSet(TokenSet &set) const;

	/**
	 * Returns true if and only if this SyntaxDescriptor belongs to an
	 * AnnotationClass.
	 *
	 * @return true if and only if this SyntaxDescriptor belongs to an
	 * AnnotationClass.
	 */
	bool isAnnotation() const;

	/**
	 * Returns true if and only if this SyntaxDescriptor belongs to a
	 * StrcturedClass.
	 *
	 * @return true if and only if this SyntaxDescriptor belongs to a
	 * StrcturedClass.
	 */
	bool isStruct() const;

	/**
	 * Returns true if and only if this SyntaxDescriptor belongs to a
	 * FieldDescriptor.
	 *
	 * @return true if and only if this SyntaxDescriptor belongs to a
	 * FieldDescriptor.
	 */
	bool isFieldDescriptor() const;

	/**
	 * Returns true if and only if this SyntaxDescriptor has only empty
	 * entries in start, end and short.
	 *
	 * @return true if and only if this SyntaxDescriptor has only empty
	 * entries in start, end and short.
	 */
	bool isEmpty() const;
};
}
#endif