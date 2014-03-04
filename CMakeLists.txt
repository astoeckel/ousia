PROJECT(basicwriter)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

SET(CMAKE_AUTOMOC ON)
FIND_PACKAGE(Qt5Widgets REQUIRED)

INCLUDE_DIRECTORIES(src/)

# Enable C++11 and all warnings
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11")

ADD_EXECUTABLE(basicwriter
		src/main.cpp
		src/gui/notepad.cpp
		src/gui/components/UIPaneFrame.cpp
#		src/gui/components/UIGrid.cpp
#		src/gui/components/GridTree.cpp
)

QT5_USE_MODULES(basicwriter Widgets)
