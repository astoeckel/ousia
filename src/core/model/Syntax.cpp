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

#include <core/common/Utils.hpp>

#include "Ontology.hpp"
#include "Syntax.hpp"

namespace ousia {

/* Class TokenDescriptor */

bool TokenDescriptor::isValid() const
{
	return special || isEmpty() || Utils::isUserDefinedToken(token);
}

/* Class SyntaxDescriptor */

bool operator==(const SyntaxDescriptor &o1, const SyntaxDescriptor &o2)
{
	return (o1.depth == o2.depth) && (o1.open == o2.open) &&
	       (o1.close == o2.close) && (o1.shortForm == o2.shortForm) &&
	       (o1.descriptor == o2.descriptor);
}

bool operator<(const SyntaxDescriptor &o1, const SyntaxDescriptor &o2)
{
#define LTOP(X1, X2, OTHER) ((X1 != X2) ? ((X1 < X2) ? true : false) : OTHER)
	return LTOP(
	    o1.depth, o2.depth,
	    LTOP(o1.open, o2.open,
	         LTOP(o1.close, o2.close, LTOP(o1.shortForm, o2.shortForm,
	                                       LTOP(o1.descriptor.get(),
	                                            o2.descriptor.get(), false)))));
}

bool SyntaxDescriptor::isAnnotation() const
{
	return descriptor->isa(&RttiTypes::AnnotationClass);
}
bool SyntaxDescriptor::isFieldDescriptor() const
{
	return descriptor->isa(&RttiTypes::FieldDescriptor);
}
bool SyntaxDescriptor::isStruct() const
{
	return descriptor->isa(&RttiTypes::StructuredClass);
}

void SyntaxDescriptor::insertIntoTokenSet(TokenSet &set) const
{
	if (open != Tokens::Empty) {
		set.insert(open);
	}
	if (close != Tokens::Empty) {
		set.insert(close);
	}
	if (shortForm != Tokens::Empty) {
		set.insert(shortForm);
	}
}

bool SyntaxDescriptor::isEmpty() const
{
	return open == Tokens::Empty && close == Tokens::Empty &&
	       shortForm == Tokens::Empty;
}
}
