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

# Include required Boost components using the Boost cmake package
FIND_PACKAGE(Boost COMPONENTS system filesystem REQUIRED)

# Set utf8cpp include path
SET(UTF8CPP_INCLUDE_DIR "lib/utf8cpp")

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
	${Boost_INCLUDE_DIR}
	${UTF8CPP_INCLUDE_DIR}
)

# Link directories
LINK_DIRECTORIES(
	${MOZJS_LIBRARY_DIRS}
)

ADD_DEFINITIONS(
	${MOZJS_CFLAGS_OTHER}
)

ADD_LIBRARY(ousia_core
	src/core/CodeTokenizer
	src/core/CSS
	src/core/Registry
	src/core/Tokenizer
	src/core/XML
	src/core/common/Argument
	src/core/common/CharReader
	src/core/common/Exceptions
	src/core/common/Function
	src/core/common/Logger
	src/core/common/Number
	src/core/common/Property
	src/core/common/Rtti
	src/core/common/RttiBuilder
	src/core/common/Terminal
	src/core/common/Utils
	src/core/common/Variant
	src/core/common/VariantConverter
	src/core/common/VariantReader
	src/core/common/VariantWriter
	src/core/managed/Events
	src/core/managed/Managed
	src/core/managed/Manager
	src/core/model/Document
	src/core/model/Domain
	src/core/model/Index
	src/core/model/Node
	src/core/model/Project
	src/core/model/Typesystem
	src/core/parser/Parser
	src/core/parser/ParserStack
	src/core/parser/Scope
	src/core/resource/Resource
	src/core/resource/ResourceLocator
#	src/core/script/ScriptEngine
)

ADD_LIBRARY(ousia_boost
	src/plugins/boost/FileLocator
)

TARGET_LINK_LIBRARIES(ousia_boost
	ousia_core
	${Boost_LIBRARIES}
)

ADD_LIBRARY(ousia_css
	src/plugins/css/CSSParser
)

TARGET_LINK_LIBRARIES(ousia_css
	ousia_core
)

ADD_LIBRARY(ousia_html
	src/plugins/html/DemoOutput
)

TARGET_LINK_LIBRARIES(ousia_html
	ousia_core
)

ADD_LIBRARY(ousia_xml
	src/plugins/xml/XmlParser
)

TARGET_LINK_LIBRARIES(ousia_xml
	ousia_core
	${EXPAT_LIBRARIES}
)

#ADD_LIBRARY(ousia_mozjs
#	src/plugins/mozjs/MozJsScriptEngine
#)
#
#TARGET_LINK_LIBRARIES(ousia_mozjs
#	ousia_core
#	${MOZJS_LIBRARIES}
#)

# If testing is enabled, build the unit tests
IF(TEST)

	INCLUDE_DIRECTORIES(
		${GTEST_INCLUDE_DIRS}
		test/
	)

	ADD_EXECUTABLE(ousia_test_core
		test/core/CodeTokenizerTest
		test/core/CSSTest
		test/core/RangeSetTest
		test/core/RegistryTest
		test/core/TokenizerTest
		test/core/XMLTest
		test/core/common/ArgumentTest
		test/core/common/CharReaderTest
		test/core/common/FunctionTest
		test/core/common/LoggerTest
		test/core/common/PropertyTest
		test/core/common/RttiTest
		test/core/common/VariantReaderTest
		test/core/common/VariantWriterTest
		test/core/common/VariantTest
		test/core/common/UtilsTest
		test/core/managed/ManagedContainerTest
		test/core/managed/ManagedTest
		test/core/managed/ManagerTest
		test/core/managed/VariantObjectTest
		test/core/model/DomainTest
		test/core/model/DocumentTest
		test/core/model/IndexTest
		test/core/model/NodeTest
		test/core/model/TypesystemTest
		test/core/parser/ParserStackTest
		test/core/resource/ResourceLocatorTest
#		test/core/script/FunctionTest
#		test/core/script/ObjectTest
#		test/core/script/VariantTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_core
		${GTEST_LIBRARIES}
		ousia_core
	)

	ADD_EXECUTABLE(ousia_test_boost
		test/plugins/boost/FileLocatorTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_boost
		${GTEST_LIBRARIES}
		ousia_core
		ousia_boost
	)

	ADD_EXECUTABLE(ousia_test_css
		test/plugins/css/CSSParserTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_css
		${GTEST_LIBRARIES}
		ousia_core
		ousia_css
	)

	ADD_EXECUTABLE(ousia_test_html
		test/plugins/html/DemoOutputTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_html
		${GTEST_LIBRARIES}
		ousia_core
		ousia_html
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
	ADD_TEST(ousia_test_boost ousia_test_boost)
	ADD_TEST(ousia_test_css ousia_test_css)
	ADD_TEST(ousia_test_html ousia_test_html)
	ADD_TEST(ousia_test_xml ousia_test_xml)
#	ADD_TEST(ousia_test_mozjs ousia_test_mozjs)
ENDIF()

################################################################################
# Commands for building installing                                             #
################################################################################

INSTALL(DIRECTORY data/ DESTINATION share/ousia
		FILE_PERMISSIONS
				OWNER_READ GROUP_READ WORLD_READ
				OWNER_WRITE
		DIRECTORY_PERMISSIONS
				OWNER_READ GROUP_READ WORLD_READ
				OWNER_WRITE
				OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
)

