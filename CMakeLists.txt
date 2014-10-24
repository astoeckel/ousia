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

# Option for enabling testing. Turn on with 'cmake -Dtest=ON'.
OPTION(test "Build all tests." OFF) # Makes boolean 'test' available.

# Option for building the documentation.
FIND_PACKAGE(Doxygen)
OPTION(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

# Enable C++11 and all warnings
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0  -Wall -pedantic -std=c++11")

# Use PkgConfig to find mozjs-24
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(MOZJS REQUIRED mozjs-24)

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
IF(test)
	# Compile gtest
	ADD_SUBDIRECTORY(lib/gtest-1.7.0)

	# Enable testing and declare some variables used for compiling the test cases
	ENABLE_TESTING()
	SET(GTEST_INCLUDE_DIR ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
	SET(GTEST_LIBRARIES gtest gtest_main)
ENDIF()

################################################################################
# Commands for building Ousía                                                  #
################################################################################

# Include directories
INCLUDE_DIRECTORIES(
	src/
	${MOZJS_INCLUDE_DIRS}
)

# Link directories
LINK_DIRECTORIES(
	${MOZJS_LIBRARY_DIRS}
)

ADD_DEFINITIONS(
	${MOZJS_CFLAGS_OTHER}
)

# ousia_utils library
ADD_LIBRARY(ousia_utils
	src/core/utils/Utils.cpp
)

# ousia_script library (containing the code of t
ADD_LIBRARY(ousia_script
	src/core/script/Function.cpp
	src/core/script/Variant.cpp
	src/core/script/ScriptEngine.cpp
	src/core/script/Object.cpp
)

# Link the ousia executable against ousia_core
TARGET_LINK_LIBRARIES(ousia_script
	ousia_utils
)

# Definition of the main program
#ADD_EXECUTABLE(ousia
#)

# If testing is enabled, build the unit tests
IF(test)
	# Include the gtest include files and the src directory
	INCLUDE_DIRECTORIES(
		${GTEST_INCLUDE_DIR}
		src/
	)

	# Add all unit test files
	ADD_EXECUTABLE(ousia_test
		test/core/model/RangeSet
		test/core/utils/Utils
		test/core/script/Function
		test/core/script/Variant
	)

	TARGET_LINK_LIBRARIES(ousia_test
		${GTEST_LIBRARIES}
		ousia_utils
		ousia_script
	)

	# Register the unit test
	ADD_TEST(ousia_test ousia_test)
ENDIF()

