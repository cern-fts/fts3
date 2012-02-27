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
    

cmake_minimum_required(VERSION 2.6)
project(FTS3)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

link_directories (${PROJECT_BINARY_DIR}/src/common) 
link_directories (${PROJECT_BINARY_DIR}/src/config) 
link_directories (${PROJECT_BINARY_DIR}/src/server) 

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wall -pedantic -Wno-long-long -D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector")
set(libsubdirs "lib64;lib" CACHE STRING "The path suffixes to find libraries")
# load module
include(DefineInstallationPaths REQUIRED)


ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    MESSAGE ("Building with Debug settings...")
    add_definitions (-DFTS3_COMPILE_WITH_UNITTEST)
    OPTION(FTS3_COMPILE_WITH_UNITTEST "Build with unit test" ON)
endif ()

SET(Boost_DEBUG ON)
set(BOOST_LIBRARYDIR /usr/lib64)
SET(Boost_ADDITIONAL_VERSIONS "1.41" "1.41.0")
set(Boost_USE_MULTITHREADED ON)
find_package(OCCI)
find_package(Boost)

add_subdirectory(test)
add_subdirectory(src)



