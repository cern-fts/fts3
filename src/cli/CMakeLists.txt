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

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

set(fts3_cli_source_files 
	ui/CliBase.cpp 
	SrvManager.cpp
	ServiceProxyHolder.cpp
)

set(fts3_cli_libs 
	fts3_ws_ifce_client
	cgsi_plugin_cpp 
	boost_program_options
)

set(fts3_common_libs 
	fts3_common 
	boost_thread-mt
)

set(fts3_include_dir 
    "${PROJECT_SOURCE_DIR}/src/"
    "${CMAKE_BINARY_DIR}/src/"
)
include_directories (${fts3_include_dir})

add_executable(fts3-transfer-submit 
	tools/fts3-transfer-submit.cpp 
	ui/SubmitTransferCli.cpp 
	${fts3_cli_source_files}
)
target_link_libraries (fts3-transfer-submit ${fts3_cli_libs} ${fts3_common_libs})

add_executable(fts3-transfer-status 
	tools/fts3-transfer-status.cpp 
	ui/TransferStatusCli.cpp 
	ui/JobIDCli.cpp	
	${fts3_cli_source_files}
)
target_link_libraries (fts3-transfer-status ${fts3_cli_libs})

add_executable(fts3-transfer-list 
	tools/fts3-transfer-list.cpp
	ui/DNCli.cpp 
	ui/VONameCli.cpp
	ui/ListTransferCli.cpp 
	${fts3_cli_source_files}
)
target_link_libraries (fts3-transfer-list ${fts3_cli_libs} ${fts3_common_libs})

add_executable(fts3-transfer-cancel 
	tools/fts3-transfer-cancel.cpp 
	ui/JobIDCli.cpp 
	${fts3_cli_source_files}
)
target_link_libraries (fts3-transfer-cancel ${fts3_cli_libs})

add_executable(fts3-transfer-getroles 
	tools/fts3-transfer-getroles.cpp 
	ui/DNCli.cpp 
	${fts3_cli_source_files}
)
target_link_libraries (fts3-transfer-getroles ${fts3_cli_libs})

add_executable(fts3-transfer-listvomanagers 
	tools/fts3-transfer-listvomanagers.cpp 
	ui/VONameCli.cpp 
	${fts3_cli_source_files}
)
target_link_libraries (fts3-transfer-listvomanagers ${fts3_cli_libs})


install(TARGETS         fts3-transfer-submit 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${BIN_INSTALL_DIR} )
	
install(TARGETS         fts3-transfer-status 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${BIN_INSTALL_DIR} )	
	
install(TARGETS         fts3-transfer-list 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${BIN_INSTALL_DIR} )		

	
