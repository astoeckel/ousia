# Ousía
# Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

################################################################################
# Basic Project Definitions and Dependencies                                   #
################################################################################

PROJECT(ousia)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

# Option for enabling testing. Turn on with 'cmake -DTEST=ON'.
# TODO: Automatically activate tests if gtest is available
OPTION(TEST "Build all tests." OFF) # Makes boolean 'test' available.

# Option for building the documentation.
FIND_PACKAGE(Doxygen)
OPTION(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

# Enable C++11 and all warnings
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic-errors -std=c++11")

# Include expat and mozjs-24 via PkgConfig
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(MOZJS REQUIRED mozjs-24)
PKG_CHECK_MODULES(EXPAT REQUIRED expat)

################################################################################
# Inclusion of doxygen                                                         #
################################################################################

IF(BUILD_DOCUMENTATION)
	IF(NOT DOXYGEN_FOUND)
		MESSAGE(FATAL_ERROR "Doxygen is needed to build the documentation.")
	ENDIF()

	SET(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
	SET(DOXYFILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

	CONFIGURE_FILE(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

	ADD_CUSTOM_TARGET(doc
		COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM)

	INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
ENDIF()


################################################################################
# Inclusion of gtest                                                           #
################################################################################

# Unit test handling
IF(TEST)
	# Compile gtest
	ADD_SUBDIRECTORY(lib/gtest-1.7.0)

	# Enable testing and declare some variables used for compiling the test cases
	ENABLE_TESTING()
	SET(GTEST_INCLUDE_DIRS ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
	SET(GTEST_LIBRARIES gtest gtest_main)
ENDIF()

################################################################################
# Commands for building Ousía                                                  #
################################################################################

# Include directories
INCLUDE_DIRECTORIES(
	src/
	${MOZJS_INCLUDE_DIRS}
	${EXPAT_INCLUDE_DIRS}
)

# Link directories
LINK_DIRECTORIES(
	${MOZJS_LIBRARY_DIRS}
)

ADD_DEFINITIONS(
	${MOZJS_CFLAGS_OTHER}
)

# ousia_script library (containing the bindings needed for script engines)
# TODO: This is not only the script library.
ADD_LIBRARY(ousia_core
	src/core/BufferedCharReader
	src/core/CodeTokenizer
	src/core/CSSParser
	src/core/Exceptions
	src/core/Logger
	src/core/Managed
	src/core/Node
	src/core/Tokenizer
#	src/core/Typesystem
	src/core/Utils
	src/core/parser/Parser
	src/core/parser/ParserStack
	src/core/parser/Scope
#	src/core/script/Function
#	src/core/script/Object
#	src/core/script/ScriptEngine
#	src/core/script/Variant
	src/core/variant/Variant
)

# ousia_xml library

ADD_LIBRARY(ousia_xml
	src/plugins/xml/XmlParser
)

TARGET_LINK_LIBRARIES(ousia_xml
	ousia_core
	${EXPAT_LIBRARIES}
)

# ousia_mozjs library

ADD_LIBRARY(ousia_mozjs
	src/plugins/mozjs/MozJsScriptEngine
)

TARGET_LINK_LIBRARIES(ousia_mozjs
	ousia_core
	${MOZJS_LIBRARIES}
)

# If testing is enabled, build the unit tests
IF(TEST)

	INCLUDE_DIRECTORIES(
		${GTEST_INCLUDE_DIRS}
	)

	ADD_EXECUTABLE(ousia_test_core
		test/core/BufferedCharReaderTest
		test/core/CodeTokenizerTest
		test/core/CSSParserTest
		test/core/LoggerTest
		test/core/ManagedTest
		test/core/ManagedContainersTest
		test/core/NodeTest
		test/core/RangeSetTest
		test/core/TokenizerTest
		test/core/UtilsTest
		test/core/parser/ParserStackTest
#		test/core/script/FunctionTest
#		test/core/script/ObjectTest
#		test/core/script/VariantTest
		test/core/variant/VariantTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_core
		${GTEST_LIBRARIES}
		ousia_core
	)

	ADD_EXECUTABLE(ousia_test_xml
		test/plugins/xml/XmlParserTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_xml
		${GTEST_LIBRARIES}
		ousia_core
		ousia_xml
	)

#	ADD_EXECUTABLE(ousia_test_mozjs
#		test/plugins/mozjs/MozJsScriptEngineTest
#	)

#	TARGET_LINK_LIBRARIES(ousia_test_mozjs
#		${GTEST_LIBRARIES}
#		ousia_core
#		ousia_mozjs
#	)

	# Register the unit tests
	ADD_TEST(ousia_test_core ousia_test_core)
	ADD_TEST(ousia_test_xml ousia_test_xml)
#	ADD_TEST(ousia_test_mozjs ousia_test_mozjs)
ENDIF()

