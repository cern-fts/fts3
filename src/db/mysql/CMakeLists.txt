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

find_package(MySQL REQUIRED)

include_directories (${MYSQL_INCLUDE_DIR})
include_directories (/usr/include/soci)

set(fts_db_mysql_SOURCES MySqlAPI.cpp)
add_library(fts_db_mysql SHARED ${fts_db_mysql_SOURCES})
target_link_libraries(fts_db_mysql
    fts_common
    soci_core
    soci_mysql
    ${MYSQL_LIBRARIES}
)
set_target_properties(fts_db_mysql PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/db/mysql
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)

# Artifacts
install(TARGETS fts_db_mysql
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX} 
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        NAMELINK_SKIP
)
