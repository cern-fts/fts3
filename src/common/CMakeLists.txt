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

option(WITHOUT_GOOGLE_COREDUMPER "Include Google Coredumper" OFF)

# Find libuuid
find_package(LibUuid REQUIRED)
include_directories(${LIBUUID_INCLUDE_DIRS})

# Find boost
find_package(Boost COMPONENTS regex iostreams system thread)

# Build Google coredumper
# Depend on .touch to avoid re-running configure every time
if(NOT WITHOUT_GOOGLE_COREDUMPER)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper/.touch
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper
        COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper/.touch
    )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper/Makefile
        WORKING_DIRECTORY google-coredumper
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/google-coredumper/configure CFLAGS='-fPIC -D_GNU_SOURCE'
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper/.touch
    )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper/.libs/libcoredumper.a
        WORKING_DIRECTORY google-coredumper
        COMMAND make
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper/Makefile
    )
    add_custom_target(libcoredumper
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper/.libs/libcoredumper.a
    )
    
    add_library(coredumper STATIC IMPORTED)
    add_dependencies(coredumper libcoredumper)
    set_target_properties(coredumper PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/google-coredumper/.libs/libcoredumper.a
    )
    
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/google-coredumper/src)
else()
    add_definitions(-DWITHOUT_GOOGLE_COREDUMPER)
endif()

# Sources
file(GLOB fts_common_sources "*.cpp")

add_library(fts_common SHARED ${fts_common_sources})
target_link_libraries(fts_common
    ${Boost_LIBRARIES}
    ${LIBUUID_LIBRARIES}
    -lpthread
)
if(NOT WITHOUT_GOOGLE_COREDUMPER)
    target_link_libraries (fts_common coredumper)
endif()

set_target_properties(fts_common PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/common
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)

if(FTS3_COMPILE_WITH_UNITTEST)
    find_package(Boost COMPONENTS unit_test_framework REQUIRED)
	target_link_libraries(fts_common ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} )
endif()

# Install only if we are building the server or the client
# (Do not install if only building a database backend, for instance)
if(SERVERBUILD OR CLIENTBUILD)
    install(TARGETS fts_common
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    )
endif ()
