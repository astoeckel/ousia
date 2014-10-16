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

PROJECT(basicwriter)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

# Option for enabling testing. Turn on with 'cmake -Dtest=ON'.
OPTION(test "Build all tests." OFF) # Makes boolean 'test' available.

# Enable C++11 and all warnings
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0  -Wall -pedantic -std=c++11")

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

# Includ directories
INCLUDE_DIRECTORIES(
	src/
)

# ousia_script library (containing the code of t
ADD_LIBRARY(ousia_script
	src/core/script/Variant.cpp
)

# Definition of the main program
#ADD_EXECUTABLE(ousia
#)

# Link the ousia executable against ousia_core
#TARGET_LINK_LIBRARIES(ousia
#	ousia_script
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
		test/core/script/Variant
	)

	TARGET_LINK_LIBRARIES(ousia_test
		${GTEST_LIBRARIES}
		ousia_script
	)

	# Register the unit test
	ADD_TEST(ousia_test ousia_test)
ENDIF()

