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

find_package (GLIB2)
find_package (GLOBUS)

include_directories (${GLIB2_INCLUDE_DIRS})
include_directories (${GLOBUS_INCLUDE_DIRS})

# Require soap headers
include_directories ("${CMAKE_BINARY_DIR}/src/ws-ifce/gsoap")


set(fts_delegation_api_simple_SOURCES util.cpp cfg.cpp delegation_simple_api.cpp)
add_library(fts_delegation_api_simple SHARED ${fts_delegation_api_simple_SOURCES})
target_link_libraries(fts_delegation_api_simple
    gridsite
    ${GLIB2_LIBRARIES}
    gsoap++
    cgsi_plugin_cpp
    is_ifce
    fts_delegation_api_cpp
)
set_target_properties(fts_delegation_api_simple PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/delegation-cli
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR} CLEAN_DIRECT_OUTPUT 1
)

# Artifacts
install(TARGETS fts_delegation_api_simple 
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
)
