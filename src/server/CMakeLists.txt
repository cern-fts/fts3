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

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(GLIB2)
find_package(ZMQ)
find_package(JSONCPP)

include_directories(
	${GLIB2_INCLUDE_DIRS}
	${ZMQ_INCLUDE_DIRS}
	${JSONCPP_INCLUDE_DIRS}
)

# Server lib
find_package(Boost COMPONENTS thread regex filesystem timer chrono)

file(GLOB fts_server_lib_sources
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
    "services/*/*.cpp"
)
list(REMOVE_ITEM fts_server_lib_sources "main.cpp")

add_library(fts_server_lib SHARED ${fts_server_lib_sources})
target_link_libraries(fts_server_lib
    fts_db_generic
    fts_msg_bus
    fts_proxy
    gridsite
    ${GLIB2_LIBRARIES}
    ${Boost_LIBRARIES}
    ${ZMQ_LIBRARIES}
    JSONCPP::JSONCPP
)
set_target_properties(fts_server_lib PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/server
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)

# Server executable
add_executable(fts_server main.cpp)
add_dependencies(fts_server fts_server_lib)
target_link_libraries(fts_server
    fts_server_lib
    fts_config
    fts_common
    fts_msg_ifce
    ${Boost_LIBRARIES}
)

# Man pages
set(MAN_INPUT_DIR "${PROJECT_SOURCE_DIR}/doc/man/server")
install(FILES
		"${MAN_INPUT_DIR}/fts_server.8"
		"${MAN_INPUT_DIR}/fts_db_cleaner.8"
		"${MAN_INPUT_DIR}/fts_activemq.8"
		"${MAN_INPUT_DIR}/fts_url_copy.8"
		"${MAN_INPUT_DIR}/fts_qos.8"
		"${MAN_INPUT_DIR}/fts_optimizer.8"
		"${MAN_INPUT_DIR}/fts_token.8"
	DESTINATION ${MAN_INSTALL_DIR}/man8/)

# Artifacts
install(TARGETS fts_server_lib
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
)

install(TARGETS fts_server
	RUNTIME DESTINATION ${SBIN_INSTALL_DIR}
)
