cmake_minimum_required(VERSION 2.6)
project(FTS3)

set(CMAKE_BUILD_TYPE Debug)
ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)

add_subdirectory(test)
add_subdirectory(src)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
set(CMAKE_BUILD_TYPE Debug)
set(libsubdirs "lib64;lib" CACHE STRING "The path suffixes to find libraries")

find_package(OCCI)
find_package(Boost)
find_package(Log4CPP)
