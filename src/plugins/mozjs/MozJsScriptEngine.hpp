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

#include <core/script/ScriptEngine.hpp>
#include <core/script/Function.hpp>
#include <core/script/Object.hpp>

/* Forward declarations from header jsapi.h */

class JSRuntime;
class JSContext;
class JSCompartment;
class JSString;
class JSObject;

namespace JS {
class Value;
template <typename T>
class Rooted;
typedef Rooted<JSObject *> RootedObject;
typedef Rooted<Value> RootedValue;
}

namespace ousia {
namespace script {

class MozJsScriptEngineScope;

class MozJsScriptEngineFunction : public Function {
private:
	MozJsScriptEngineScope &scope;
	JS::RootedValue *fun;
	JS::RootedObject *parent;

public:
	MozJsScriptEngineFunction(MozJsScriptEngineScope &scope, JS::Value &fun, JSObject *parent);

	~MozJsScriptEngineFunction();

	MozJsScriptEngineFunction *clone() const override;

	Variant call(const std::vector<Variant> &args) const override;
};

class MozJsScriptEngineScope : public ScriptEngineScope {

friend MozJsScriptEngineFunction;

private:
	JSRuntime *rt;
	JSContext *cx;
	JSCompartment *oldCompartment;
	JS::RootedObject *global;

	void handleErr(bool ok = false);

	Variant arrayToVariant(JSObject *obj);

	Variant objectToVariant(JSObject *obj);

	Variant valueToVariant(JS::Value &val, JSObject *parent = nullptr);

	std::string toString(JS::Value &val);

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

