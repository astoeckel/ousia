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

#include "ParserContext.hpp"

namespace ousia {

/* Class ParserContext */

ParserContext::ParserContext(ParserScope &scope, Registry &registry,
                             Logger &logger, Manager &manager,
                             Handle<model::Project> project)
    : scope(scope),
      registry(registry),
      logger(logger),
      manager(manager),
      project(project)
{
}
}

