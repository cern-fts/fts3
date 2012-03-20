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

# GSOAP file generation - general part
find_package(gSOAP)

set(generated_files_dir "${CMAKE_CURRENT_BINARY_DIR}/gsoap")
file(MAKE_DIRECTORY ${generated_files_dir})

set(gsoap_env_header_file "${PROJECT_SOURCE_DIR}/src/ws-ifce/gsoap/gsoap_env.h")

set(gsoap_env_source_files
    "${generated_files_dir}/envC.cpp"
)

add_custom_command(
    OUTPUT ${gsoap_env_source_files}
    DEPENDS ${gsoap_env_header_file}
    COMMAND ${GSOAP_SOAPCPP2} -d ${generated_files_dir} -penv ${gsoap_env_header_file}
)

# Generate transfer interface
set(fts3_transfer_wsdl "${PROJECT_SOURCE_DIR}/src/ws-ifce/fts3-transfer-0.0.0.wsdl")
set(gsoap_transfer_typemap "${PROJECT_SOURCE_DIR}/src/ws-ifce/gsoap/transfer_typemap.dat")
set(gsoap_transfer_header_file "${generated_files_dir}/gsoap_transfer_interface.h")
set(gsoap_transfer_prefix "transfer")

set(gsoap_transfer_source_files
    "${generated_files_dir}/${gsoap_transfer_prefix}C.cpp"
    "${generated_files_dir}/${gsoap_transfer_prefix}FileTransferSoapBindingService.cpp"
)

add_custom_command(
    OUTPUT ${gsoap_transfer_header_file}
    DEPENDS ${gsoap_transfer_typemap} ${fts3_transfer_wsdl}
    COMMAND ${GSOAP_WSDL2H} -e -y ${GSOAP_276_COMPAT_FLAGS} -t ${gsoap_transfer_typemap} ${fts3_transfer_wsdl} -o ${gsoap_transfer_header_file}
)

add_custom_command(
    OUTPUT ${gsoap_transfer_source_files}
    DEPENDS ${gsoap_transfer_header_file}
    COMMAND ${GSOAP_SOAPCPP2} -n -w -i -x -p ${gsoap_transfer_prefix} -q ${gsoap_transfer_prefix} -d ${generated_files_dir} -I ${GSOAP_IMPORT_DIR} ${gsoap_transfer_header_file} 
)

# Generate config interface
set(fts3_config_wsdl "${PROJECT_SOURCE_DIR}/src/ws-ifce/fts3-config-0.0.0.wsdl")
set(gsoap_config_typemap "${PROJECT_SOURCE_DIR}/src/ws-ifce/gsoap/config_typemap.dat")
set(gsoap_config_header_file "${generated_files_dir}/gsoap_config_interface.h")
set(gsoap_config_prefix "config")

set(gsoap_config_source_files
    "${generated_files_dir}/${gsoap_config_prefix}C.cpp"
    "${generated_files_dir}/${gsoap_config_prefix}SoapBindingService.cpp"
)

add_custom_command(
    OUTPUT  ${gsoap_config_header_file}
    DEPENDS  ${gsoap_config_typemap} ${fts3_config_wsdl} 
    COMMAND ${GSOAP_WSDL2H} -e -y ${GSOAP_276_COMPAT_FLAGS} -t ${gsoap_config_typemap} ${fts3_config_wsdl} -o ${gsoap_config_header_file}
)

add_custom_command(
    OUTPUT ${gsoap_config_source_files}
    DEPENDS ${gsoap_config_header_file}
    COMMAND ${GSOAP_SOAPCPP2} -n -w -i -x -q ${gsoap_config_prefix} -p ${gsoap_config_prefix} -d ${generated_files_dir}  -I ${GSOAP_IMPORT_DIR} ${gsoap_config_header_file}
)

add_library(fts3_ws_ifce_server SHARED 
    ${gsoap_config_source_files}
    ${gsoap_env_source_files}
    ${gsoap_transfer_source_files}
)
set_target_properties(fts3_ws_ifce_server PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -w" )
target_link_libraries(fts3_ws_ifce_server -lgsoap++)

# Client side library 
add_library(fts3_ws_ifce_client SHARED
    ${generated_files_dir}/${gsoap_transfer_prefix}C.cpp
    ${generated_files_dir}/${gsoap_transfer_prefix}FileTransferSoapBindingProxy.cpp
    ${generated_files_dir}/envC.cpp
    ${generated_files_dir}/${gsoap_config_prefix}C.cpp
    ${generated_files_dir}/${gsoap_config_prefix}SoapBindingProxy.cpp
)

set_target_properties(fts3_ws_ifce_client PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -w" )
target_link_libraries(fts3_ws_ifce_client -lgsoap++)

