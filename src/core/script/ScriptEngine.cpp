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

#include "ScriptEngine.hpp"

namespace ousia {
namespace script {

ScriptEngineException::ScriptEngineException(int line, int col,
		const std::string &msg) :
	line(line), col(col),
	msg(std::to_string(line) + ":" + std::to_string(col) + " " + msg) {}

ScriptEngineException::ScriptEngineException(const std::string &msg) :
	line(-1), col(-1), msg(msg) {}

const char* ScriptEngineException::what() const noexcept
{
	return msg.c_str();
}

void ScriptEngineFactory::registerScriptEngine(const std::string &name,
		ScriptEngine *engine)
{
	registry[name] = engine;
}

bool ScriptEngineFactory::unregisterScriptEngine(const std::string &name)
{
	return registry.erase(name) > 0;
}

/* Class ScriptEngineFactory */

ScriptEngineScope* ScriptEngineFactory::createScope(
		const std::string &name) const
{
	auto it = registry.find(name);
	if (it != registry.end()) {
		return it->second->createScope();
	}
	return nullptr;
}


}
}

