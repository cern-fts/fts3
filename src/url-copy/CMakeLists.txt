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


SET(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${PROJECT_SOURCE_DIR}/src/db/generic/)
include_directories (${PROJECT_SOURCE_DIR}/src/common/)
include_directories (${PROJECT_SOURCE_DIR}/src/server/)
include_directories (${fts3_include_dir})
include_directories (/usr/include/gfal2)
include_directories (/usr/include/glib-2.0)
include_directories (/usr/lib64/glib-2.0/include)

#set(fts3_url_copy_lib_SOURCES parse_url.cpp mq_producer.cpp process.cpp mq_manager.cpp stringhelper.cpp process_service_handler.h process_service.h )
#add_library(fts3_url_copy_lib SHARED ${fts3_url_copy_lib_SOURCES})
#target_link_libraries(fts3_url_copy_lib -lfts3_config -lfts3_common -lfts3_server_lib -lboost_program_options -lgfal_transfer -lgfal2 -lglib-2.0    )


set(fts3_url_copy_SOURCES main.cpp parse_url.cpp )
add_executable (fts3_url_copy ${fts3_url_copy_SOURCES})
#SET(fts3_libs "-lfts3_config -lfts3_common -lfts3_server_lib")
SET(fts3_libs "-lfts3_config -lfts3_common ")
target_link_libraries(fts3_url_copy  -luuid -lboost_program_options -lboost_thread-mt -lgfal_transfer -lgfal2 -lglib-2.0 ${fts3_libs})


install(TARGETS         fts3_url_copy 
	RUNTIME             DESTINATION ${BIN_INSTALL_DIR} )
