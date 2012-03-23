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
cmake_minimum_required (VERSION 2.6)

# ------------------------------------------------------------------------
# Set up include directories

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(fts3_include_dir
    "${PROJECT_SOURCE_DIR}/src/"
    "${CMAKE_BINARY_DIR}/src/"
)

include_directories (${fts3_include_dir})

# ------------------------------------------------------------------------
# Set up libs

set (fts3_cli_internal_libs
    fts3_ws_ifce_client
    fts3_common
)

set(fts3_cli_libs
    ${fts3_cli_internal_libs}
    cgsi_plugin_cpp
    boost_program_options
    boost_thread-mt
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(fts3_cli_libs boost_unit_test_framework ${fts3_cli_libs})
endif ()

# ------------------------------------------------------------------------
# Common CLI library

set(fts3_cli_transfer_common_sources
    ui/DNCli.cpp
    ui/CliBase.cpp
    ui/JobIDCli.cpp
    ui/VONameCli.cpp
    SrvManager.cpp
    ServiceProxyHolder.cpp
)

add_library(fts3_cli_transfer_common SHARED ${fts3_cli_transfer_common_sources})
target_link_libraries(fts3_cli_transfer_common ${fts3_cli_libs})
add_dependencies(fts3_cli_transfer_common ${fts3_cli_libs})

# ------------------------------------------------------------------------
# fts3-transfer-submit
add_executable(fts3-transfer-submit
    tools/fts3-transfer-submit.cpp
    ui/SubmitTransferCli.cpp
)
add_dependencies(fts3-transfer-submit fts3_cli_transfer_common)
target_link_libraries (fts3-transfer-submit fts3_cli_transfer_common)

# ------------------------------------------------------------------------
# fts3-transfer-status

add_executable(fts3-transfer-status
    tools/fts3-transfer-status.cpp
    ui/TransferStatusCli.cpp 
)
add_dependencies(fts3-transfer-status fts3_cli_transfer_common)
target_link_libraries (fts3-transfer-status fts3_cli_transfer_common)

# ------------------------------------------------------------------------
# fts3-transfer-list

add_executable(fts3-transfer-list
    tools/fts3-transfer-list.cpp
    ui/ListTransferCli.cpp 
)
add_dependencies(fts3-transfer-list fts3_cli_transfer_common)
target_link_libraries (fts3-transfer-list fts3_cli_transfer_common)

# ------------------------------------------------------------------------
# fts3-transfer-cancel

add_executable(fts3-transfer-cancel
    tools/fts3-transfer-cancel.cpp
)
add_dependencies(fts3-transfer-cancel fts3_cli_transfer_common)
target_link_libraries (fts3-transfer-cancel fts3_cli_transfer_common)

# ------------------------------------------------------------------------
# fts3-transfer-getroles

add_executable(fts3-transfer-getroles 
    tools/fts3-transfer-getroles.cpp
)
add_dependencies(fts3-transfer-getroles fts3_cli_transfer_common)
target_link_libraries (fts3-transfer-getroles fts3_cli_transfer_common)

# ------------------------------------------------------------------------
# fts3-transfer-listvomanagers
add_executable(fts3-transfer-listvomanagers
    tools/fts3-transfer-listvomanagers.cpp
)
add_dependencies(fts3-transfer-listvomanagers fts3_cli_transfer_common)
target_link_libraries (fts3-transfer-listvomanagers fts3_cli_transfer_common)

# ------------------------------------------------------------------------
# FTS3 config tools - common structures

set(fts3_cli_config_common_sources
    config/fts3_config_set_class.cpp
    config/parser.cpp
)

add_library(fts3_cli_config_common SHARED ${fts3_cli_config_common_sources})
target_link_libraries(fts3_cli_config_common ${fts3_cli_libs})
add_dependencies(fts3_cli_config_common ${fts3_cli_libs})

# ------------------------------------------------------------------------
# fts3-config-set command

add_executable(fts3-config-set
    config/fts3-config-set.cpp
)
add_dependencies(fts3-config-set fts3_cli_config_common)
target_link_libraries (fts3-config-set fts3_cli_config_common)

# ------------------------------------------------------------------------
# fts3-config-get command

add_executable(fts3-config-get 
    config/fts3-config-get.cpp
)
add_dependencies(fts3-config-get fts3_cli_config_common)
target_link_libraries (fts3-config-get fts3_cli_config_common)

# ------------------------------------------------------------------------

install(TARGETS
    fts3-transfer-submit
    fts3-transfer-status
    fts3-transfer-list
    fts3-transfer-listvomanagers
    fts3-transfer-getroles

    RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX}
    LIBRARY             DESTINATION ${BIN_INSTALL_DIR}
)
