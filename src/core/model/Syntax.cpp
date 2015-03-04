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
