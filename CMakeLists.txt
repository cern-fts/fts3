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
project(fts3)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

link_directories (/usr/lib64/gfal2-plugins)
link_directories (${PROJECT_BINARY_DIR}/src/common)
link_directories (${PROJECT_BINARY_DIR}/src/cred) 
link_directories (${PROJECT_BINARY_DIR}/src/ws-ifce) 
link_directories (${PROJECT_BINARY_DIR}/src/ws-ifce/gsoap) 
link_directories (${PROJECT_BINARY_DIR}/src/config) 
link_directories (${PROJECT_BINARY_DIR}/src/url-copy) 
link_directories (${PROJECT_BINARY_DIR}/src/server) 
link_directories (${PROJECT_BINARY_DIR}/src/db/generic)
link_directories (${PROJECT_BINARY_DIR}/src/db/python) 
link_directories (${PROJECT_BINARY_DIR}/src/cli)
link_directories (${PROJECT_BINARY_DIR}/src/monitoring) 
link_directories (${PROJECT_BINARY_DIR}/src/delegation-cli) 
link_directories (${PROJECT_BINARY_DIR}/src/dbClear) 
link_directories (${PROJECT_BINARY_DIR}/src/glue2-publisher) 

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

option(ORACLEBUILD "Build oracle plug-in" OFF)
option(MYSQLBUILD "Build for EPEL" OFF)
option(MAINBUILD "Build fts" OFF)
option(ALLBUILD "Build fts all including db plug-ins" OFF)
	 
ADD_DEFINITIONS(-DWITH_IPV6)
#ADD_DEFINITIONS(-DPURIFY) 
ADD_DEFINITIONS(-DOPENSSL_THREADS)
ADD_DEFINITIONS(-DWITH_OPENSSL)
ADD_DEFINITIONS(-DNDEBUG)
ADD_DEFINITIONS(-DSOCKET_CLOSE_ON_EXIT)
ADD_DEFINITIONS(-D_REENTRANT)
ADD_DEFINITIONS(-DBOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS)

set(libsubdirs "lib64;lib" CACHE STRING "The path suffixes to find libraries")
# load module
include(DefineInstallationPaths REQUIRED)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-check -fno-strict-aliasing -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Wextra -ansi -Wall -pedantic -Wno-long-long -Wno-write-strings -D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-all -finstrument-functions -Wsign-promo -Wstrict-overflow=5 -Winit-self -Wmissing-braces -Wparentheses")
    MESSAGE ("Building with Debug settings...")
    #add_definitions (-DFTS3_COMPILE_WITH_UNITTEST)
    #OPTION(FTS3_COMPILE_WITH_UNITTEST "Build with unit test" ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
    #ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)   
    ADD_DEFINITIONS(-D_GLIBCXX_DEBUG_PEDANTIC -DDEBUG=1 -g3  -O0 )
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations  -fno-strict-aliasing -Wextra -Wall -D_FORTIFY_SOURCE=2 -fstack-protector-all -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64")
endif ()

set(BOOST_LIBRARYDIR /usr/lib64)
SET(Boost_ADDITIONAL_VERSIONS "1.41" "1.41.0")
set(Boost_USE_MULTITHREADED ON)

if (ORACLEBUILD STREQUAL "ON")
find_package(OCCI)
endif ()

if (ALLBUILD STREQUAL "ON")
find_package(OCCI)
find_package(APR 1.2.7 REQUIRED)
find_package(APRUtil 1.2.7 REQUIRED)
find_package(ActiveMQcpp 3.2.0 REQUIRED)
endif ()

if (MAINBUILD STREQUAL "ON")
find_package(APR 1.2.7 REQUIRED)
find_package(APRUtil 1.2.7 REQUIRED)
find_package(ActiveMQcpp 3.2.0 REQUIRED)
endif ()


find_package(Boost)


add_subdirectory(test)
add_subdirectory(src)



