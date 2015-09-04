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


# GSOAP file generation - general part
find_package(gSOAP)

set(generated_files_dir "${CMAKE_CURRENT_BINARY_DIR}/gsoap")
file(MAKE_DIRECTORY ${generated_files_dir})

set(gsoap_env_header_file "${PROJECT_SOURCE_DIR}/src/ws-ifce/gsoap/gsoap_env.h")

set(gsoap_env_generated_files
    "${generated_files_dir}/envC.cpp"
)
set(gsoap_env_source_files
    "${PROJECT_SOURCE_DIR}/src/ws-ifce/gsoap/gsoap_env.cpp"
    ${gsoap_env_generated_files}
)

add_custom_command(
    OUTPUT ${gsoap_env_generated_files}
    DEPENDS ${gsoap_env_header_file}
    COMMAND ${GSOAP_SOAPCPP2} -d ${generated_files_dir} -penv ${gsoap_env_header_file}
)

# Generate transfer & config interface
set(fts_transfer_wsdl "${PROJECT_SOURCE_DIR}/src/ws-ifce/fts3-transfer-0.0.0.wsdl")
set(fts_config_wsdl "${PROJECT_SOURCE_DIR}/src/ws-ifce/fts3-config-0.0.0.wsdl")
set(fts_delegation_wsdl "${PROJECT_SOURCE_DIR}/src/ws-ifce/fts3-delegation-0.0.0.wsdl")

set(wsdl_files 
	${fts_transfer_wsdl} 
	${fts_config_wsdl} 
	${fts_delegation_wsdl}
)

set(gsoap_typemap "${PROJECT_SOURCE_DIR}/src/ws-ifce/gsoap/typemap.dat")
set(gsoap_header_file "${generated_files_dir}/gsoap_interface.h")
set(delegation_header_file "${generated_files_dir}/delegation_interface.h")
set(gsoap_prefix "fts3")
set(delegation_prefix "delegation")

set(gsoap_source_files
	"${generated_files_dir}/${gsoap_prefix}C.cpp"
    "${generated_files_dir}/${gsoap_prefix}Client.cpp"
    "${generated_files_dir}/${gsoap_prefix}Server.cpp"
)

set(delegation_source_files
	"${generated_files_dir}/${delegation_prefix}C.cpp"
    "${generated_files_dir}/${delegation_prefix}Client.cpp"
)

add_custom_command(
    OUTPUT ${gsoap_header_file}
    DEPENDS ${gsoap_typemap} ${fts_transfer_wsdl} 
    COMMAND ${GSOAP_WSDL2H} -e -y ${GSOAP_276_COMPAT_FLAGS} -t ${gsoap_typemap} ${wsdl_files} -o ${gsoap_header_file}
)

add_custom_command(
    OUTPUT ${gsoap_source_files}
    DEPENDS ${gsoap_header_file}
    COMMAND ${GSOAP_SOAPCPP2} -n -w -x -p ${gsoap_prefix} -q ${gsoap_prefix}  -d ${generated_files_dir} -I ${GSOAP_IMPORT_DIR} ${gsoap_header_file} 
)

# generate delegation stubs for client!
add_custom_command(
    OUTPUT ${delegation_header_file}
    DEPENDS ${gsoap_typemap} ${fts_delegation_wsdl} 
    COMMAND ${GSOAP_WSDL2H} -e -y ${GSOAP_276_COMPAT_FLAGS} -t ${gsoap_typemap} ${fts_delegation_wsdl} -o ${delegation_header_file}
)

# generate delegation stubs for client!
add_custom_command(
    OUTPUT ${delegation_source_files}
    DEPENDS ${delegation_header_file}
    COMMAND ${GSOAP_SOAPCPP2} -C -n -w -x -p ${delegation_prefix} -d ${generated_files_dir} -I ${GSOAP_IMPORT_DIR} ${delegation_header_file} 
)

# Generated header files
include_directories("${generated_files_dir}")

add_library(fts_ws_ifce_server SHARED 
    ${gsoap_env_source_files}
    ${generated_files_dir}/${gsoap_prefix}C.cpp
    ${generated_files_dir}/${gsoap_prefix}Server.cpp
)
add_dependencies(fts_ws_ifce_server fts_common)
set_target_properties(fts_ws_ifce_server PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -w" )
add_dependencies(fts_ws_ifce_server fts_common)
target_link_libraries(fts_ws_ifce_server -lgsoap++)

# Client side library 
add_library(fts_ws_ifce_client SHARED
    ${gsoap_env_source_files}
    ${generated_files_dir}/${gsoap_prefix}C.cpp
    ${generated_files_dir}/${gsoap_prefix}Client.cpp
)
add_dependencies(fts_ws_ifce_client fts_common)
set_target_properties(fts_ws_ifce_client PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -w" )
target_link_libraries(fts_ws_ifce_client -lgsoap++)

# Client side delegation library 
add_library(fts_delegation_api_cpp SHARED
    ${PROJECT_SOURCE_DIR}/src/ws-ifce/gsoap/gsoap_env.cpp
    ${generated_files_dir}/${delegation_prefix}C.cpp
    ${generated_files_dir}/${delegation_prefix}Client.cpp
)

set_target_properties(fts_delegation_api_cpp PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/ws-ifce)
set_target_properties(fts_delegation_api_cpp PROPERTIES VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set_target_properties(fts_delegation_api_cpp PROPERTIES SOVERSION ${VERSION_MAJOR})
set_target_properties(fts_delegation_api_cpp PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(fts_delegation_api_cpp -lgsoap++)

set_target_properties(fts_ws_ifce_server PROPERTIES 
						LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/ws-ifce VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} SOVERSION ${VERSION_MAJOR} CLEAN_DIRECT_OUTPUT 1)


set_target_properties(fts_ws_ifce_client PROPERTIES 
						LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/ws-ifce VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} SOVERSION ${VERSION_MAJOR} CLEAN_DIRECT_OUTPUT 1)



install(TARGETS         fts_ws_ifce_client fts_ws_ifce_server fts_delegation_api_cpp
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )

