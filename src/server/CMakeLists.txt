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
set(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts3_include_dir})
set(fts3_libs "-lfts3_config -lfts3_common -lfts3_db_generic")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(fts3_libs "-lboost_unit_test_framework ${fts3_libs}")
endif ()

# GSOAP file generation
find_package(gSOAP)

set(generated_files_dir "${CMAKE_CURRENT_BINARY_DIR}/ws")
set(gsoap_typemap "${PROJECT_SOURCE_DIR}/src/server/ws/typemap.dat")
set(fts3_wsdl "${PROJECT_SOURCE_DIR}/src/ws-ifce/fts3-1.0.0.wsdl")
set(gsoap_header_file "${generated_files_dir}/ftsH.h")

set(gsoap_source_files 
    "${generated_files_dir}/ftsC.cpp" 
    "${generated_files_dir}/ftsFileTransferSoapBindingService.cpp" 
    "${PROJECT_SOURCE_DIR}/src/server/gsoap.cpp"         
    "${PROJECT_SOURCE_DIR}/src/server/ws/FtsServiceTask.cpp"         
    "${PROJECT_SOURCE_DIR}/src/server/gsoap_transfer_methods.cpp"         
)

include_directories (${generated_files_dir})

file(MAKE_DIRECTORY ${generated_files_dir})

add_custom_command(	
    OUTPUT  ${gsoap_header_file}
	DEPENDS  ${gsoap_typemap} ${fts3_wsdl} 
	COMMAND ${GSOAP_WSDL2H} ${GSOAP_276_COMPAT_FLAGS} -e -y  -t ${gsoap_typemap}  ${fts3_wsdl}  -o ${gsoap_header_file})


add_custom_command(	
    OUTPUT ${gsoap_source_files}
	DEPENDS ${gsoap_header_file} 
    COMMAND ${GSOAP_SOAPCPP2} -n -w -i -S -x -p fts -d ${generated_files_dir} -I ${GSOAP_IMPORT_DIR} ${gsoap_header_file} )


# gSOAP web service lib
set(fts3_server_gsoap_lib_sources "${gsoap_source_files}" )
add_library(fts3_server_gsoap_lib SHARED ${fts3_server_gsoap_lib_sources})
set_target_properties(fts3_server_gsoap_lib PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -w" )
target_link_libraries(fts3_server_gsoap_lib -lgsoap++)

# Server lib
file(GLOB fts3_server_lib_sources RELATIVE "${PROJECT_SOURCE_DIR}/src/server" "*.cpp")
list(REMOVE_ITEM fts3_server_lib_sources "main.cpp")
list(REMOVE_ITEM fts3_server_lib_sources "gsoap_transfer_methods.cpp")
add_library(fts3_server_lib SHARED ${fts3_server_lib_sources})
add_dependencies(fts3_server_lib fts3_server_gsoap_lib)
target_link_libraries(fts3_server_lib ${fts3_libs} -lfts3_server_gsoap_lib -luuid -lboost_thread-mt)

# Server executable
add_executable (fts3_server main.cpp)
add_dependencies(fts3_server fts3_server_lib)
target_link_libraries(fts3_server -lfts3_server_lib)

