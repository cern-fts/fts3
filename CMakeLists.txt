#
# Copyright (c) CERN 2013-2015
#
# Copyright (c) Members of the EMI Collaboration. 2010-2013
#  See  http://www.eu-emi.eu/partners for details on the copyright
#  holders.
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
#

cmake_minimum_required(VERSION 2.8)
project(fts3)


set(CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/cmake/modules
    ${CMAKE_MODULE_PATH}
)

set(VERSION_MAJOR 3)
set(VERSION_MINOR 10)
set(VERSION_PATCH 0)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# Generate include header with the version string
file(
    WRITE ${CMAKE_BINARY_DIR}/src/common/version.h
    "\#define VERSION \"${VERSION_STRING}\"\n"
)
include_directories("${CMAKE_BINARY_DIR}/src/common/")

# Optional subsystem builds
option(ALLBUILD "Build fts all including db plug-ins" OFF)

include(CMakeDependentOption REQUIRED)
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
cmake_dependent_option(
    TESTBUILD "Build the tests" OFF
    "NOT ALLBUILD" ON
)

# Definitions
add_definitions(-D_LARGEFILE64_SOURCE)
add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-DWITH_IPV6)
add_definitions(-DOPENSSL_THREADS)
add_definitions(-DSOCKET_CLOSE_ON_EXIT)
add_definitions(-D_REENTRANT)

# Enable C++11 if possible
include(EnableCpp11 REQUIRED)

# Define where to install the generated artifacts
include(DefineInstallationPaths REQUIRED)

# Debug builds
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Building with Debug settings...")

    # Increase warnings level
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi -Wall -Wextra -pedantic -Wconversion -Wno-long-long -Wuninitialized -Wno-write-strings -Wsign-promo -Wstrict-overflow=5 -Winit-self -Wmissing-braces -Wparentheses")
    # Generate additional code
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftrapv -fstack-protector-all -finstrument-functions")

    # Compile the unit tests
    option (FTS3_COMPILE_WITH_UNITTEST_NEW "Build with unit test" ON)
endif ()

# Dependencies
find_package(Threads)
find_package(OpenSSL)

# Find Boost
set(Boost_ADDITIONAL_VERSIONS
    "1.41" "1.41.0" "1.42" "1.42.0" "1.43" "1.43.0" "1.44" "1.44.0" "1.45" "1.45.0"
    "1.46" "1.47.0" "1.48" "1.49.0" "1.50" "1.51.0"
)
set(Boost_USE_MULTITHREADED ON)
set(Boost_NO_BOOST_CMAKE ON)

# Hack for SL5 with EPEL5 enabled
set(BOOST_INCLUDEDIR "/usr/include/boost148")
set(BOOST_LIBRARYDIR "/usr/lib${LIB_SUFFIX}/boost148/")

find_package(Boost 1.48 REQUIRED)

link_directories (${Boost_LIBRARY_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

# Subdirectories
add_subdirectory(src)

if (TESTBUILD)
    include (CTest)
    add_subdirectory (test)
endif (TESTBUILD)

