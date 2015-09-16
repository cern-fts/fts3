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

find_package(GFAL2)
find_package(GLIB2)
include_directories (
    ${GLIB2_INCLUDE_DIRS}
    ${GFAL2_INCLUDE_DIRS}
)

find_package(Boost COMPONENTS thread)

# Include directly the sources from common to reduce the memory footprint
set(fts_url_copy_SOURCES
    args.cpp
    main.cpp
    file_management.cpp
    heuristics.cpp
    reporter.cpp
)

add_executable(fts_url_copy ${fts_url_copy_SOURCES})
target_link_libraries(fts_url_copy
    uuid ssl crypto
    fts_common
    fts_msg_ifce fts_proxy
    ${Boost_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GFAL2_LIBRARIES}
)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
	target_link_libraries(fts_url_copy boost_unit_test_framework)
endif()


# Install artifacts
install(TARGETS fts_url_copy 
	RUNTIME DESTINATION ${SBIN_INSTALL_DIR}
)
