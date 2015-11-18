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

find_package (Boost COMPONENTS unit_test_framework)
include_directories (${Boost_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/src)

# Shortcut to compile and declare a test in one shot
# Usage: define_test (mytest common) => add a test with the source mytest.cpp linking against the common library
#        define_test (mytest common helper.cpp) => similar, but compiling also helper.cpp
function (define_test name link)
    add_executable (${name} "${name}.cpp" ${ARGN})
    target_link_libraries (${name} ${link} ${Boost_LIBRARIES})
    add_test (${name} ${name})
endfunction(define_test)

# Tests
add_subdirectory (unit)
