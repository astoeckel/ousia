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

#include <sstream>
#include <memory>

#include <gtest/gtest.h>

#include <plugins/mozjs/MozJsScriptEngine.hpp>

namespace ousia {
namespace script {
namespace mozjs {

/* Global engine object */
MozJsScriptEngine engine;
auto scope = std::unique_ptr<MozJsScriptEngineScope>{engine.createScope()};

#define GENERIC_JS_TEST_NAME MozJsScriptEngine
#define GENERIC_JS_TEST_SCOPE

#include "../genericjs/GenericJsScriptEngineTest.hpp"

}
}
}

