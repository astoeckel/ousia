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

#include <core/resource/ResourceManager.hpp>
#include <core/Registry.hpp>

#include "ParserScope.hpp"
#include "ParserContext.hpp"

namespace ousia {

/* Class ParserContext */

ParserContext::ParserContext(Registry &registry,
                             ResourceManager &resourceManager,
                             ParserScope &scope, Handle<Project> project,
                             Logger &logger, SourceId sourceId)
    : registry(registry),
      resourceManager(resourceManager),
      scope(scope),
      project(project),
      logger(logger),
      sourceId(sourceId)
{
}

Rooted<Node> ParserContext::link(const std::string &path,
                                 const std::string mimetype,
                                 const std::string rel,
                                 const RttiSet &supportedTypes)
{
	return resourceManager.link(*this, path, mimetype, rel, supportedTypes);
}

Rooted<Node> ParserContext::include(const std::string &path,
                                    const std::string mimetype,
                                    const std::string rel,
                                    const RttiSet &supportedTypes)
{
	return resourceManager.include(*this, path, mimetype, rel, supportedTypes);
}

ParserContext ParserContext::clone(ParserScope &scope, SourceId sourceId) const
{
	return ParserContext{registry, resourceManager, scope,
	                     project,  logger,          sourceId};
}

Manager &ParserContext::getManager() const { return project->getManager(); }
}

