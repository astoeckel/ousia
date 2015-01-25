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
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/Registry.hpp>

namespace ousia {

struct StandaloneParserContext {
public:
	Manager manager;
	Logger logger;
	Registry registry;
	Rooted<Project> project;
	ParserScope scope;
	ParserContext context;

	StandaloneParserContext()
	    : project(new Project(manager, registry)),
	      context(project, scope, logger)
	{
	}

	StandaloneParserContext(Logger &externalLogger)
	    : project(new Project(manager, registry)),
	      context(project, scope, externalLogger)
	{
	}
};
}

#endif /* _OUSIA_STANDALONE_PARSER_CONTEXT_ */

