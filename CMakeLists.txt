PROJECT(basicwriter)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

SET(CMAKE_AUTOMOC ON)
FIND_PACKAGE(Qt5Widgets REQUIRED)

# Enable C++11 and all warnings
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11")

# Option for enabling testing. Turn on with 'cmake -Dtest=ON'.
OPTION(test "Build all tests." OFF) # Makes boolean 'test' available.

# Unit test handling
if(test)
	# Compile gtest
	add_subdirectory(lib/gtest-1.7.0)

	# Enable testing and declare some variables used for compiling the test cases
	enable_testing()
	set(GTEST_INCLUDE_DIR ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
	set(GTEST_LIBRARIES gtest gtest_main)
endif()

INCLUDE_DIRECTORIES(src/)

ADD_LIBRARY(ousia_core
	src/model/GraphNode
	src/model/domain/Domain
)

ADD_EXECUTABLE(ousia
	src/main
)

TARGET_LINK_LIBRARIES(ousia
	ousia_core
)

QT5_USE_MODULES(ousia Widgets)

if(test)
	# Include the gtest include files and the src directory
	INCLUDE_DIRECTORIES(
		${GTEST_INCLUDE_DIR}
		src/
	)

	# Add all unit test files
	ADD_EXECUTABLE(ousia_test
		test/model/GraphNode
	)

	TARGET_LINK_LIBRARIES(ousia_test
		${GTEST_LIBRARIES}
		ousia_core
	)

	# Register the unit test
	ADD_TEST(ousia_test ousia_test)
endif()

