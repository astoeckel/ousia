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

#ifndef _OUSIA_SCRIPT_ENGINE_HPP_
#define _OUSIA_SCRIPT_ENGINE_HPP_

#include <map>
#include <string>
#include <exception>

#include <core/Utils.hpp>

#include "Variant.hpp"

// TODO: Provide more Exception classes than ScriptEngineException -- one for
// internal errors, one for script errors

// TODO: Allow reporting multiple exceptions (e.g. to report all syntax errors
// at once)

// TODO: Add API that allow pre-compilation of scripts

namespace ousia {
namespace script {

/**
 * Class used for signaling errors while executing code or registering variables
 * in the script engine.
 */
class ScriptEngineException : public std::exception {
public:
	/**
	 * Line and column at which the exception occured. Set to -1 if the error
	 * does not correspond to a line or column.
	 */
	const int line, col;

	/**
	 * Error message.
	 */
	const std::string msg;

	/**
	 * ScriptEngineException constructor.
	 *
	 * @param line in the script code at which the exception occured.
	 * @param col in the script code at which the exception occured.
	 * @param msg is the message containing the reason for the exception.
	 */
	ScriptEngineException(int line, int col, const std::string &msg);

	/**
	 * ScriptEngineException constructor.
	 *
	 * @param msg is the message containing the reason for the exception.
	 */
	ScriptEngineException(const std::string &msg);

	/**
	 * Returns the error message.
	 */
	virtual const char *what() const noexcept override;
};

/**
 * The ScriptEngineScope class represents an execution scope -- an execution
 * scope is the base class
 */
class ScriptEngineScope {
private:
	/**
	 * Helper used to check the given identifiers for their validity.
	 *
	 * @param name is the name of the identifier that should be checked.
	 * @throws ScriptEngineException if the given identifier is invalid.
	 */
	static void checkIdentifier(const std::string &name)
	{
		if (!Utils::isIdentifier(name)) {
			throw ScriptEngineException{"Invalid identifier \"" + name + "\""};
		}
	}

protected:
	/**
	 * Implementation of the run function.
	 */
	virtual Variant doRun(const std::string &code) = 0;

	/**
	 * Implementation of the setVariable function.
	 */
	virtual void doSetVariable(const std::string &name, const Variant &val,
	                           bool constant) = 0;

	/**
	 * Implementation of the getVariable function.
	 */
	virtual Variant doGetVariable(const std::string &name) = 0;

public:
	/**
	 * Virtual destructor. Must be overwritten by implementing classes.
	 */
	virtual ~ScriptEngineScope(){};

	/**
	 * Runs the given code in the excution context.
	 *
	 * @param code is a string containing the code the script engine should run.
	 * @return a variant containg the result of the executed code.
	 * @throws ScriptEngineException if an error occured during code execution.
	 */
	Variant run(const std::string &code) { return doRun(code); }

	/**
	 * Sets the value of a variable in the scope with the given name.
	 *
	 * @param name is the name of the variable in the scope. Must be a
	 * well-formed identifier.
	 * @param val is the value of the variable.
	 * @param constant if true, the value of the variable cannot be changed by
	 * the script code.
	 * @throws ScriptEngineException if name is not a well-formed identifier.
	 */
	void setVariable(const std::string &name, const Variant &val,
	                 bool constant = false)
	{
		checkIdentifier(name);
		doSetVariable(name, val, constant);
	}

	/**
	 * Reads the value of the variable with the given name.
	 *
	 * @param name is the name of the variable. The name must be well-formed.
	 * @return the value of the variable, or a NULL variant if the variable does
	 * not exist.
	 * @throws ScriptEngineException if name is not a well-formed identifier.
	 */
	Variant getVariable(const std::string &name)
	{
		checkIdentifier(name);
		return doGetVariable(name);
	}
};

/**
 * The abstract ScriptEngine class is used to provide an interface for script
 * engine implementations. A script engine implementation has to provide a
 * function which creates an execution scope.
 */
class ScriptEngine {
public:
	/**
	 * Requests an execution scope from the script engine implementation. The
	 * calling code is responsible for disposing the returned pointer.
	 */
	virtual ScriptEngineScope *createScope() = 0;
};

/**
 * The ScriptEngineFactory class is a central registry for ScriptEngine
 * instances and factory of ScriptEngineScope instances for a certain scripting
 * language.
 */
class ScriptEngineFactory {
private:
	/**
	 * Internal map between the script language name and the actual script
	 * engine instance.
	 */
	std::map<std::string, ScriptEngine *> registry;

public:
	/**
	 * Registers a ScriptEngine instance for a new scripting language.
	 *
	 * @param name is the name of the scripting language as MIME, e.g.
	 * "text/javascript"
	 * @param engine is the backend that should be registered.
	 */
	void registerScriptEngine(const std::string &name, ScriptEngine *engine);

	/**
	 * Removes a script engine from the registry.
	 *
	 * @param name is the name of the script engine that
	 */
	bool unregisterScriptEngine(const std::string &name);

	/**
	 * Creates an execution scope for the scripting language with the given
	 * name.
	 *
	 * @param name is the name of the scripting language for which the scope
	 * is being created.
	 * @return a pointer to the new execution scope or null if a script engine
	 * with the given name does not exist. The caller of this function is
	 * responsible
	 */
	ScriptEngineScope *createScope(const std::string &name) const;
};
}
}

#endif /* _OUSIA_SCRIPT_ENGINE_HPP_ */

