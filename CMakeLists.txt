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
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.7)

# Option for enabling testing. Turn on with 'cmake -DTEST=ON'. Needs gtest to
# be available in the lib/ folder. Download using the "download_dependencies.sh"
# script.
OPTION(TEST "Build all tests." OFF)
IF (EXISTS lib/gtest-1.7.0)
	SET(TEST ON)
ENDIF()

# Set to ON to install the GtkSourceview highlighter for osml
OPTION(INSTALL_GEDIT_HIGHLIGHTER
	"Installs the GtkSourceview 3.0 highlighter for OSML, enables highlighting in gedit"
	OFF)

# Option for building the documentation.
FIND_PACKAGE(Doxygen)
OPTION(BUILD_DOCUMENTATION
	"Create and install the HTML based API documentation (requires Doxygen)"
	${DOXYGEN_FOUND})

# Enable C++11 and all warnings
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic-errors -std=c++11")

# Flag for enabling the generation of data for gcov
OPTION(ENABLE_GCOV "Enables the compiler flags needed for gcov/lcov" OFF)
IF (ENABLE_GCOV)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
ENDIF()

# Include expat and mozjs-24 via PkgConfig
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(MOZJS REQUIRED mozjs-24)
PKG_CHECK_MODULES(EXPAT REQUIRED expat)

# Include required Boost components using the Boost cmake package
FIND_PACKAGE(Boost COMPONENTS system filesystem program_options REQUIRED)

# Set utf8cpp include path
SET(UTF8CPP_INCLUDE_DIR "lib/utf8cpp")

################################################################################
# Check the gcc version                                                        #
################################################################################

IF(CMAKE_COMPILER_IS_GNUCC)
	EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion
		OUTPUT_VARIABLE GCC_VERSION)
	IF(GCC_VERSION VERSION_LESS 4.8)
		ERROR("This projects requires at last GCC 4.8 or newer to be built")
	ENDIF()
ENDIF()

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
# Commands for assembling and including the configuration file                 #
################################################################################

CONFIGURE_FILE(
	"${PROJECT_SOURCE_DIR}/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
)

# Adapt the include path for finding the configuration file
INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")

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
	src/core/Registry
	src/core/XML
	src/core/RangeSet
	src/core/common/Argument
	src/core/common/CharReader
	src/core/common/Exceptions
	src/core/common/Function
	src/core/common/Logger
	src/core/common/Location
	src/core/common/Number
	src/core/common/Property
	src/core/common/Rtti
	src/core/common/RttiBuilder
	src/core/common/SourceContextReader
	src/core/common/Token
	src/core/common/Utils
	src/core/common/Variant
	src/core/common/VariantConverter
	src/core/common/VariantReader
	src/core/common/VariantWriter
	src/core/frontend/Terminal
	src/core/frontend/TerminalLogger
	src/core/managed/Events
	src/core/managed/Managed
	src/core/managed/Manager
	src/core/model/Document
	src/core/model/Domain
	src/core/model/Index
	src/core/model/Node
	src/core/model/Project
	src/core/model/RootNode
	src/core/model/Style
	src/core/model/Typesystem
	src/core/parser/Parser
	src/core/parser/ParserContext
	src/core/parser/ParserScope
	src/core/parser/stack/Callbacks
#	src/core/parser/stack/DocumentHandler
#	src/core/parser/stack/DomainHandler
#	src/core/parser/stack/GenericParserStates
#	src/core/parser/stack/Handler
#	src/core/parser/stack/ImportIncludeHandler
	src/core/parser/stack/State
#	src/core/parser/stack/Stack
	src/core/parser/stack/TokenRegistry
#	src/core/parser/stack/TypesystemHandler
	src/core/parser/utils/SourceOffsetVector
	src/core/parser/utils/TokenizedData
	src/core/parser/utils/Tokenizer
	src/core/parser/utils/TokenTrie
	src/core/resource/Resource
	src/core/resource/ResourceLocator
	src/core/resource/ResourceManager
	src/core/resource/ResourceRequest
#	src/core/script/ScriptEngine
)

# Format libraries

#ADD_LIBRARY(ousia_css
#	src/plugins/css/CodeTokenizer
#	src/plugins/css/Tokenizer
#	src/plugins/css/CSSParser
#)

#TARGET_LINK_LIBRARIES(ousia_css
#	ousia_core
#)

ADD_LIBRARY(ousia_osml
#	src/formats/osml/OsmlParser
	src/formats/osml/OsmlStreamParser
)

TARGET_LINK_LIBRARIES(ousia_osml
	ousia_core
)

ADD_LIBRARY(ousia_osxml
	src/formats/osxml/OsxmlAttributeLocator
	src/formats/osxml/OsxmlEventParser
	src/formats/osxml/OsxmlParser
)

TARGET_LINK_LIBRARIES(ousia_osxml
	ousia_core
	${EXPAT_LIBRARIES}
)

# Resource locators

ADD_LIBRARY(ousia_filesystem
	src/plugins/filesystem/FileLocator
	src/plugins/filesystem/SpecialPaths
)

TARGET_LINK_LIBRARIES(ousia_filesystem
	ousia_core
	${Boost_LIBRARIES}
)

# Output libraries

ADD_LIBRARY(ousia_html
	src/plugins/html/DemoOutput
)

TARGET_LINK_LIBRARIES(ousia_html
	ousia_core
)

ADD_LIBRARY(ousia_xml
	src/plugins/xml/XmlOutput
)

