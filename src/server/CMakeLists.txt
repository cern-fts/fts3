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

set(fts_include_dir 
    "${PROJECT_SOURCE_DIR}/src/"
    "${CMAKE_BINARY_DIR}/src/"
)

include_directories (${fts_include_dir})
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/url-copy/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/db/generic/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/cred/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/ws-ifce/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/ws-ifce/)
include_directories (/usr/lib64/glib-2.0/include)
include_directories (/usr/lib/glib-2.0/include)
include_directories (/usr/include/glib-2.0)
set(fts_libs "-lfts_config -lfts_common -lfts_db_generic")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(fts_libs "-lboost_unit_test_framework ${fts_libs}")
endif ()

# Web service / gsoap part.
set(fts_server_gsoap_transfer_sources
#    "${PROJECT_SOURCE_DIR}/src/server/ws/gsoap.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/CGsiAdapter.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/InternalLogRetriever.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/AuthorizationManager.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/transfer/gsoap_transfer_methods.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/transfer/RequestLister.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/transfer/JobSubmitter.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/transfer/VersionResolver.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/transfer/LogFileStreamer.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/delegation/gsoap_deleg_methods.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/delegation/GSoapDelegationHandler.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/config/gsoap_config_methods.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/config/ConfigurationHandler.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/config/Configuration.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/config/StandaloneCfg.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/config/StandaloneSeCfg.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/config/StandaloneGrCfg.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/ws/config/PairCfg.cpp"
	"${PROJECT_SOURCE_DIR}/src/server/ws/config/SePairCfg.cpp"
	"${PROJECT_SOURCE_DIR}/src/server/ws/config/GrPairCfg.cpp"
)

add_library(fts_server_gsoap_transfer SHARED ${fts_server_gsoap_transfer_sources})
set_target_properties(fts_server_gsoap_transfer PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -w" )
add_dependencies(fts_server_gsoap_transfer fts_ws_ifce_server fts_ws_ifce_client fts_common fts_config fts_db_generic)
target_link_libraries(fts_server_gsoap_transfer fts_ws_ifce_server fts_ws_ifce_client fts_common fts_config fts_db_generic -lgridsite -lcgsi_plugin_voms_cpp)
set_target_properties(fts_server_gsoap_transfer PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/server
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)



# Find libuuid
find_package(LibUuid REQUIRED)
include_directories(${LIBUUID_INCLUDE_DIRS})

# Server lib
file(GLOB fts_server_lib_sources RELATIVE "${PROJECT_SOURCE_DIR}/src/server" "*.cpp")
list(REMOVE_ITEM fts_server_lib_sources "main.cpp")
add_library(fts_server_lib SHARED ${fts_server_lib_sources})
add_dependencies(fts_server_lib fts_server_gsoap_transfer)
target_link_libraries(fts_server_lib 
	${fts_libs} 
	-lfts_server_gsoap_transfer 
	${LIBUUID_LIBRARIES} 
	-lboost_thread-mt 
	-lboost_regex-mt 
	-lis_ifce 
	-lldap_r
	-lglib-2.0 
	-lgridsite
	-lcgsi_plugin_voms_cpp
	-lpugixml
)
set_target_properties(fts_server_lib            PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/server
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)



# Server executable
add_executable (fts_server main.cpp)
add_dependencies(fts_server fts_server_lib)
target_link_libraries(fts_server ${fts_libs} -lfts_server_lib -lfts_proxy -lfts_config -lfts_common -lfts_db_generic -lfts_server_gsoap_transfer -lgsoap++ -lfts_ws_ifce_server -lfts_ws_ifce_client)

# ------------------------------------------------------------------------
# man pages

set(MAN_INPUT_DIR "${PROJECT_SOURCE_DIR}/doc/man/server")
set(MAN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/man8")
set(MAN_INSTALL_DIR "/usr/share/man/man8")

file(MAKE_DIRECTORY ${MAN_OUTPUT_DIR})

find_package(GZip)

# gzip the manpage
add_custom_command(
	TARGET fts_server
	POST_BUILD
	COMMAND ${GZIP} -c ${MAN_INPUT_DIR}/fts_server > ${MAN_OUTPUT_DIR}/fts_server.8.gz
)
#install it
install(
	FILES ${MAN_OUTPUT_DIR}/fts_server.8.gz
	DESTINATION ${MAN_INSTALL_DIR}
)
# ------------------------------------------------------------------------


install(TARGETS         fts_server_gsoap_transfer 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )

install(TARGETS         fts_server_lib 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )

install(TARGETS         fts_server 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )
