#
# Copyright (c) CERN 2015
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

add_definitions ("-DBOOST_TEST_DYN_LINK")

set (UNIT_TEST_LIST "" CACHE INTERNAL "Unit tests" FORCE)

# Shortcut to compile and declare a test in one shot
# Usage: define_test (mytest common) => add a test with the source mytest.cpp linking against the common library
#        define_test (mytest common helper.cpp) => similar, but compiling also helper.cpp
function (define_test name link)
    add_library (${name} SHARED "${name}.cpp" ${ARGN})
    target_link_libraries (${name} ${link})
    set_target_properties(${name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/unit
    )

    list (APPEND UNIT_TEST_LIST ${name})
    set (UNIT_TEST_LIST "${UNIT_TEST_LIST}" CACHE INTERNAL "Unit tests" FORCE)
endfunction(define_test)

# Build individual unit tests
add_subdirectory (cli)
add_subdirectory (common)
add_subdirectory (config)
add_subdirectory (msg-bus)

# Build THE test binary
message ("Found tests: ${UNIT_TEST_LIST}")
add_executable (unit unit.cpp)
target_link_libraries(unit
    ${Boost_LIBRARIES}
    ${UNIT_TEST_LIST}
)
add_test (unit unit)
