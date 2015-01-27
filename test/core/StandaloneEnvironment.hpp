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

#ifndef _OUSIA_STANDALONE_ENVIRONMENT_
#define _OUSIA_STANDALONE_ENVIRONMENT_

#include <memory>

#include <core/common/Logger.hpp>
#include <core/model/Project.hpp>
#include <core/parser/Parser.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/resource/ResourceManager.hpp>
#include <core/Registry.hpp>

namespace ousia {

struct StandaloneEnvironment {
	ConcreteLogger &logger;
	Manager manager;
	Registry registry;
	ResourceManager resourceManager;
	ParserScope scope;
	Rooted<Project> project;
	ParserContext context;

	StandaloneEnvironment(ConcreteLogger &logger)
	    : logger(logger), project(new Project(manager)),
	      context(registry, resourceManager, scope, project, logger)
	{
		logger.reset();
		logger.setSourceContextCallback(
		    resourceManager.getSourceContextCallback());
	}

	~StandaloneEnvironment()
	{
		logger.setSourceContextCallback(NullSourceContextCallback);
	}

	NodeVector<Node> parse(const std::string &path,
	                     const std::string mimetype, const std::string rel,
	                     const RttiSet &supportedTypes)
	{
		return context.import(path, mimetype, rel, supportedTypes);
	}
};
}

#endif /* _OUSIA_STANDALONE_ENVIRONMENT_ */

