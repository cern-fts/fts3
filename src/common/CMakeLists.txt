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

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_include_dir 
	"${PROJECT_SOURCE_DIR}/src/"
	"${CMAKE_BINARY_DIR}/src/"
)

#include_directories (/usr/include/openssl)
if (ORACLEBUILD STREQUAL "OFF")
include_directories(${OPENSSL_INCLUDE_DIR})
endif ()
include_directories (${fts_include_dir})
include_directories (.)

FILE(GLOB SSL_CPP "StaticSslLocking*")

# Find libuuid
find_package(LibUuid REQUIRED)
include_directories(${LIBUUID_INCLUDE_DIRS})

file(GLOB fts_common_sources "*.cpp")
if (ORACLEBUILD STREQUAL "ON")
list(REMOVE_ITEM fts_common_sources ${SSL_CPP})
endif ()


# Find libuuid
find_package(LibUuid REQUIRED)
include_directories(${LIBUUID_INCLUDE_DIRS})



add_library(fts_common SHARED ${fts_common_sources})
target_link_libraries(fts_common ${Boost_LIBRARIES} )
target_link_libraries(fts_common -lldap_r -lboost_regex-mt -lboost_iostreams-mt -lboost_system-mt -lboost_thread-mt ${LIBUUID_LIBRARIES} )
if (ORACLEBUILD STREQUAL "OFF")
target_link_libraries(fts_common ${OPENSSL_LIBRARIES})
target_link_libraries(fts_common ${LIBUUID_LIBRARIES})
endif ()



set_target_properties(fts_common             PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/common
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)



if(FTS3_COMPILE_WITH_UNITTEST)
	target_link_libraries(fts_common -lboost_unit_test_framework-mt )
endif()


if (MAINBUILD STREQUAL "ON" OR ALLBUILD STREQUAL "ON")
install(TARGETS         fts_common 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
endif ()
