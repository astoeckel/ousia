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

#include "MozJsScriptEngine.hpp"

namespace ousia {
namespace script {

/*
 * Some important links to the SpiderMonkey (mozjs) documentation:
 *
 * Documentation overview:
 * https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/
 *
 * User Guide:
 * https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/JSAPI_User_Guide
 *
 * API Reference:
 * https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/JSAPI_reference
 */

/* Constants */

static const uint32_t MOZJS_RT_MEMSIZE = 64L * 1024L * 1024L;
static const uint32_t MOZJS_CTX_STACK_CHUNK_SIZE = 8192;

/* Class MozJsScriptEngineScope */

/**
 * The class of the global object.
 */
static JSClass globalClass = {
    "global",              JSCLASS_GLOBAL_FLAGS, JS_PropertyStub,
    JS_DeletePropertyStub, JS_PropertyStub,      JS_StrictPropertyStub,
    JS_EnumerateStub,      JS_ResolveStub,       JS_ConvertStub,
    nullptr,               nullptr,              nullptr,
    nullptr,               nullptr};

MozJsScriptEngineScope::MozJsScriptEngineScope(JSRuntime *rt) : rt(rt)
{
	// Create the execution context
	cx = JS_NewContext(rt, MOZJS_CTX_STACK_CHUNK_SIZE);
	if (!cx) {
		throw ScriptEngineException{"MozJs JS_NewContext failed"};
	}

	// Start a context request
	JS_BeginRequest(cx);

	// Set some context options
	JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_EXTRA_WARNINGS |
	                      JSOPTION_VAROBJFIX | JSOPTION_DONT_REPORT_UNCAUGHT);

	// Create the rooted global object
	global =
	    new JS::RootedObject(cx, JS_NewGlobalObject(cx, &globalClass, nullptr));

	// Enter a compartment (heap memory region) surrounding the global object
	oldCompartment = JS_EnterCompartment(cx, *global);

	// Populate the global object with the standard classes
	if (!JS_InitStandardClasses(cx, *global)) {
		throw ScriptEngineException{"MozJS JS_InitStandardClasses failed"};
	}
}

MozJsScriptEngineScope::~MozJsScriptEngineScope()
{
	// Leave the compartment
	JS_LeaveCompartment(cx, oldCompartment);

	// Free the reference to the local object
	delete global;

	// End the request
	JS_EndRequest(cx);

	// Destroy the execution context
	JS_DestroyContext(cx);
}

Variant MozJsScriptEngineScope::toVariant(const JS::Value &val)
{
	if (val.isBoolean()) {
		return Variant{val.toBoolean()};
	}
	if (val.isInt32()) {
		return Variant{(int64_t)val.toInt32()};
	}
	if (val.isDouble()) {
		return Variant{val.toDouble()};
	}
	if (val.isString()) {
		// TODO: Remove the need for using "c_str"!
		return Variant{toString(val.toString()).c_str()};
	}
	return Variant::Null;
}

void MozJsScriptEngineScope::handleErr(bool ok)
{
	if (!ok && JS_IsExceptionPending(cx)) {
		JS::Value exception;
		if (JS_GetPendingException(cx, &exception)) {
			// Fetch messgage string, line and column
			JS::Value msg, line, col;
			JS_GetPendingException(cx, &exception);
			JS_GetProperty(cx, JSVAL_TO_OBJECT(exception), "message", &msg);
			JS_GetProperty(cx, JSVAL_TO_OBJECT(exception), "lineNumber", &line);
			JS_GetProperty(cx, JSVAL_TO_OBJECT(exception), "columnNumber",
			               &col);

			// Clear the exception
			JS_ClearPendingException(cx);

			// Produce a nice error message in case the caught exception is of
			// the "Error" class
			if (msg.isString() && line.isInt32() && col.isInt32()) {
				// Throw a script engine exception with the corresponding line,
				// column and string
				throw ScriptEngineException{line.toInt32(), col.toInt32(),
				                            toString(msg)};
			}

			// Otherwise simply convert the exception to a string
			throw ScriptEngineException{toString(exception)};
		}
	}
}

std::string MozJsScriptEngineScope::toString(const JS::Value &val)
{
	// If the given value already is a Javascript string, return it directly.
	if (val.isString()) {
		return toString(val.toString());
	}

	// The given value is not really a string, so convert it to one first
	JSString *str = JS_ValueToString(cx, val);
	if (!str) {
		throw ScriptEngineException{"Cannot convert value to string"};
	}

	return toString(str);
}

std::string MozJsScriptEngineScope::toString(JSString *str)
{
	// Encode the string
	char *buf = JS_EncodeStringToUTF8(cx, str);
	if (!buf) {
		throw ScriptEngineException{"JS_EncodeStringToUTF8 failed"};
	}

	// Copy the string into a std::string, free the original buffer and return
	std::string res{buf};
	JS_free(cx, buf);
	return res;
}

Variant MozJsScriptEngineScope::doRun(const std::string &code)
{
	JS::Value rval;
	handleErr(JS_EvaluateScript(cx, *global, code.c_str(), code.length(), "", 0,
	                            &rval));
	return toVariant(rval);
}

void MozJsScriptEngineScope::doSetVariable(const std::string &name,
                                           const Variant &val, bool constant)
{
	// TODO
}

Variant MozJsScriptEngineScope::doGetVariable(const std::string &name)
{
	// TODO
	return Variant::Null;
}

/* Class MozJsScriptEngine */

MozJsScriptEngine::MozJsScriptEngine()
{
	rt = JS_NewRuntime(MOZJS_RT_MEMSIZE, JS_NO_HELPER_THREADS);
	if (!rt) {
		throw ScriptEngineException{"MozJs JS_NewRuntime failed"};
	}
}

MozJsScriptEngine::~MozJsScriptEngine()
{
	JS_DestroyRuntime(rt);
	JS_ShutDown();
}

MozJsScriptEngineScope *MozJsScriptEngine::createScope()
{
	return new MozJsScriptEngineScope(rt);
}
}
}

