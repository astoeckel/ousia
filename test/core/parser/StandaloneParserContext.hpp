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

#ifndef _OUSIA_STANDALONE_PARSER_CONTEXT_
#define _OUSIA_STANDALONE_PARSER_CONTEXT_

#include <memory>

#include <core/model/Project.hpp>
#include <core/parser/Parser.hpp>

namespace ousia {
namespace parser {

struct StandaloneParserContext {
public:
	Manager manager;
	Logger logger;
	Scope scope;
	Registry registry;
	Rooted<model::Project> project;
	ParserContext context;

	StandaloneParserContext()
	    : project(new model::Project(manager)),
	      context(scope, registry, logger, manager, project)
	{
	}

	StandaloneParserContext(Logger &externalLogger)
	    : project(new model::Project(manager)),
	      context(scope, registry, externalLogger, manager, project){};
};
}
}

#endif /* _OUSIA_STANDALONE_PARSER_CONTEXT_ */

