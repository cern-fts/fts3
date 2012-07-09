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

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(fts3_include_dir 
    "${PROJECT_SOURCE_DIR}/src/"
    "${CMAKE_BINARY_DIR}/src/"
)

include_directories (${fts3_include_dir})
include_directories (${PROJECT_SOURCE_DIR}/src/url-copy/)
include_directories (${PROJECT_SOURCE_DIR}/src/db/generic/)
include_directories (${PROJECT_SOURCE_DIR}/src/common/)
include_directories (${PROJECT_SOURCE_DIR}/src/cred/)
include_directories (/usr/lib64/glib-2.0/include)
include_directories (/usr/include/glib-2.0)
set(fts3_libs "-lfts3_config -lfts3_common -lfts3_db_generic")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(fts3_libs "-lboost_unit_test_framework ${fts3_libs}")
endif ()

# Web service / gsoap part.
set(fts3_server_gsoap_transfer_sources
    "${PROJECT_SOURCE_DIR}/src/server/ws/gsoap.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/RequestLister.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/JobSubmitter.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/GSoapDelegationHandler.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/ConfigurationHandler.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/gsoap_transfer_methods.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/gsoap_config_methods.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/gsoap_deleg_methods.cpp"
)

add_library(fts3_server_gsoap_transfer SHARED ${fts3_server_gsoap_transfer_sources})
set_target_properties(fts3_server_gsoap_transfer PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -w" )
target_link_libraries(fts3_server_gsoap_transfer "-lfts3_ws_ifce_server")
add_dependencies(fts3_server_gsoap_transfer fts3_ws_ifce_server)

# Find libuuid
find_package(LibUuid REQUIRED)
include_directories(${LIBUUID_INCLUDE_DIRS})

# Server lib
file(GLOB fts3_server_lib_sources RELATIVE "${PROJECT_SOURCE_DIR}/src/server" "*.cpp")
list(REMOVE_ITEM fts3_server_lib_sources "main.cpp")
add_library(fts3_server_lib SHARED ${fts3_server_lib_sources})
add_dependencies(fts3_server_lib fts3_server_gsoap_transfer)
target_link_libraries(fts3_server_lib 
	${fts3_libs} 
	-lfts3_server_gsoap_transfer 
	${LIBUUID_LIBRARIES} 
	-lboost_thread-mt 
	-lboost_regex 
	-lis_ifce 
	-lldap_r 
	-lglib-2.0 
	-lgridsite
	-lcgsi_plugin_voms_cpp
)

# Server executable
add_executable (fts3_server main.cpp)
add_dependencies(fts3_server fts3_server_lib)
target_link_libraries(fts3_server -lfts3_server_lib -lfts3_proxy )

# ------------------------------------------------------------------------
# man pages

set(MAN_INPUT_DIR "${PROJECT_SOURCE_DIR}/doc/man/server")
set(MAN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/man1")
set(MAN_INSTALL_DIR "/usr/share/man/man1")

file(MAKE_DIRECTORY ${MAN_OUTPUT_DIR})

find_package(GZip)

# gzip the manpage
add_custom_command(
	TARGET fts3_server
	POST_BUILD
	COMMAND ${GZIP} -c ${MAN_INPUT_DIR}/fts3_server > ${MAN_OUTPUT_DIR}/fts3_server.1.gz
)
#install it
install(
	FILES ${MAN_OUTPUT_DIR}/fts3_server.1.gz
	DESTINATION ${MAN_INSTALL_DIR}
)
# ------------------------------------------------------------------------


install(TARGETS         fts3_server_gsoap_transfer 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )

install(TARGETS         fts3_server_lib 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )

install(TARGETS         fts3_server 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )
