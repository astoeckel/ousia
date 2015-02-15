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
#include "DomainHandler.hpp"
#include "GenericParserStates.hpp"
#include "ImportIncludeHandler.hpp"
#include "TypesystemHandler.hpp"

namespace ousia {
namespace parser_stack {

const std::multimap<std::string, const State *> GenericParserStates{
    {"document", &States::Document},
    {"*", &States::DocumentChild},
    {"domain", &States::Domain},
    {"struct", &States::DomainStruct},
    {"annotation", &States::DomainAnnotation},
    {"attributes", &States::DomainAttributes},
    {"attribute", &States::DomainAttribute},
    {"field", &States::DomainField},
    {"fieldRef", &States::DomainFieldRef},
    {"primitive", &States::DomainStructPrimitive},
    {"childRef", &States::DomainStructChild},
    {"parentRef", &States::DomainStructParent},
    {"field", &States::DomainStructParentField},
    {"fieldRef", &States::DomainStructParentFieldRef},
    {"typesystem", &States::Typesystem},
    {"enum", &States::TypesystemEnum},
    {"entry", &States::TypesystemEnumEntry},
    {"struct", &States::TypesystemStruct},
    {"field", &States::TypesystemStructField},
    {"constant", &States::TypesystemConstant},
    {"import", &States::Import},
    {"include", &States::Include}};
}
}

