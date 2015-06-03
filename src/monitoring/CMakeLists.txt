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

cmake_minimum_required(VERSION 2.8)

find_package(ActiveMQcpp 3.2.0 REQUIRED)

set (CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories (${PROJECT_SOURCE_DIR}/src/common/)
include_directories (${PROJECT_SOURCE_DIR}/src/config/)
include_directories (.)

# We include directly the sources from common to avoid pulling
# lots of dependencies to the messaging, increasing its size
set(fts_msg_ifce_SOURCES
    msg-ifce.cpp
    utility_routines.cpp
    Logger.cpp
    ${PROJECT_SOURCE_DIR}/src/common/name_to_uid.cpp
    ${PROJECT_SOURCE_DIR}/src/common/producer.cpp
    ${PROJECT_SOURCE_DIR}/src/common/consumer.cpp
)

add_library(fts_msg_ifce SHARED ${fts_msg_ifce_SOURCES})
add_dependencies(fts_msg_ifce fts_common)

set_target_properties(fts_msg_ifce	 	PROPERTIES
						LIBRARY_OUTPUT_DIRECTORY
                        ${PROJECT_BINARY_DIR}/src/monitoring
						VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
						SOVERSION ${VERSION_MAJOR}
						CLEAN_DIRECT_OUTPUT 1)

SET(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")

find_package (Boost COMPONENTS filesystem thread system)
find_package (JSONC REQUIRED)

include_directories (${ACTIVEMQCPP_INCLUDE_DIR})
include_directories (${PROJECT_SOURCE_DIR}/src/common/)
include_directories (${fts_include_dir})
include_directories (${JSONC_INCLUDE_DIRS})


set(fts_msg_bulk_SOURCES utility_routines.cpp msg-bulk-send.cpp MsgPipe.cpp MsgProducer.cpp Logger.cpp)

add_executable (fts_msg_bulk ${fts_msg_bulk_SOURCES})
add_dependencies(fts_msg_bulk fts_common fts_config)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
	target_link_libraries(fts_msg_bulk -lboost_unit_test_framework)
endif()

target_link_libraries(fts_msg_bulk
    ${ACTIVEMQCPP_LIBRARIES} fts_common fts_config -lrt ${Boost_LIBRARIES} ${JSONC_LIBRARIES})


install(TARGETS         fts_msg_bulk 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(TARGETS         fts_msg_ifce 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
