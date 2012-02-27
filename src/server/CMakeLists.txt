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
set(fts3_libs "-lfts3_config -lfts3_common")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(fts3_libs "-lboost_unit_test_framework ${fts3_libs}")
endif ()

# Server lib
file(GLOB fts3_server_lib_sources RELATIVE "${PROJECT_SOURCE_DIR}/src/server" "*.cpp")
list(REMOVE_ITEM fts3_server_lib_sources "main.cpp")
#list(APPEND fts3_server_lib_sources "ws/ftsFileTransferSoapBindingService.cpp")
add_library(fts3_server_lib SHARED ${fts3_server_lib_sources})
target_link_libraries(fts3_server_lib ${fts3_libs})
target_link_libraries(fts3_server_lib -lboost_thread-mt)

# Server executable
add_executable (fts3_server main.cpp)
add_dependencies(fts3_server fts3_server_lib)
target_link_libraries(fts3_server -lfts3_server_lib)

