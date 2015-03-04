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

#include "DocumentHandler.hpp"
#include "OntologyHandler.hpp"
#include "GenericParserStates.hpp"
#include "ImportIncludeHandler.hpp"
#include "TypesystemHandler.hpp"

namespace ousia {
namespace parser_stack {

const std::multimap<std::string, const State *> GenericParserStates{
    {"document", &States::Document},
    {"*", &States::DocumentChild},
    {"ontology", &States::Ontology},
    {"struct", &States::OntologyStruct},
    {"annotation", &States::OntologyAnnotation},
    {"attributes", &States::OntologyAttributes},
    {"attribute", &States::OntologyAttribute},
    {"field", &States::OntologyField},
    {"fieldRef", &States::OntologyFieldRef},
    {"primitive", &States::OntologyStructPrimitive},
    {"childRef", &States::OntologyStructChild},
    {"parentRef", &States::OntologyStructParent},
    {"field", &States::OntologyStructParentField},
    {"fieldRef", &States::OntologyStructParentFieldRef},
    {"syntax", &States::OntologySyntax},
    {"open", &States::OntologySyntaxOpen},
    {"close", &States::OntologySyntaxClose},
    {"short", &States::OntologySyntaxShort},
    {"whitespace", &States::OntologySyntaxWhitespace},
    {"*", &States::OntologySyntaxToken},
    {"typesystem", &States::Typesystem},
    {"enum", &States::TypesystemEnum},
    {"entry", &States::TypesystemEnumEntry},
    {"struct", &States::TypesystemStruct},
    {"field", &States::TypesystemStructField},
    {"constant", &States::TypesystemConstant},
    {"import", &States::Import},
    {"include", &States::Include}
};
}
}

