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
include_directories(BEFORE ${OCCI_INCLUDE_DIR})
include_directories (${PROJECT_SOURCE_DIR}/src/db/generic/)
include_directories (${PROJECT_SOURCE_DIR}/src/common/)
include_directories (${PROJECT_SOURCE_DIR}/src/config/)
include_directories (${PROJECT_BINARY_DIR}/src/db/generic/)
include_directories (${PROJECT_BINARY_DIR}/src/common/)
include_directories (${PROJECT_BINARY_DIR}/src/config/)
include_directories (.)

set(fts_msg_ifce_SOURCES msg-ifce.cpp utility_routines.cpp Logger.cpp)
add_library(fts_msg_ifce SHARED ${fts_msg_ifce_SOURCES})
target_link_libraries(fts_msg_ifce -lboost_thread-mt -lboost_system-mt -lrt)
set_target_properties(fts_msg_ifce	 	PROPERTIES
						LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/monitoring
						VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
						SOVERSION ${VERSION_MAJOR}
						CLEAN_DIRECT_OUTPUT 1)

SET(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")

include_directories (${APRUTIL_INCLUDE_DIR})
include_directories (${APR_INCLUDE_DIR})
include_directories (${ACTIVEMQCPP_INCLUDE_DIR})

include_directories (${PROJECT_SOURCE_DIR}/src/common/)
include_directories (${fts_include_dir})
include_directories (/usr/include/glib-2.0)
include_directories (/usr/lib64/glib-2.0/include)

set(fts_msg_bulk_SOURCES utility_routines.cpp msg-bulk-send.cpp MsgRecoverPipe.cpp MsgPipe.cpp MsgProducer.cpp MsgFile.cpp concurrent_queue.cpp Logger.cpp)
add_executable (fts_msg_bulk ${fts_msg_bulk_SOURCES})
add_dependencies(fts_msg_bulk fts_common)
target_link_libraries(fts_msg_bulk ${ACTIVEMQCPP_LIBRARIES} ${APR_LIBRARIES} ${APRUTIL_LIBRARIES}  -lfts_common -lfts_config -lrt -lglib-2.0 -lboost_thread-mt -lboost_system-mt -lrt)

set(fts_msg_cron_SOURCES msg-cron.cpp Logger.cpp utility_routines.cpp)
add_executable (fts_msg_cron ${fts_msg_cron_SOURCES})
target_link_libraries(fts_msg_cron  ${ACTIVEMQCPP_LIBRARIES} ${APR_LIBRARIES} ${APRUTIL_LIBRARIES}  -lglib-2.0)
add_dependencies(fts_msg_cron fts_common fts_db_generic fts_config)
target_link_libraries(fts_msg_cron -lpthread -lfts_db_generic -lfts_common -lfts_config -lboost_thread-mt -lboost_system-mt -lrt )


install(TARGETS         fts_msg_bulk 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(TARGETS         fts_msg_cron
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(TARGETS         fts_msg_ifce 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
