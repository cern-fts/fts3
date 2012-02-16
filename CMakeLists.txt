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

set(libsubdirs "lib64;lib" CACHE STRING "The path suffixes to find libraries")

ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    MESSAGE ("Building with Debug settings...")
    find_package(CppUnit)
    add_definitions (-DFTS3_COMPILE_WITH_UNITTEST)
ENDIF ()

find_package(OCCI)
find_package(Boost)
find_package(Log4CPP)

add_subdirectory(test)
add_subdirectory(src)
