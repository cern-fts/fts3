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

add_definitions ("-DBOOST_TEST_DYN_LINK")

set (UNIT_TEST_LIST "" CACHE INTERNAL "Unit tests" FORCE)

# Disable "-Wconversion" warnings in unit tests
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

# Shortcut to compile and declare a test in one shot
# Usage: define_test (mytest common) => add a test with the source mytest.cpp linking against the common library
#        define_test (mytest common helper.cpp) => similar, but compiling also helper.cpp
function (define_test name link)
    add_library (${name} SHARED "${name}.cpp" ${ARGN})
    target_link_libraries (${name} ${link})
    install(TARGETS ${name}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/fts-tests/)

    list (APPEND UNIT_TEST_LIST ${name})
    set (UNIT_TEST_LIST "${UNIT_TEST_LIST}" CACHE INTERNAL "Unit tests" FORCE)
endfunction(define_test)

# Build individual unit tests
add_subdirectory (common)
add_subdirectory (config)
add_subdirectory (cred)
add_subdirectory (db)
add_subdirectory (msg-bus)
add_subdirectory (server)
add_subdirectory (url-copy)

# Build the test binary
message(STATUS "Found tests: ${UNIT_TEST_LIST}")

# Use RPATH to link with individual FTS test libraries
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}/fts-tests/")

add_executable(fts-unit-tests unit.cpp)
target_link_libraries(fts-unit-tests
    "-Wl,--no-as-needed"
    ${Boost_LIBRARIES}
    ${UNIT_TEST_LIST})

install(TARGETS fts-unit-tests
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(fts-unit-test fts-unit-tests)
