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
include_directories (BEFORE ../generic)
include_directories (BEFORE ../common)
include_directories (BEFORE ../config)
include_directories (.)

set(fts3_msg_ifce_SOURCES msg-ifce.cpp utility_routines.cpp Logger.cpp)
add_library(fts3_msg_ifce SHARED ${fts3_msg_ifce_SOURCES})
target_link_libraries(fts3_msg_ifce -lrt)


SET(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")

include_directories (${APRUTIL_INCLUDE_DIR})
include_directories (${APR_INCLUDE_DIR})
include_directories (${ACTIVEMQCPP_INCLUDE_DIR})

include_directories (${PROJECT_SOURCE_DIR}/src/common/)
include_directories (${fts3_include_dir})
include_directories (/usr/include/glib-2.0)
include_directories (/usr/lib64/glib-2.0/include)

set(fts3_msg_bulk_SOURCES utility_routines.cpp msg-bulk-send.cpp MsgRecoverPipe.cpp MsgPipe.cpp MsgProducer.cpp MsgFile.cpp concurrent_queue.cpp Logger.cpp)
add_executable (fts3_msg_bulk ${fts3_msg_bulk_SOURCES})
target_link_libraries(fts3_msg_bulk ${ACTIVEMQCPP_LIBRARIES} ${APR_LIBRARIES} ${APRUTIL_LIBRARIES} -lrt -lglib-2.0)

set(fts3_msg_cron_SOURCES msg-cron.cpp Logger.cpp utility_routines.cpp)
add_executable (fts3_msg_cron ${fts3_msg_cron_SOURCES})
target_link_libraries(fts3_msg_cron  ${ACTIVEMQCPP_LIBRARIES} ${APR_LIBRARIES} ${APRUTIL_LIBRARIES}  -lglib-2.0)
target_link_libraries(fts3_msg_cron ${OCCI_LIBRARY} )
target_link_libraries(fts3_msg_cron ${CLNTSH_LIBRARY} )
target_link_libraries(fts3_msg_cron -lpthread -lfts3_config -lfts3_common -lboost_thread-mt -lrt )


install(TARGETS         fts3_msg_bulk 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(TARGETS         fts3_msg_cron
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(TARGETS         fts3_msg_ifce 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
