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
 * @file ParserScopeCallbacks.hpp
 *
 * Contains the definition of some callback function types used in the context
 * of the resolution process for nodes in the document graph.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_RESOLUTION_CALLBACKS_HPP_
#define _OUSIA_RESOLUTION_CALLBACKS_HPP_

#include <functional>

#include <core/model/Node.hpp>

namespace ousia {

// Forward declarations
class Logger;

/**
 * Callback function type used for creating a dummy object while no correct
 * object is available for resolution.
 */
using ResolutionImposterCallback = std::function<Rooted<Node>()>;

/**
 * Callback function type called whenever the result of a resolution is
 * available.
 *
 * @param resolved is the new, resolved node.
 * @param owner is the node that was passed as "owner".
 * @param logger is the logger to which errors should be logged.
 */
using ResolutionResultCallback = std::function<
    void(Handle<Node> resolved, Handle<Node> owner, Logger &logger)>;

/**
 * The ResolveCallback can be used to trigger the resolution of a certain node.
 *
 * @param async if true, the resolution may be deferred. In this case the
 * resultCallback may be called at any later point in the program.
 * @param type is the type of node that should be resolved.
 * @param path is the path for which a node should be resolved.
 * @param resultCallback is the callback function to which the result of
 * the resolution process is passed. This function is called once the
 * resolution was successful.
 * @return true if the resolution was immediately successful. This does not
 * mean, that the resolved object does not exist, as it may be resolved
 * later.
 */
using ResolveCallback = std::function<
    bool(bool async, const Rtti *type, const std::vector<std::string> &path,
         ResolutionResultCallback resultCallback)>;
}

#endif /* _OUSIA_RESOLUTION_CALLBACKS_HPP_ */
