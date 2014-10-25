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

#ifndef _MOZ_JS_SCRIPT_ENGINE_HPP_
#define _MOZ_JS_SCRIPT_ENGINE_HPP_

#include <jsapi.h>

#include <core/script/ScriptEngine.hpp>

namespace ousia {
namespace script {

class MozJsScriptEngineScope : public ScriptEngineScope {

private:
	JSRuntime *rt;
	JSContext *cx;
	JSCompartment *oldCompartment;
	JS::RootedObject *global;

	void handleErr(bool ok);

	Variant toVariant(const JS::Value &val);

	std::string toString(const JS::Value &val);

	std::string toString(JSString *str);

protected:
	Variant doRun(const std::string &code) override;
	void doSetVariable(const std::string &name, const Variant &val,
	                   bool constant) override;
	Variant doGetVariable(const std::string &name) override;

public:
	MozJsScriptEngineScope(JSRuntime *rt);

	~MozJsScriptEngineScope() override;

};

class MozJsScriptEngine : public ScriptEngine {

private:
	JSRuntime *rt;

public:

	MozJsScriptEngine();

	~MozJsScriptEngine();

	MozJsScriptEngineScope *createScope() override;

};
}
}

#endif /* _MOZ_JS_SCRIPT_ENGINE_HPP_ */

