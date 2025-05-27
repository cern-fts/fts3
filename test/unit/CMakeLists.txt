#
# Copyright (c) CERN 2025
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

# Disable "-Wconversion" warnings in unit tests
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

find_package(GFAL2)
find_package(GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS} ${GFAL2_INCLUDE_DIRS})

add_executable(fts-unit-tests unit.cpp)

# Build individual unit tests
add_subdirectory (common)
add_subdirectory (config)
add_subdirectory (cred)
add_subdirectory (db)
add_subdirectory (msg-bus)
add_subdirectory (server)
add_subdirectory (url-copy)

target_link_libraries(fts-unit-tests "-Wl,--no-as-needed" ${Boost_LIBRARIES})

install(TARGETS fts-unit-tests RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(fts-unit-test fts-unit-tests)