TARGET_LINK_LIBRARIES(ousia_xml
	ousia_core
)

#ADD_LIBRARY(ousia_mozjs
#	src/plugins/mozjs/MozJsScriptEngine
#)
#
#TARGET_LINK_LIBRARIES(ousia_mozjs
#	ousia_core
#	${MOZJS_LIBRARIES}
#)

# Command line interface

#ADD_EXECUTABLE(ousia
#	src/cli/Main
#)

#TARGET_LINK_LIBRARIES(ousia
#	ousia_core
#	ousia_filesystem
#	ousia_html
#	ousia_xml
#	ousia_osml
#	ousia_osxml
#	${Boost_LIBRARIES}
#)

# If testing is enabled, build the unit tests
IF(TEST)

	INCLUDE_DIRECTORIES(
		${GTEST_INCLUDE_DIRS}
		test/
	)

	ADD_EXECUTABLE(ousia_test_core
		test/core/RangeSetTest
		test/core/RegistryTest
		test/core/XMLTest
		test/core/common/ArgumentTest
		test/core/common/CharReaderTest
		test/core/common/FunctionTest
		test/core/common/LoggerTest
		test/core/common/PropertyTest
		test/core/common/RttiTest
		test/core/common/SourceContextReaderTest
		test/core/common/VariantConverterTest
		test/core/common/VariantReaderTest
		test/core/common/VariantWriterTest
		test/core/common/VariantTest
		test/core/common/UtilsTest
		test/core/frontend/TerminalLoggerTest
		test/core/managed/ManagedContainerTest
		test/core/managed/ManagedTest
		test/core/managed/ManagerTest
		test/core/managed/VariantObjectTest
		test/core/model/DomainTest
		test/core/model/DocumentTest
		test/core/model/IndexTest
		test/core/model/NodeTest
		test/core/model/StyleTest
		test/core/model/TypesystemTest
		test/core/parser/ParserScopeTest
#		test/core/parser/stack/StackTest
		test/core/parser/stack/StateTest
		test/core/parser/stack/TokenRegistryTest
		test/core/parser/utils/SourceOffsetVectorTest
		test/core/parser/utils/TokenizedDataTest
		test/core/parser/utils/TokenizerTest
		test/core/parser/utils/TokenTrieTest
		test/core/resource/ResourceLocatorTest
		test/core/resource/ResourceRequestTest
#		test/core/script/FunctionTest
#		test/core/script/ObjectTest
#		test/core/script/VariantTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_core
		${GTEST_LIBRARIES}
		ousia_core
	)

#	ADD_EXECUTABLE(ousia_test_css
#		test/plugins/css/Tokenizer
#		test/plugins/css/CodeTokenizerTest
#		test/plugins/css/CSSParserTest
#	)

#	TARGET_LINK_LIBRARIES(ousia_test_css
#		${GTEST_LIBRARIES}
#		ousia_core
#		ousia_css
#	)

	ADD_EXECUTABLE(ousia_test_filesystem
		test/plugins/filesystem/FileLocatorTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_filesystem
		${GTEST_LIBRARIES}
		ousia_core
		ousia_filesystem
	)

	ADD_EXECUTABLE(ousia_test_html
		test/plugins/html/DemoOutputTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_html
		${GTEST_LIBRARIES}
		ousia_core
		ousia_html
	)

#	ADD_EXECUTABLE(ousia_test_mozjs
#		test/plugins/mozjs/MozJsScriptEngineTest
#	)

#	TARGET_LINK_LIBRARIES(ousia_test_mozjs
#		${GTEST_LIBRARIES}
#		ousia_core
#		ousia_mozjs
#	)

	ADD_EXECUTABLE(ousia_test_osml
#		test/formats/osml/OsmlParserTest
		test/formats/osml/OsmlStreamParserTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_osml
		${GTEST_LIBRARIES}
		ousia_core
		ousia_osml
		ousia_filesystem
	)

	ADD_EXECUTABLE(ousia_test_osxml
		test/formats/osxml/OsxmlEventParserTest
#		test/formats/osxml/OsxmlParserTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_osxml
		${GTEST_LIBRARIES}
		ousia_core
		ousia_osxml
		ousia_filesystem
	)

	ADD_EXECUTABLE(ousia_test_xml
		test/plugins/xml/XmlOutputTest
	)

	TARGET_LINK_LIBRARIES(ousia_test_xml
		${GTEST_LIBRARIES}
		ousia_core
		ousia_xml
	)

	# Register the unit tests
	ADD_TEST(ousia_test_core ousia_test_core)
#	ADD_TEST(ousia_test_css ousia_test_css)
	ADD_TEST(ousia_test_filesystem ousia_test_filesystem)
	ADD_TEST(ousia_test_html ousia_test_html)
#	ADD_TEST(ousia_test_mozjs ousia_test_mozjs)
	ADD_TEST(ousia_test_osml ousia_test_osml)
	ADD_TEST(ousia_test_osxml ousia_test_osxml)
	ADD_TEST(ousia_test_xml ousia_test_xml)
ENDIF()

################################################################################
# Commands for installing                                                      #
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

#INSTALL(TARGETS ousia
#		RUNTIME DESTINATION bin
#)

IF(INSTALL_GEDIT_HIGHLIGHTER)
	INSTALL(FILES contrib/gtksourceview-3.0/language-specs/ousia.lang
			DESTINATION share/gtksourceview-3.0/language-specs
			PERMISSIONS
				OWNER_READ GROUP_READ WORLD_READ
				OWNER_WRITE)
ENDIF()
