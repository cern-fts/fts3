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

set (CMAKE_INCLUDE_CURRENT_DIR ON)


SET(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")
link_directories (/usr/lib64/gfal2-plugins)
include_directories (${PROJECT_SOURCE_DIR}/src/db/generic/)
include_directories (${PROJECT_SOURCE_DIR}/src/common/)
include_directories (${PROJECT_SOURCE_DIR}/src/server/)
include_directories (${PROJECT_SOURCE_DIR}/src/monitoring/)

include_directories (${PROJECT_BINARY_DIR}/src/db/generic/)
include_directories (${PROJECT_BINARY_DIR}/src/common/)
include_directories (${PROJECT_BINARY_DIR}/src/server/)
include_directories (${PROJECT_BINARY_DIR}/src/monitoring/)

include_directories (${fts_include_dir})
include_directories (/usr/include/gfal2)
include_directories (/usr/include/glib-2.0)
include_directories (/usr/lib64/glib-2.0/include)

set(fts_url_copy_SOURCES main.cpp file_management.cpp reporter.cpp)
add_executable (fts_url_copy ${fts_url_copy_SOURCES})
add_dependencies(fts_url_copy fts_common fts_msg_ifce)
SET(fts_libs "-lfts_config -lfts_common")
target_link_libraries(fts_url_copy  -luuid  -lboost_thread-mt -lboost_system-mt -lssl -lgfal_transfer -lgfal2 -lglib-2.0 -lfts_msg_ifce ${fts_libs})


install(TARGETS         fts_url_copy 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )
