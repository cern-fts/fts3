# Copyright @ Members of the EMI Collaboration, 2010.
# See www.eu-emi.eu for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at 
#
#    http://www.apache.org/licenses/LICENSE-2.0 
#
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions and 
# limitations under the License. 
cmake_minimum_required(VERSION 2.8)
project(fts3)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})


set(VERSION_MAJOR 3)
set(VERSION_MINOR 4)
set(VERSION_PATCH 0)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

file(
    WRITE ${CMAKE_BINARY_DIR}/src/common/version.h
    "\#define VERSION \"@VERSION_STRING@\"\n"
)
include_directories("${CMAKE_BINARY_DIR}/src/common/")

option(ALLBUILD "Build fts all including db plug-ins" OFF)

include (CMakeDependentOption REQUIRED)
cmake_dependent_option(
    ORACLEBUILD "Build oracle plug-in" OFF
    "NOT ALLBUILD" ON
)
cmake_dependent_option(
    MYSQLBUILD "Build for EPEL" OFF
    "NOT ALLBUILD" ON
)
cmake_dependent_option(
    SERVERBUILD "Build fts" OFF
    "NOT ALLBUILD" ON
)
cmake_dependent_option(
    CLIENTBUILD "Build the client libraries, tools and bindings" OFF
    "NOT ALLBUILD" ON
)

 
add_definitions(-DWITH_IPV6)
add_definitions(-DOPENSSL_THREADS)
add_definitions(-DNDEBUG)
add_definitions(-DSOCKET_CLOSE_ON_EXIT)
add_definitions(-D_REENTRANT)
add_definitions(-DBOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS)

# Enable C++11
include(EnableCpp11 REQUIRED)

set(libsubdirs "lib64;lib" CACHE STRING "The path suffixes to find libraries")
# load module
include(DefineInstallationPaths REQUIRED)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftrapv -Wno-long-long -Wconversion -Wuninitialized -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Wextra -ansi -Wall -pedantic -Wno-write-strings -D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-all -finstrument-functions -Wsign-promo -Wstrict-overflow=5 -Winit-self -Wmissing-braces -Wparentheses")
    MESSAGE ("Building with Debug settings...")
    add_definitions (-DFTS3_COMPILE_WITH_UNITTEST_NEW)
    OPTION(FTS3_COMPILE_WITH_UNITTEST_NEW "Build with unit test" ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
    #ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)   
    #ADD_DEFINITIONS(-D_GLIBCXX_DEBUG_PEDANTIC -DDEBUG=1 -g3  -O0 -D_GLIBXX_DEBUG_PEDANTIC -D_GLIBCXX_DEBUG_PEDANTIC -D_GLIBCXX_CONCEPT_CHECKS -D_GLIBCPP_DEBUG_PEDANTIC -D_GLIBCPP_CONCEPT_CHECKS)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-long-long -Wconversion -Wuninitialized  -Wno-deprecated-declarations -Wextra -Wall -D_FORTIFY_SOURCE=2 -fstack-protector-all -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64")
endif ()

find_package(Threads)
find_package(OpenSSL)

set(Boost_ADDITIONAL_VERSIONS
    "1.41" "1.41.0" "1.42" "1.42.0" "1.43" "1.43.0" "1.44" "1.44.0" "1.45" "1.45.0"
    "1.46" "1.47.0" "1.48" "1.49.0" "1.50" "1.51.0"
)
set(Boost_USE_MULTITHREADED ON)
set (Boost_NO_BOOST_CMAKE ON)

# Hack for SL5 with EPEL5 enabled
set(BOOST_INCLUDEDIR "/usr/include/boost141") # EPEL5
set (BOOST_LIBRARYDIR "/usr/lib${LIB_SUFFIX}/boost141/") # EPEL5
set (Boost_ADDITIONAL_VERSIONS "1.41" "1.41.0")

find_package(Boost 1.41 REQUIRED)

link_directories (${Boost_LIBRARY_DIRS})
include_directories (${Boost_INCLUDE_DIR})


# Subdirectories
add_subdirectory(test)
add_subdirectory(src)

